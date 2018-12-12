/* See LICENSE file for copyright and license details. */

/* behavior issues:
 *
 *  ? runtrap hangs in do_trap() if trapper hangs:
 *  -->  user error: failure in configuration or expression of trapper
 *
 *  ? runtrap itself will not exit unless/until do_trap(SIGTERM) causes
 *    child termination:
 *  -->  if thats what child in fact does, behavior is consistent
 *  -->  otherwise: user error as above
 *
 */

#include <sys/wait.h>

#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

#define warn(...) eputs(getprogname(), ": warning: ", __VA_ARGS__)

typedef struct child child_t;

struct child {
	char **argv;
	pid_t pid;
	tain_t when;
};

/* environ: */
extern char **environ;

/* variables in scope: */
static int my_sigpipe[2];
static int flag_term;
static int last_signal;
static sigset_t my_sigset;
static char *my_trap_path;
static char *my_trap_id;
static child_t my_child;

static void
sigpipe_ping(void)
{
	int terrno, w;

	terrno = errno;

	do {
		w = write(my_sigpipe[1], "!", 1);
	} while ((w < 0) && (errno == EINTR));

	errno = terrno;
}


static void
sig_trap(int sig)
{
	/* discard any signals if already got SIGTERM: */
	if (flag_term) {
		/* XXX, emit warning diagnostic */
		sigpipe_ping();
		return;
	}

	switch (sig) {
	case SIGCHLD:
		sig = last_signal;
		break;
	case SIGTERM:
		/* if we catch SIGTERM:
		 *   assume exit is desired and setup to terminate
		 */
		flag_term = 1;
		/* fallthrough: */
	default:
		/* pass signal to trapper: */
		do_trap(sig);
		break;
	}

	last_signal = sig;
	sigpipe_ping();
}


static void
setup(void)
{
	int i;

	if (pipe(my_sigpipe) < 0)
		fatal_syserr("failure setting up selfpipe");

	for (i = 0; i < 2; i++) {
		fd_cloexec(my_sigpipe[i]);
		fd_nonblock(my_sigpipe[i]);
	}

	sigset_fill(&my_sigset);
	sigset_block(&my_sigset);

	sig_catch(SIGTERM, &sig_trap);
	sig_catch(SIGINT, &sig_trap);
	sig_catch(SIGCHLD, &sig_trap);

	/* catching these signals to pass to program: */
	sig_catch(SIGALRM, &sig_trap);
	sig_catch(SIGCONT, &sig_trap);
	sig_catch(SIGHUP, &sig_trap);
	sig_catch(SIGQUIT, &sig_trap);
	sig_catch(SIGTSTP, &sig_trap);
	sig_catch(SIGUSR1, &sig_trap);
	sig_catch(SIGUSR2, &sig_trap);

	sig_ignore(SIGPIPE);
}


static void
do_trap(int signo)
{
	pid_t pid;
	int wstat;
	char *trap_argv[7];
	char fmt_pid[NFMT_SIZE];
	char fmt_signo[NFMT_SIZE];

	/* invariant: */
	trap_argv[0] = my_trap_path;
	trap_argv[1] = "trap";
	trap_argv[2] = my_trap_id;
	/* variant: */
	trap_argv[3] = nfmt_uint32(fmt_pid, my_child.pid);
	trap_argv[4] = nfmt_uint32(fmt_signo, signo);
	trap_argv[5] = (char *)sysstr_signal(signo);
	/* */
	trap_argv[6] = NULL;

	/* discard signal if no child:
	 *   note: the kill() test may be ineffective
	 *   also: race conditions are still possible
	 */
	if (!(my_child.pid) || (kill(my_child.pid, 0) < 0)) {
		warn("child not running, discarding signal ", trap_argv[5]);
		return;
	}

	warn("do_trap() on signal ", trap_argv[5]);

	while ((pid = fork()) < 0) {
		warn("failure on fork() for runtrap ", my_trap_id, ": ",
		     my_trap_path);
		sleep(2);
	}

	/* child: */
	if (!pid) {
		/* reset default signal handlers, unblock: */
		sig_default(SIGTERM);
		sig_default(SIGCHLD);
		sig_default(SIGALRM);
		sig_default(SIGCONT);
		sig_default(SIGHUP);
		sig_default(SIGINT);
		sig_default(SIGQUIT);
		sig_default(SIGTSTP);
		sig_default(SIGUSR1);
		sig_default(SIGUSR2);
		sig_default(SIGPIPE);
		sigset_unblock(&my_sigset);
		/* do it: */
		execvx(trap_argv[0], (char *const *) trap_argv, environ, NULL);
		/* uh oh: */
		fatal_syserr("failure on exec of trap ", trap_argv[0]);
	}

	/* XXX:
	 *   ? possible timeout/failsafe if trapper fails to complete
	 *   ? including SIGTERM directly to child pid if SIGTERM && flag_term
	 */

	/* parent: block for completion of trapper */
	while (waitpid(pid, &wstat, 0) != pid) {
		if (errno != EINTR) {
			warn("failure waitpid() on trapper: ",
			     sysstr_errno(errno));
			break;
		}
	}
	/* XXX, emit wstat diagnostic to stderr? */
	warn("trapper completion");
}


