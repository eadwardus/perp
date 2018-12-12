/* See LICENSE file for copyright and license details. */
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static int warn;

#define fatal_alloc(...)  fatal(111, "allocation failure: ", __VA_ARGS__)

#define warning(...) \
{ if (warn) { eputs(getprogname(), ": warning: ", __VA_ARGS__); } }

/*
 * runlock
 */

static void
do_setlock(const char *arg, pid_t pid)
{
	int block, fd;
	const char *lockfile;

	block = 0;
	lockfile = arg;

	if (arg[0] == ':') {
		if (!arg[1])
			fatal_usage("empty lockfile argument with option ",
			            (pid ? "-P" : "-L"));
		block = 1;
		lockfile = &arg[1];
	}

	fd = pidlock_set(lockfile, pid, (block ? PIDLOCK_WAIT : PIDLOCK_NOW));
	if (fd < 0)
		fatal_syserr("unable to acquire lock on", lockfile);
}



/*
 * runargs
 */

static void
arglist_add(const char *arg)
{
	char *newarg;

	/* NULL arg acceptable to terminate arglist: */
	newarg = NULL;

	if (!arg)
		if (!(newarg = cstr_dup(arg)))
			fatal_syserr("failure duplicating new argv");

	if (dynstuf_push(&arglist, newarg) < 0)
		fatal_syserr("failure allocating new argv");
}


static void
do_argfile(const char *optc, const char *argfile)
{
	dynstr_t L;
	ioq_t q;
	int eof, fd, r;
	char *line;
	uchar_t qbuf[IOQ_BUFSIZE];

	if ((fd = open(argfile, O_RDONLY | O_NONBLOCK)) < 0)
		fatal_syserr("unable to open file for option -", optc, " ",
		             argfile);

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	L = dynstr_INIT();

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading argument file for option -",
			              optc, " ", argfile);

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

		/* add this argument: */
		arglist_add(line);
	}

	/* success: */
	if (fd)
		close(fd);
}



/*
 * runenv
 */

static char *
val_unescape(char *s)
{
	char *c, *new;

	c = s;
	new = s;

	for (; *c; c++) {
		if (*c != '\\') {
			*new++ = *c;
			continue;
		}
		c++;
		switch (*c) {
			/* recognize a few escape sequences in value string: */
		case '\\':
			*new++ = '\\';
			break;
		case 'n':
			*new++ = '\n';
			break;
		case 't':
			*new++ = '\t';
			break;
			/* "protected" space (use to preserve from cstr_trim()): */
		case '_':
			*new++ = ' ';
			break;
			/* if string ends with single backslash: */
		case '\0':
			*new++ = '\\';
			continue;
			break;
		default:
			/* escape sequence not defined, leave verbatim: */
			*new++ = '\\';
			*new++ = *c;
			break;
		}
	}

	*new = '\0';

	return s;
}


static void
do_envfile(const char *optc, const char *arg)
{
	dynstr_t L;
	ioq_t q;
	size_t split;
	int eof, fd, r;
	char *line;
	char *key, *val;
	uchar_t qbuf[IOQ_BUFSIZE];

	if ((fd = open(arg, O_RDONLY | O_NONBLOCK)) < 0)
		fatal_syserr("failure opening file for option -", optc,
		             ": ", arg);

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	eof = 0;
	L = dynstr_INIT();

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading file for option -",
			             optc, ": ", arg);

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
			/* process escape sequences in val: */
			val_unescape(val);
		} else {
			/* no value sets up delete of existing variable: */
			val = NULL;
		}

		/* skip empty key: */
		if (key[0] == '\0')
			continue;

		/* add new environment variable
		 * (null value sets up delete of existing variable)
		 */
		if (newenv_set(key, val) < 0)
			fatal_syserr("failure allocating new environment");
	}

	/* success: */
	if (fd)
		close(fd);
}


