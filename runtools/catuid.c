/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

#define report(k,v) eputs((k), ":\t", (v))

/* getsid() not posix: */
extern pid_t getsid(pid_t pid);

static void
usage(void)
{
	eputs("usage: ", getprogname());
	die(1);
}

int
main(int argc, char *argv[])
{
	char nfmt[NFMT_SIZE];

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	report("UID",  nfmt_uint32(nfmt, (uint32_t)getuid()));
	report("EUID", nfmt_uint32(nfmt, (uint32_t)geteuid()));
	report("GID",  nfmt_uint32(nfmt, (uint32_t)getgid()));
	report("EGID", nfmt_uint32(nfmt, (uint32_t)getegid()));
	report("PID",  nfmt_uint32(nfmt, (uint32_t)getpid()));
	report("PPID", nfmt_uint32(nfmt, (uint32_t)getppid()));
	report("PGID", nfmt_uint32(nfmt, (uint32_t)getpgrp()));
	report("SID",  nfmt_uint32(nfmt, (uint32_t)getsid(0)));
	report("umask", nfmt_uint32o_pad0(nfmt, (uint32_t)umask(0), 4));

	return 0;
}
