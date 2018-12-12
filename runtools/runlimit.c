/* See LICENSE file for copyright and license details. */
#include <sys/resource.h>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

/* additional stderr: */
#define warn(...) \
{ if (verbose) eputs(getprogname(), " warning: ", __VA_ARGS__); }

#define barf(...) fatal(99, "barf on programming error: ", __VA_ARGS__)

/* rlimits in environment: */
static int opt_E;
/* rlimits in file: */
static int opt_F;
static char *arg_F;
/* stderr on warnings (option -v): */
static int verbose;

struct runlimit {
	char *name;
	char *env; /* XXX, unused */
	int id;
	int is_set;
	rlim_t rl_soft;
	rlim_t rl_hard;
};

struct runlimit runlimits[] = {
	{ "RLIMIT_AS",      "RUNLIMIT_AS",      -1, 0, 0, 0 },
	{ "RLIMIT_VMEM",    "RUNLIMIT_VMEM",    -1, 0, 0, 0 },
	{ "RLIMIT_CORE",    "RUNLIMIT_CORE",    -1, 0, 0, 0 },
	{ "RLIMIT_DATA",    "RUNLIMIT_DATA",    -1, 0, 0, 0 },
	{ "RLIMIT_FSIZE",   "RUNLIMIT_FSIZE",   -1, 0, 0, 0 },
	{ "RLIMIT_MEMLOCK", "RUNLIMIT_MEMLOCK", -1, 0, 0, 0 },
	{ "RLIMIT_NOFILE",  "RUNLIMIT_NOFILE",  -1, 0, 0, 0 },
	{ "RLIMIT_NPROC",   "RUNLIMIT_NPROC",   -1, 0, 0, 0 },
	{ "RLIMIT_RSS",     "RUNLIMIT_RSS",     -1, 0, 0, 0 },
	{ "RLIMIT_STACK",   "RUNLIMIT_STACK",   -1, 0, 0, 0 },
	{ "RLIMIT_CPU",     "RUNLIMIT_CPU",     -1, 0, 0, 0 },
	/* list terminal: */
	{ NULL, NULL, -1, 0, 0, 0 }
};

/* runlimit_ndx()
 *   lookup name in runlimits[]
 *   return:
 *    >= 0: index to match in runlimits[]
 *      -1: not found
 */
static int
runlimit_ndx(const char *name)
{
	int i;

	for (i = 0; runlimits[i].name; i++)
		if (!cstr_cmp(name, runlimits[i].name))
			return i;

	/* not found: */
	return -1;
}


/* runlimit_setup_opt()
 *   set runlimit for name with arg, as given with option character opt
 *   abort on fail
 */
static void
runlimit_setup_opt(const char *name, const char *arg, char opt)
{
	struct rlimit rlim;
	uint32_t u;
	int ndx, id;
	const char *z;
	char nbuf[NFMT_SIZE];
	char option[2];

	/* shouldn't happen from command line! */
	if ((ndx = runlimit_ndx(name)) < 0)
		barf(name, " is blaargh!");

	option = (char [2]){ opt, '\0' };

	if ((id = rlimit_lookup(name)) < 0) {
		warn("option -", option, ": resource ", name,
		     " not supported on this system");
		return;
	}

	if (getrlimit(id, &rlim) < 0)
		fatal_syserr("option -", option,
		             ": failure getrlimit() for resource ", name);

	runlimits[ndx].rl_hard = rlim.rlim_max;
	if ((arg[0] == '_') || (arg[0] == '^') || (arg[0] == '=')) {
		runlimits[ndx].rl_soft = rlim.rlim_max;
	} else {
		if ((z = nuscan_uint32(&u, arg)))
			fatal_usage("option -", option,
			            ": non-numeric argument for resource ",
			            name, ": ", arg);
		if (u < rlim.rlim_max) {
			runlimits[ndx].rl_soft = (rlim_t)u;
		} else {
			warn("option -", option,
			     ": truncating runlimit request for ", name,
			     " to hard limit: ",
			     nfmt_uint32(nbuf,(uint32_t)rlim.rlim_max));
			runlimits[ndx].rl_soft = rlim.rlim_max;
		}
	}
	runlimits[ndx].is_set++;
	runlimits[ndx].id = id;
}


