/* See LICENSE file for copyright and license details. */
#include <sys/types.h>

#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
usage(void)
{
	eputs("usage: ", getprogname(), " progname [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	pid_t pid;

	setprogname(argv[0]);

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (!argc)
		fatal_usage("missing required program argument");

	if ((pid = fork()) < 0)
		fatal_syserr("failure to detach");

	/* parent exits: */
	if (pid)
		_exit(0);
	setsid();

	/* execvx() provides path search for prog */
	execvx(argv[0], argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}


/* eof: rundetach.c */
