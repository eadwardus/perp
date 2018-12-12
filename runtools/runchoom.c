/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

/* default definitions: */
#ifndef CHOOM_BASE_DEFAULT
#define CHOOM_BASE_DEFAULT  "/proc"
#endif

#ifndef CHOOM_KEY_DEFAULT
#define CHOOM_KEY_DEFAULT  "oom_adj"
#endif

/* see proc(5) manual (linux-specific): */
#ifndef CHOOM_SET_DEFAULT
#define CHOOM_SET_DEFAULT  "-17"
#endif

/* options/optargs: */
static int opt_e; /* fail on error */
static int opt_v; /* verbose */
static char *arg_base;
static char *arg_key;
static char *arg_set;

static char pathbuf[256];
static char setbuf[256];
static char pidfmt[NFMT_SIZE];

/* syserr_warn() macro: */
#define syserr_warn(...) \
{\
eputs(progname, ": ", __VA_ARGS__, ": ", \
      sysstr_errno_mesg(errno), " (",    \
      sysstr_errno(errno), ")");         \
}

/*
 * definitions:
 */

static
int write_all(int fd, void *buf, size_t to_write)
{
	ssize_t w;

	while (to_write) {
		do {
			w = write(fd, buf, to_write);
		} while ((w < 0) && (errno == EINTR));

		if (w < 0)
			return -1; /* error! */

		buf += w;
		to_write -= w;
	}

	/* return:
	 *    = -1, error (above)
	 *    =  0, success (here)
	 */

	return 0;
}


/* do_choom()
 *   if opt_e: abort on fail
 *   no chdir(), no effect to cwd
 */
static
void do_choom(void)
{
	int fd;

	/* comment:
	 * normally we would first create/write a tmpfile
	 * then atomically rename() tmpfile into procfile
	 * but open(...,O_CREAT,...) of a new tmpfile on /proc fails (EEXIST)
	 * leaving no choice but to write into the procfile directly
	 */

	if ((fd = open(pathbuf, O_WRONLY | O_TRUNC, 0)) == -1) {
		if (opt_e)
			fatal_syserr("failure on open() for path: ",
			             pathbuf);
		/* else: */
		if (opt_v)
			syserr_warn("ignoring failure on open() for path: ",
			            pathbuf);
		return;
	}
	fd_cloexec(fd);

	if (write_all(fd, setbuf, cstr_len(setbuf)) == -1) {
		if (opt_e) {
			fatal_syserr("failure on write() to path: ",
			             pathbuf);
		}
		/* else: */
		if (opt_v) {
			syserr_warn("ignoring failure on write() to path: ",
			            pathbuf);
		}
		return;
	}

	/* comment:
	 * fsync() on /proc fails (EINVAL)
	 * so we simply ignore any following errors
	 */
	fsync(fd);
	close(fd);

	/* success: */
	if (opt_v)
		eputs(progname, ": successfully configured ", pathbuf);
}

static void
usage(void)
{
	eputs("usage: ", getprogname(),
	      " [-ev] [-b base] [-k key] [-p pid] [-s str] program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	size_t n;
	uint32_t pid;
	const char *prog, *z;

	setprogname(argv[0]);

	pid = 0;

	ARGBEGIN {
	case 'b':
		arg_base = EARGF(usage());
		break;
	case 'e':
		opt_e++;
		break;
	case 'k':
		arg_key = EARGF(usage());
		break;
	case 'p':
		if ((z = nuscan_uint32(&pid, EARGF(usage))))
			fatal_usage("bad numeric argument for -",
			            ARGC(), ": ", EARGF(usage));
		break;
	case 's':
		arg_set = EARGF(usage());
		break;
	case 'v':
		opt_v++;
		break;
	default:
		usage();
	} ARGEND

	if (argc)
		prog = argv[0];

	if (prog)
		fatal_usage("missing required program argument");

	if (!arg_base)
		arg_base = getenv("CHOOM_BASE");
	if (!arg_base)
		arg_base = CHOOM_BASE_DEFAULT;

	if (arg_base[0] != '/')
		fatal_usage("path base is not absolute (must begin with `/'): ",
		            arg_base);

	if (!arg_key)
		arg_key = getenv("CHOOM_KEY");
	if (!arg_key)
		arg_key = CHOOM_KEY_DEFAULT;

	if (!arg_set)
		arg_set = getenv("CHOOM_SET");
	if (!arg_set)
		arg_set = CHOOM_SET_DEFAULT;

	nfmt_uint32(pidfmt, (pid == 0) ? (uint32_t) getpid() : pid);

	n = cstr_vlen(arg_base, "/", pidfmt, "/", arg_key);
	if (sizeof(pathbuf) - n < 5) {
		errno = ENAMETOOLONG;
		fatal_syserr("path too long: ", arg_base, "/", pidfmt, "/",
		             arg_key);
	}
	cstr_vcopy(pathbuf, arg_base, "/", pidfmt, "/", arg_key);

	n = cstr_vlen(arg_set, "\n");
	if (sizeof(setbuf) - n < 1)
		fatal_usage("setting too long: ", arg_set);
	cstr_vcopy(setbuf, arg_set, "\n");

	do_choom();

	execvx(prog, argv, envp, NULL);
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
