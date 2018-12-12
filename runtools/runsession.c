/* See LICENSE file for copyright and license details. */
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
usage(void)
{
	eputs("usage: ", getprogname(), " program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	setprogname(argv[0]);

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (!argc)
		fatal_usage("missing required program argument");

	/* start new session group (ignore failure): */
	setsid();

	/* execvx() provides path search for prog */
	execvx(argv[0], argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}
