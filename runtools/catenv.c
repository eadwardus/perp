/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
usage(void)
{
	eputs("usage: ", getprogname());
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

	for (; *envp; envp++)
		ioq_vputs(ioq1, *envp, "\n");

	ioq_flush(ioq1);

	return 0;
}