/* runlimit_setup_ndx()
 *   setup runlimits[ndx] with runlimit specified in arg obtained from source
 *   ndx previously obtained from runlimit_ndx()
 *   skips any setup on any previous setting found
 *   abort on failure
 */
static void
runlimit_setup_ndx(int ndx, const char *arg, const char *source)
{
	struct rlimit rlim;
	uint32_t u;
	int id;
	const char *name, *z;
	char nbuf[NFMT_SIZE];

	/* skipping on previous setup: */
	if (runlimits[ndx].is_set)
		return;

	name = runlimits[ndx].name;

	if ((!arg) || (!*arg))
		fatal_usage("NULL or empty argument for resource ", name,
		            " found in ", source);

	if ((id = rlimit_lookup(name)) < 0) {
		warn("resource ", name, " from ", source,
		     "not supported on this system");
		return;
	}

	if (getrlimit(id, &rlim) < 0)
		fatal_syserr("failure getrlimit() for resource ", name,
		             " from ", source);

	runlimits[ndx].rl_hard = rlim.rlim_max;
	if ((arg[0] == '_') || (arg[0] == '^') || (arg[0] == '=')) {
		runlimits[ndx].rl_soft = rlim.rlim_max;
	} else {
		if ((z = nuscan_uint32(&u, arg)))
			fatal_usage("non-numeric argument for resource ",
			            name, " from ", source, ": ", arg);
		if (u < rlim.rlim_max) {
			runlimits[ndx].rl_soft = (rlim_t)u;
		} else {
			warn("truncating runlimit request for ", name,
			     " from ", source, " to hard limit: ",
			     nfmt_uint32(nbuf, (uint32_t) rlim.rlim_max));
			runlimits[ndx].rl_soft = rlim.rlim_max;
		}
	}
	runlimits[ndx].is_set++;
	runlimits[ndx].id = id;
}


/* do_rlimfile()
 *   read RLIMIT_* values from rlimfile
 *   abort on failure
 */
