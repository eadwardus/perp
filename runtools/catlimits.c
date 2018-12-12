/* See LICENSE file for copyright and license details. */
#include <sys/resource.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static const char *resources[] = {
	"RLIMIT_AS",
	"RLIMIT_CORE",
	"RLIMIT_CPU",
	"RLIMIT_DATA",
	"RLIMIT_FSIZE",
	"RLIMIT_LOCKS",
	"RLIMIT_MEMLOCK",
	"RLIMIT_NOFILE",
	"RLIMIT_NPROC",
	"RLIMIT_RSS",
	"RLIMIT_SBSIZE",
	"RLIMIT_STACK",
	NULL
};

static void
usage(void)
{
	eputs("usage: ", getprogname());
	die(1);
}

int
main(int argc, char *argv[])
{
	struct rlimit rlim;
	int i, r;
	char nfmt[NFMT_SIZE];
	char *resource;

	setprogname(argv[0]);

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	for (i = 0; resources[i]; i++) {
		resource = resources[i];
		if ((r = rlimit_lookup(resource)) < 0) {
			ioq_vputs(ioq1, resource,
			          "\t[not provided on this platform]\n");
			continue;
		}
		getrlimit(r, &rlim);
		ioq_vputs(ioq1, resource, "\t", rlimit_mesg(r), ": ");
		if (rlim.rlim_cur == RLIM_INFINITY)
			ioq_vputs(ioq1, "unlimited\n");
		else
			ioq_vputs(ioq1, nfmt_uint32(nfmt, rlim.rlim_cur), "\n");
	}

	ioq_flush(ioq1);

	return 0;
}
