/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
sig_trap(int sig)
{
	(void)sig;
}

static void
usage(void)
{
	eputs("usage: ", getprogname(), " [-L label] secs program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	uint32_t secs;
	const char *z;

	setprogname(argv[0]);

	ARGBEGIN {
	case 'L':
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		fatal_usage("missing required argument(s)");

	if ((z = nuscan_uint32(&secs, argv[0])))
		fatal_usage("bad numeric argument for secs: ", argv[0]);

	argv++;

	/* catch SIGALRM on pause()/sleep() without termination: */
	sig_catch(SIGALRM, sig_trap);

	if (!secs)
		pause();
	else
		sleep(secs);

	sig_uncatch(SIGALRM);
	errno = 0;

	/* execvx() provides path search for prog */
	execvx(argv[0], argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}