static void
child_exec(void)
{
	pid_t pid;
	tain_t now, when_ok;
	char **argv;

	argv = my_child.argv;

	tain_now(&now);
	tain_load(&when_ok, 1, 0);
	tain_plus(&when_ok, &my_child.when, &when_ok);
	if (tain_less(&now, &when_ok)) {
		warn("pausing for respawn of ", argv[0], " ...");
		sleep(1);
	}

	while ((pid = fork()) < 0) {
		warn("failure on fork() while starting ", argv[0]);
		sleep(2);
	}

	/* child: */
	if (!pid) {
		/* reset default signal handlers, unblock: */
		sig_default(SIGTERM);
		sig_default(SIGCHLD);
		sig_default(SIGALRM);
		sig_default(SIGCONT);
		sig_default(SIGHUP);
		sig_default(SIGINT);
		sig_default(SIGQUIT);
		sig_default(SIGTSTP);
		sig_default(SIGUSR1);
		sig_default(SIGUSR2);
		sig_default(SIGPIPE);
		sigset_unblock(&my_sigset);
		/* do it: */
		execvx(argv[0], argv, environ, NULL);
		/* uh oh: */
		fatal_syserr("failure on exec of ", argv[0]);
	}

	/* parent: */
	my_child.pid = pid;
	tain_now(&my_child.when);
}



static void
child_wait(void)
{
	pid_t pid;
	int wstat;

	while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0)
		if (pid == my_child.pid)
			my_child.pid = 0;
			/* XXX, emit some diagnostic for wstat? */
}


static void
main_loop(void)
{
	struct pollfd pollv[1];
	int e;
	char *c;

	pollv[0].fd = my_sigpipe[0];
	pollv[0].events = POLLIN;

	for (;;) {
		/* terminal condition: */
		if (flag_term && !my_child.pid)
			break;

		/* start/restart child: */
		if (!my_child.pid)
			child_exec();

		/* note:
		 *   if do_trap(SIGTERM) doesn't kill child
		 *   ie: if ((flag_term) && (my_child.pid != 0))
		 *   we hang
		 */

		/* poll on sigpipe: */
		sigset_unblock(&my_sigset);
		do {
			e = poll(pollv, 1, -1);
		} while ((e < 0) && (errno == EINTR));
		sigset_block(&my_sigset);

		/* consume sigpipe: */
		while (read(my_sigpipe[0], &c, 1) == 1)
			continue;

		/* consume dead children: */
		child_wait();
	}
}

static void
usage(void)
{
	eputs("usage: ", getprogname(), " id trapper child [args ...]");
	die(1);
}

int
main(int argc, char *argv[])
{
	setprogname(argv[0]);

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc < 3)
		fatal_usage("missing required arguments");

	/* trapper: */
	my_trap_id = *argv++;
	my_trap_path = *argv++;

	/* child: */
	my_child.argv = argv;
	my_child.pid = 0;
	tain_load(&my_child.when, 0, 0);

	/* initialize sigpipe[], etc: */
	setup();
	/* da bidness: */
	main_loop();

	return 0;
}
