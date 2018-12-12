/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
do_check(const char *lockfile, int quiet)
{
	pid_t lockpid;
	char nfmt[NFMT_SIZE];

	switch ((lockpid = pidlock_check(lockfile))) {
	case -1:
		if (quiet)
			die(111);
		fatal_syserr("failure checking lock on ", lockfile);
	case  0:
		if (quiet)
			die(0);
		eputs(progname, ": no lock held on ", lockfile);
		die(0);
	default:
		if (quiet)
			die(1);
		nfmt_uint32(nfmt, (uint32_t)lockpid);
		eputs(progname, ": lock held on ", lockfile, " by pid ", nfmt);
		die(1);
	}
	/* not reached */
}

static void
usage(void)
{
	eputs("usage: ", getprogname(),
	      " -c [-q] lockfile\n",
	      "       ", getprogname(),
	      " [-bpqx] lockfile program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	int fd, opt_block, opt_check, opt_checkquiet, opt_err, opt_pid;
	const char *lockfile;
	const char *prog;

	setprogname(argv[0]);

	opt_block = 0;
	opt_err = 1;
	opt_pid = 0;
	opt_check = 0;
	opt_checkquiet = 0;

	ARGBEGIN {
	case 'b':
		opt_block = 1;
		break;
	case 'c':
		opt_check = 1;
		break;
	case 'p':
		opt_pid = 1;
		break;
	case 'q':
		opt_checkquiet = 1;
		break;
	case 'x':
		opt_err = 0;
		break;
	default:
		usage();
	} ARGEND

	if (argc)
		lockfile = argv[0];
	if (argc > 1)
		prog = argv[1];

	if (opt_check) {
		if (!lockfile)
			fatal_usage("missing lockfile argument");
		do_check(lockfile, opt_checkquiet);
		/* not reached */
	}

	if (!prog)
		fatal_usage("missing required argument(s)");

	argv++;

	fd = pidlock_set(lockfile, (opt_pid ? getpid() : 0),
	                 (opt_block ? PIDLOCK_WAIT : PIDLOCK_NOW));

	if (fd < 0) {
		if (!opt_err)
			die(0);
		/* else: */
		fatal_syserr("unable to acquire lock on ", lockfile);
	}

	execvx(prog, argv, envp, NULL);
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