static void
do_envdir(const char *optc, const char *arg)
{
	ioq_t q;
	DIR *dir;
	dynstr_t L;
	struct dirent *d;
	int err, fd, fd_orig;
	char *line;
	uchar_t qbuf[IOQ_BUFSIZE];

	if ((fd_orig = open(".", O_RDONLY | O_NONBLOCK)) < 0)
		fatal_syserr("unable to open current directory (option -",
		             optc, " ", arg, ")");
	if ((err = chdir(arg)) < 0)
		fatal_syserr("unable to chdir (option -", optc, " ", arg, ")");

	if (!(dir = opendir(".")))
		fatal_syserr("unable to open directory (option -", optc,
		             " ", arg, ")");

	L = dynstr_INIT()

	for (;;) {
		errno = 0;
		if (!(d = readdir(dir))) {
			if (errno)
				fatal_syserr("unable to read directory"
				             " (option -", optc, " ", arg, ")");
			/* else all done: */
			break;
		}

		/* skip any dot files: */
		if (d->d_name[0] == '.')
			continue;

		if ((fd = open(d->d_name, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open ", arg, "/",
			             d->d_name, " (option -", optc, " ",
			             arg, ")");

		/* prepare ioq buffer and recycle line buffer: */
		ioq_init(&q, fd, qbuf, sizeof qbuf, &read);
		dynstr_CLEAR(&L);

		/* one line read: */
		if ((err = ioq_getln(&q, &L)) < 0)
			fatal_syserr("unable to read ", arg, "/",
			             d->d_name, " (option -", optc, " ",
			             arg, ")");
		close(fd);

		/* work directly on line buffer: */
		if ((line = dynstr_STR(&L))) {
			cstr_trim(line);
			val_unescape(line);
		}

		/* add new environmental variable
		 * (null value sets up delete of existing variable)
		 */
		err = newenv_set(d->d_name, ((line && line[0]) ? line : NULL));
		if (err < 0)
			fatal_syserr("failure allocating new environment");
	}

	closedir(dir);
	if (fchdir(fd_orig) < 0)
		fatal_syserr("failure changing to original directory (option -",
		             optc, " ", arg, ")");
	close(fd_orig);
}


static void
do_env(const char *optc, const char *arg)
{
	struct stat sb;

	if (stat(arg, &sb) < 0)
		fatal_syserr("failure stat() on path argument for option -",
		             optc, ": ", arg);

	if (S_ISREG(sb.st_mode))
		do_envfile(optc, arg);
	else if (S_ISDIR(sb.st_mode))
		do_envdir(optc, arg);

	fatal_usage("argument is neither file nor directory for option -",
	            optc, ": ", arg);
}



/*
 * runenv + envuid
 */

static void
do_envuid(const char *user)
{
	struct passwd *pw;
	char nfmt[NFMT_SIZE];
	int err;

	if (!(pw = getpwnam(user)))
		fatal_usage("no user account found for envuid option -U: ",
		            user);

	if ((err = newenv_set("GID", nfmt_uint32(nfmt, (uint32_t) pw->pw_gid))))
		fatal_syserr("failure allocating new environment");

	if ((err = newenv_set("UID", nfmt_uint32(nfmt, (uint32_t) pw->pw_uid))))
		fatal_syserr("failure allocating new environment");
}



/*
 * runlimit
 */

static void
runlimit_set(const char *rdef, const char *arg)
{
	struct rlimit rlim;
	uint32_t u;
	int r;
	const char *z;

	if ((r = rlimit_lookup(rdef)) < 0) {
		warning("resource limit ", rdef,
		        " not supported on this system");
		return;
	}

	if (getrlimit(r, &rlim) < 0)
		fatal_syserr("failure getrlimit() for resource ", rdef);

	if (arg[0] == '!') {
		rlim.rlim_cur = rlim.rlim_max;
	} else {
		if ((z = nuscan_uint32(&u, arg)))
			fatal_usage("bad numeric argument for runlimit"
			            " option -R :", arg);
		rlim.rlim_cur = (u < rlim.rlim_max ? u : rlim.rlim_max);
	}

	if (setrlimit(r, &rlim) < 0)
		fatal_syserr("failure setrlimit() for resource ", rdef);
}


static void
do_runlimit(const char *arg_rlimit)
{
	size_t tok;
	char nbuf[40];
	char r;
	const char *arg;

	arg = arg_rlimit;

	while (*arg != '\0') {
		r = *arg;
		arg++;
		if (*arg != '=')
			fatal_usage("bad format in runlimit argument ",
			            arg_rlimit);
		arg++;
		if ((tok = cstr_pos(arg, ':')) > 0) {
			if (tok > sizeof(nbuf)) {
				fatal_usage("bad format in runlimit argument ",
				            arg_rlimit);
			}
			buf_copy(nbuf, arg, tok);
			arg += tok;
		}
		nbuf[tok] = '\0';
		if (nbuf[0] != '\0') {
			switch (r) {
			case 'a':
				runlimit_set("RLIMIT_AS", nbuf);
				break;
			case 'c':
				runlimit_set("RLIMIT_CORE", nbuf);
				break;
			case 'd':
				runlimit_set("RLIMIT_DATA", nbuf);
				break;
			case 'f':
				runlimit_set("RLIMIT_FSIZE", nbuf);
				break;
			case 'm':
				runlimit_set("RLIMIT_AS", nbuf);
				runlimit_set("RLIMIT_DATA", nbuf);
				runlimit_set("RLIMIT_STACK", nbuf);
				runlimit_set("RLIMIT_MEMLOCK", nbuf);
				break;
			case 'o':
				runlimit_set("RLIMIT_NOFILE", nbuf);
				break;
			case 'p':
				runlimit_set("RLIMIT_NPROC", nbuf);
				break;
			case 'r':
				runlimit_set("RLIMIT_RSS", nbuf);
				break;
			case 's':
				runlimit_set("RLIMIT_STACK", nbuf);
				break;
			case 't':
				runlimit_set("RLIMIT_CPU", nbuf);
				break;
			default:
				fatal_usage("bad format in runlimit argument ",
				            arg_rlimit);
				break;
			}
		}
		if (*arg == ':')
			arg++;
	}
}




/*
 * runuid
 */

static void
do_setuid(const char *user)
{
	struct passwd *pw;
	gid_t gid;
	uid_t uid;

	if (getuid())
		fatal_usage("must have root privilege to setuid");

	if (!(pw = getpwnam(user)))
		fatal_usage("no user account found for setuid option -u: ",
		            user);

	uid = pw->pw_uid;
	gid = pw->pw_gid;

	if (setgroups(1, &gid) < 0)
		fatal_syserr("failure setgroups() for setuid option -u: ",
		             user);

	if (setgid(gid) < 0)
		fatal_syserr("failure setgid() for setuid option -u: ", user);

	if (setuid(uid) < 0)
		fatal_syserr("failure setuid() for setuid option -u: ", user);
}



/*
 * fdset
 */
static void
fdset_redirect(int fd, char op, const char *target)
{
	int fdx;
	char t;

	t = target[0];

	if (!((t == '/') || (t == '.') || (t == '%')))
		fatal_usage("bad target specification in fdset argument ",
		            target);

	if (t == '%') {
		if (target[1] != '\0')
			fatal_usage("bad target specification in"
			            " fdset argument ", target);
		/* else: */
		target = "/dev/null";
	}

	fdx = -1;

	switch (op) {
	case '<': /* input redirection */
		if ((fdx = open(target, O_RDONLY)) < 0)
			fatal_syserr("failure opening ", target,
			             " for input redirection");
		break;
	case '>': /* output redirection, clobber */
		if ((fdx = open(target, O_WRONLY|O_CREAT|O_APPEND|O_TRUNC,
		     0600)) < 0)
			fatal_syserr("failure opening ", target,
			             " for output redirection");
		break;
	case '+': /* output redirection, append */
		if ((fdx = open(target, O_WRONLY|O_CREAT|O_APPEND, 0600)) < 0)
			fatal_syserr("failure opening ", target,
			             " for output redirection");
		break;
	}

	if (fd_move(fd, fdx) < 0)
		fatal_syserr("failure duplicating file descriptor for ",
		             target);
}

static void
fdset_duplicate(int fd, char target)
{
	int fdx;

	if (target == '!') {
		if (close(fd) < 0)
			warning("failure on closing file descriptor");
		return;
	}

	/* else: */
	fdx = (int)(target - '0');
	if ((fdx < 0) || (fdx > 9))
		fatal_usage("bad fd target for fdset duplication");

	if (fd_dupe(fd, fdx) < 0)
		fatal_syserr("failure duplicating file descriptor");
}

static
void do_fdset(const char *arg_fdset)
{
	size_t tok;
	int fd;
	char op;
	char target[400];
	const char *arg = arg_fdset;

#if 0
	parse instructions from argument string in the format:
	    <fd> <op> <target> [:...]
	where:
	   <fd>: single ascii numeral, 0..9
	   <op>: '<' | '>' | '+' | '='
	   <target> : depends on <op>:
	     '<' | '>' | '+' :
	         redirection operator, target is:
	             <pathname>, (must begin with '.' or '/'); or
	             "%", a special string interpreted as "/dev/null"
	     '=' :
	         duplication operator, target is:
	             single ascii numeral, 0..9; or
	             "!", instruction to close <fd>

	concatenation:
	    multiple instructions may be concatenated with ':'

	examples:
	    "0</dev/null"      redirect stdin reading from /dev/null
	    "2+./error.log"    redirect stderr writing to ./error.log in append mode
	    "2=1"              duplicate stderr to stdin
	    "0=!"              close stdin
	    "0<%:1>%:2>%"      redirect stdin, stdout, stderr to /dev/null
#endif
	while (*arg != '\0') {
		fd = (int)(*arg - '0');
		if ((fd < 0) || (fd > 9))
			fatal_usage("bad fd numeral in fdset argument ",
			            arg_fdset);
		++arg;
		op = *arg;
		if (!((op == '<') || (op == '>') ||
		      (op == '+') || (op == '=')))
			fatal_usage("bad op format in fdset argument ",
			            arg_fdset);
		++arg;
		if (!(tok = cstr_pos(arg, ':')))
			fatal_usage("empty target in fdset argument ",
			            arg_fdset);
		/* (else tok > 0) */
		if (tok > sizeof(target))
			fatal_usage("target too long in fdset argument ",
			            arg_fdset);
		buf_copy(target, arg, tok);
		target[tok] = '\0';
		/* perform op on fd with target: */
		if (op == '=') {
			/* duplication: target must be a single character: */
			if (target[1] != '\0')
				fatal_usage("bad duplication target in"
				            " fdset argument ", arg_fdset);
			fdset_duplicate(fd, *target);
		} else {
			/* '<' '>' '+' redirection: */
			fdset_redirect(fd, op, target);
		}

		/* setup for next while loop: */
		arg += tok;
		if (*arg == ':')
			++arg;
	}
}



/*
 * chdir
 */
static void
do_chdir(const char *arg)
{
	if (chdir(arg) < 0)
		fatal_syserr("failure on chdir to ", arg);
}



/*
 * chroot
 */
static void
do_chroot(const char *arg)
{
	if (getuid())
		fatal_usage("must have root privilege to chroot");
	if (chroot(arg) < 0)
		fatal_syserr("failure on chroot to ", arg);
}



/*
 * umask
 */
static void
do_umask(const char *arg)
{
	uint32_t mode;
	const char *z;

	if ((z = nuscan_uint32o(&mode, arg)))
		fatal_usage("bad octal numeric argument for option -m: ", arg);

	umask((mode_t)mode);
}


/*
 * sleep
 */
static void
sig_trap(int sig)
{
	/* catch signal, do nothing: */
	(void)sig;
}

static void
do_sleep(const char *arg)
{
	uint32_t secs;
	const char *z;

	if ((z = nuscan_uint32(&secs, arg)))
		fatal_usage("bad numeric argument for sleep option -z: ", arg);

	/* catch SIGALRM on pause()/sleep() without termination: */
	sig_catch(SIGALRM, sig_trap);

	if (!secs)
		pause();
	else
		sleep(secs);

	sig_uncatch(SIGALRM);
	errno = 0;
}

static void
usage(void)
{
	usage_align_init();
	eputs("usage: ", getprogname(),
	      " [-0 argv0] [<-a | -A> argfile] [-c chdir] [-C chroot]\n", S
	      " [-d] [<-e | -E> envfile] [-F fdset]\n", S
	      " [-L [:]lockfile | -P [:]pidlock] [-m umask]\n", S
	      " [-R <a|c|d|f|m|o|p|r|s|t>=<num|!>[:...]] [-s]\n", S
	      " [-S altpath] [-u user] [-U user] [-z secs] [-W]\n", S
	      " program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	pid_t pid;
	int opt_arginsert, opt_detach, opt_envmerge, opt_setsid;
	char *arg_alias, *arg_argfile, *arg_chdir, *arg_chroot, *arg_env;
	char *arg_envuid, *arg_fdset, *arg_pidlock, *arg_rlimit, *arg_setlock;
	char *arg_setuid, *arg_search, *arg_sleep, *arg_umask, *prog;

	setprogname(argv[0]);

	opt_arginsert = 0;
	opt_detach = 0;
	opt_envmerge = 1;
	opt_setsid = 0;
	prog = NULL;
	arg_alias = NULL;
	arg_argfile = NULL;
	arg_chdir = NULL;
	arg_chroot = NULL;
	arg_env = NULL;
	arg_envuid = NULL;
	arg_fdset = NULL;
	arg_pidlock = NULL;
	arg_rlimit = NULL;
	arg_setlock = NULL;
	arg_setuid = NULL;
	arg_search = NULL;
	arg_sleep = NULL;
	arg_umask = NULL;

	ARGBEGIN {
	case '0':
		arg_alias = EARGF(usage());
		break;
	case 'a':
		opt_arginsert = 0;
		arg_argfile = EARGF(usage());
		break;
	case 'A':
		opt_arginsert = 1;
		arg_argfile = EARGF(usage());
		break;
	case 'c':
		arg_chdir = EARGF(usage());
		break;
	case 'C':
		arg_chroot = EARGF(usage());
		break;
	case 'd':
		opt_detach = 1;
		opt_setsid = 1;
		break;
	case 'e':
		opt_envmerge = 1;
		arg_env = EARGF(usage());
		break;
	case 'E':
		opt_envmerge = 0;
		arg_env = EARGF(usage());
		break;
	case 'F':
		arg_fdset = EARGF(usage());
		break;
	case 'L':
		arg_setlock = EARGF(usage());
		break;
	case 'm':
		arg_umask = EARGF(usage());
		break;
	case 'P':
		arg_pidlock = EARGF(usage());
		break;
	case 'R':
		arg_rlimit = EARGF(usage());
		break;
	case 's':
		opt_setsid = 1;
		break;
	case 'S':
		arg_search = EARGF(usage());
		break;
	case 'u':
		arg_setuid = EARGF(usage());
		break;
	case 'U':
		arg_envuid = EARGF(usage());
		break;
	case 'W':
		warn = 1;
		break;
	case 'z':
		arg_sleep = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		fatal_usage("missing required program argument");

	if (arg_setlock && arg_pidlock)
		fatal_usage("options -L (setlock) and"
		            " -P (pidlock) are mutually exclusive");

	prog = argv[0];
	if (arg_alias)
		argv[0] = arg_alias;

	if (arg_argfile) {
		/* set argv[0]: */
		arglist_add((arg_alias ? arg_alias : prog));
		argv++;

		if (opt_arginsert)
			for (; *argv; argv++)
				arglist_add(*argv);

		do_argfile((opt_arginsert ? "A" : "a"), arg_argfile);

		if (!opt_arginsert)
			for (; *argv; argv++)
				arglist_add(*argv);

		/* append NULL to arglist: */
		arglist_add(NULL);
	}

	if (arg_env)
		do_env((opt_envmerge ? "e" : "E"), arg_env);

	if (arg_envuid)
		do_envuid(arg_envuid);

	if (opt_detach) {
		if ((pid = fork()) < 0)
			fatal_syserr("failure to detach");
		/* parent exits: */
		if (pid)
			_exit(0);
	}

	if (opt_setsid)
		/* start new session group (warn failure): */
		if (setsid() < 0) {
			warning("failure setsid() for option -s: ",
			        sysstr_errno_mesg(errno),
			        " (", sysstr_errno(errno), ")");

	/* setlock after detach so lock held by new pid: */
	if (arg_setlock)
		do_setlock(arg_setlock, 0);
	else if (arg_pidlock)
		do_setlock(arg_pidlock, getpid());

	if (arg_umask)
		do_umask(arg_umask);

	if (arg_fdset)
		do_fdset(arg_fdset);

	if (arg_rlimit)
		do_runlimit(arg_rlimit);

	if (arg_chdir)
		do_chdir(arg_chdir);

	if (arg_chroot)
		do_chroot(arg_chroot);

	if (arg_setuid)
		do_setuid(arg_setuid);

	if (arg_sleep)
		do_sleep(arg_sleep);

	newenv_exec(prog, (arg_argfile ? (char **)dynstuf_STUF(&arglist) :
	            argv), arg_search, (opt_envmerge ? envp : NULL));

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}
