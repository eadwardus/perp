/* See LICENSE file for copyright and license details. */
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
usage(void)
{
	eputs("usage: ", getprogname(), " realprog alias [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	const char *prog;

	setprogname(argv[0]);

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc < 2)
		fatal_usage("missing required argument(s)");

	/* our real executable: */
	prog = argv[0];

	/* ratchet alias into argv[0]: */
	argv++;

	/* execvx() provides path search for prog */
	execvx(prog, argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
