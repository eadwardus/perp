/* See LICENSE file for copyright and license details. */
#include <sys/wait.h>

#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

#define warn(...) eputs(progname, ": warning: ", __VA_ARGS__)
#define do_kill(p,k) if ((p) > 0) kill((p),(k))

struct child {
	char **argv;
	pid_t pid;
	tain_t when;
};

/* environ: */
extern char **environ;

/* variables in scope: */
static struct child deux[2];
static int my_sigpipe[2];
static int my_logpipe[2];
static int flag_term;
static sigset_t my_sigset;
/* running as supervisor by default: */
static int opt_super = 1;

static void
sigtrap(int sig)
{
	int want_cont;

	warn_cont = 0;

	switch (sig) {
	case SIGCHLD:
		break;
	case SIGINT:
		/* want clean term with SIGINT! */
		/* fall through: */
	case SIGTERM:
		flag_term = 1;
		want_cont = 1;
		/* fallthrough: */
	default:
		/* by default, pass signals to program: */
		do_kill(deux[1].pid, (flag_term ? SIGTERM : sig));
		if (want_cont)
			do_kill(deux[1].pid, SIGCONT);
		break;
	}

	sigpipe_ping();
}


static void
sigpipe_ping(void)
{
	int terrno;
	int w;

	terrno = errno;

	do {
		w = write(my_sigpipe[1], "!", 1);
	} while ((w < 0) && (errno == EINTR));

	errno = terrno;
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

	if (pipe(my_logpipe) < 0)
		fatal_syserr("failure creating logpipe");

	sigset_fill(&my_sigset);
	sigset_block(&my_sigset);

	sig_catch(SIGTERM, &sigtrap);
	sig_catch(SIGINT, &sigtrap);
	sig_catch(SIGCHLD, &sigtrap);

	/* catching these signals to pass to program: */
	sig_catch(SIGALRM, &sigtrap);
	sig_catch(SIGCONT, &sigtrap);
	sig_catch(SIGHUP, &sigtrap);
	sig_catch(SIGQUIT, &sigtrap);
	sig_catch(SIGTSTP, &sigtrap);
	sig_catch(SIGUSR1, &sigtrap);
	sig_catch(SIGUSR2, &sigtrap);

	sig_ignore(SIGPIPE);
}


static void
deux_init(void)
{
	/* deux[0]: logger: */
	deux[0].argv = NULL;
	deux[0].pid = 0;
	tain_load(&deux[0].when, 0, 0);

	/* deux[1]: program: */
	deux[1].argv = NULL;
	deux[1].pid = 0;
	tain_load(&deux[1].when, 0, 0);
}


static void
child_exec(int which)
{
	pid_t pid;
	tain_t now;
	tain_t when_ok;
	char **argv;

	argv = deux[which].argv;

	tain_now(&now);
	tain_load(&when_ok, 1, 0);
	tain_plus(&when_ok, &deux[which].when, &when_ok);
	if (tain_less(&now, &when_ok)) {
		warn("pausing for respawn of ", argv[0], " ...");
		sleep(1);
	}

	/*  if option -x:
	 *    not supervising, don't fork here, just exec into deux[1]
	 */
	pid = 0;
	if (opt_super || !which) {
		while ((pid = fork()) < 0) {
			warn("failure on fork() while starting ", argv[0]);
			sleep(2);
		}
	}

	/* child (or, if not opt_super, execing into deux[1]): */
	if (!pid) {
		/* setup logpipe: */
		fd_dupe(which, my_logpipe[which]);
		close(my_logpipe[0]);
		close(my_logpipe[1]);
#if 0
		if (which == 1) {
			/* redirect stderr to stdout for program: */
			fd_dupe(2, 1);
		}
#endif
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
	deux[which].pid = pid;
	tain_now(&deux[which].when);
}



static void
child_wait(void)
{
	pid_t pid;
	int i;
	int wstat;

	while ((pid = waitpid(-1, &wstat, WNOHANG)) > 0)
		for (i = 0; i < 2; ++i)
			if (pid == deux[i].pid)
				deux[i].pid = 0;
}


static void
main_loop(void)
{
	struct pollfd pfd[1];
	int e, i;
	char *c;

	pfd[0].fd = my_sigpipe[0];
	pfd[0].events = POLLIN;

	for (;;) {
		if (flag_term)
			break;

		/* start/restart children: */
		for (i = 0; i < 2; i++) {
			if (!deux[i].pid)
				child_exec(i);

		/* poll on sigpipe: */
		sigset_unblock(&my_sigset);
		do {
			e = poll(pfd, 1, -1);
		} while ((e < 0) && (errno == EINTR));
		sigset_block(&my_sigset);

		/* consume sigpipe: */
		while (read(my_sigpipe[0], &c, 1) == 1)
			continue;

		/* consume dead children: */
		child_wait();
	}

	if (deux[0].pid) {
		/* logger should exit on eof: */
		close(my_logpipe[1]);
		do_kill(deux[0].pid, SIGCONT);
		waitpid(deux[0].pid, NULL, 0);
	}
}

static void
usage(void)
{
	eputs("usage: ", getprogname(),
	      "[-d] [-L label] [-S sep] [-x] program <\"::\"|\"sep\"> logger");
	die(1);
}

int
main(int argc, char *argv[])
{
	pid_t pid;
	int opt_detach;
	char *arg_sep;

	setprogname(argv[0]);

	arg_sep = "::";
	opt_detach = 0;

	ARGBEGIN {
	case 'd':
		opt_detach++;
		break;
	case 'L':
		break;
	case 'S':
		arg_sep = EARGF(usage());
		break;
	case 'x':
		opt_super = 0;
		break;
	default:
		usage();
	} ARGEND

	if (argc < 3)
		fatal_usage("missing required arguments");

	deux_init();

	/* proggy: */
	deux[1].argv = argv++;
	for (; argv; argv++) {
		if (cstr_cmp(*argv, arg_sep)) {
			deux[0].argv = &argv[1];
			*argv = NULL;
			break;
		}
	}

	if (!deux[0].argv)
		fatal_usage("missing required logger argument");

	if (opt_detach) {
		switch ((pid = fork())) {
		case -1:
			fatal_syserr("unable to detach");
			break;
		case 0:
			/* child continues daemonized in new session: */
			setsid();
			break;
		default:
			/* parent exits: */
			exit(0);
		}
	}

	/* initialize sigpipe[], etc: */
	setup();
	main_loop();

	return 0;
}