static void
do_file(const char *rlimfile)
{
	dynstr_t L;
	ioq_t q;
	size_t lineno, split;
	int eof, fd, ndx, r;
	const char *source;
	char *line, *key, *val;
	char nbuf[NFMT_SIZE];
	uchar_t qbuf[IOQ_BUFSIZE];

	source = rlimfile;

	if (!cstr_cmp(rlimfile, "-")) {
		fd = 0;
		source = "<stdin>";
	} else {
		if ((fd = open(rlimfile, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open ", rlimfile);
	}

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	L = dynstr_INIT();
	eof = 0;
	lineno = 0;

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading ", source);

		lineno++;

		if (!r) {
			/* set terminal condition: */
			eof++;
			if (!(dynstr_STR(&L)) || !(dynstr_LEN(&L)))
				break;
			/* else:
			 * eof was encountered after partial line read
			 * (last line not terminated with '\n')
			 * proceed through the end of this loop
			 */
		}

		/* work directly on string buffer: */
		line = dynstr_STR(&L);
		cstr_trim(line);

		/* skip empty lines and comments: */
		if ((line[0] == '\0') || (line[0] == '#'))
			continue;

		/* parse line into key, value: */
		key = line;
		split = cstr_pos(key, '=');
		if (key[split] == '=') {
			val = &line[split + 1];
			key[split] = '\0';
			/* trim whitespace around '=': */
			cstr_rtrim(key);
			cstr_ltrim(val);
		} else {
			errno = EPROTO;
			fatal_syserr("empty value or format error found in ",
			             source, ", line ",
			             nfmt_uint32(nbuf, (uint32_t) lineno));
		}

		/* skip empty key: */
		if (key[0] == '\0')
			continue;

		/* setup resource: */
		if ((ndx = runlimit_ndx(key)) < 0) {
			errno = EPROTO;
			/* fail on bogus resource name: */
			fatal_syserr("unknown resource found in ", source,
			             ", line ",
			             nfmt_uint32(nbuf, (uint32_t)lineno), ": ",
			             key);
		}
		/* else: */
		runlimit_setup_ndx(ndx, val, source);
	}

	/* success: */
	if (fd)
		close(fd);
}


/* do_environment()
 *   scan runlimits[] for matching keys in environment
 *   on found keys, set runlimit with runlimit_setup_ndx()
 */
static void
do_environment(void)
{
	int i;
	char *val;

	for (i = 0; runlimits[i].name; i++)
		if ((val = getenv(runlimits[i].name)))
			runlimit_setup_ndx(i, val, "environment");
}

/* do_runlimits()
 *   scan through runlimits[]
 *   call setrlimit() for any entries that have been setup
 *   abort on failure
 */
static void
do_runlimits(void)
{
	struct rlimit rlim;
	int i;

	for (i = 0; runlimits[i].name; i++) {
		if (!runlimits[i].is_set)
			continue;

		rlim.rlim_cur = runlimits[i].rl_soft;
		rlim.rlim_max = runlimits[i].rl_hard;
		if (setrlimit(runlimits[i].id, &rlim) < 0)
			fatal_syserr("failure setrlimit() for resource ",
			             runlimits[i].name);
	}
}

static void
usage(void)
{
	usage_align_init();
	eputs("usage: ", getprogname(),
	      " [-E] [-F file] [-a availmem] [-c coresize] [-d databytes]\n", S,
	      " [-f filesize] [-l lockbytes] [-m membytes] [-o openfiles]\n", S,
	      " [-p processes] [-r rssbytes] [-s stackbytes] [-t cpusecs]\n", S,
	      " program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	setprogname(argv[0]);

	ARGBEGIN {
	case 'v':
		verbose++;
		break;
	case 'E':
		opt_E++;
		break;
	case 'F':
		opt_F++;
		arg_F = EARGF(usage());
		break;
	case 'a':
		runlimit_setup_opt("RLIMIT_AS", EARGF(usage()), ARGC());
		break;
	case 'c':
		runlimit_setup_opt("RLIMIT_CORE", EARGF(usage()), ARGC());
		break;
	case 'd':
		runlimit_setup_opt("RLIMIT_DATA", EARGF(usage()), ARGC());
		break;
	case 'f':
		runlimit_setup_opt("RLIMIT_FSIZE", EARGF(usage()), ARGC());
		break;
	case 'l':
		runlimit_setup_opt("RLIMIT_MEMLOCK", EARGF(usage()), ARGC());
		break;
	case 'm':
		/* all of _AS/_VMEM _DATA _STACK _MEMLOCK */
		runlimit_setup_opt("RLIMIT_AS", EARGF(usage()), ARGC());
		runlimit_setup_opt("RLIMIT_DATA", EARGF(usage()), ARGC());
		runlimit_setup_opt("RLIMIT_MEMLOCK", EARGF(usage()), ARGC());
		runlimit_setup_opt("RLIMIT_STACK", EARGF(usage()), ARGC());
	case 'o':
		runlimit_setup_opt("RLIMIT_NOFILE", EARGF(usage()), ARGC());
		break;
	case 'p':
		runlimit_setup_opt("RLIMIT_NPROC", EARGF(usage()), ARGC());
		break;
	case 'r':
		runlimit_setup_opt("RLIMIT_RSS", EARGF(usage()), ARGC());
		break;
	case 's':
		runlimit_setup_opt("RLIMIT_STACK", EARGF(usage()), ARGC());
		break;
	case 't':
		runlimit_setup_opt("RLIMIT_CPU", EARGF(usage()), ARGC());
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		fatal_usage("missing program argument");

	/* precedence:
	 **   cmdline overrides all
	 **   rlimit file overrides environment
	 **   environment if not otherwise set
	 */
	/* setup rlimits from file: */
	if (opt_F)
		do_file(arg_F);

	/* setup rlimits from environment */
	if (opt_E)
		do_environment();

	/* process rlimits: */
	do_runlimits();

	/* execvx() provides path search for prog */
	execvx(argv[0], argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}
