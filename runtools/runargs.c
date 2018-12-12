/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

#define fatal_alloc() fatal(111, "allocation failure");

/* arglist in scope: */
static dynstuf_t arglist = dynstuf_INIT();

static void
usage(void)
{
	eputs("usage: ", getprogname(), " [-i] argfile progname [args ...]");
	die(1);
}

static void
arglist_add(const char *arg)
{
	char *newarg;

	/* NULL arg acceptable to terminate arglist: */
	newarg = NULL;

	if (arg)
		if (!(newarg = cstr_dup(arg)))
			fatal_alloc();

	if (dynstuf_push(&arglist, newarg) < 0)
		fatal_alloc();
}


static void
do_argfile(const char *argfile)
{
	dynstr_t L;
	ioq_t q;
	int eof, fd, r;
	char *line;
	uchar_t qbuf[IOQ_BUFSIZE];

	if (!cstr_cmp(argfile, "-")) {
		fd = 0;
		argfile = "<stdin>";
	} else {
		if ((fd = open(argfile, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open", argfile);
	}

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	eof  = 0;
	L    = dynstr_INIT();

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading ", argfile);

		if (!r) {
			/* set terminal condition: */
			eof++;
			if (!(dynstr_STR(&L)) || !(dynstr_LEN(&L)))
				break;

			/* else:
			 * eof was encountered after partial line read
			 * (last line not terminated with '\n')
			 * proceed through the end of this loop
			 */
		}

		/* work directly on string buffer: */
		line = dynstr_STR(&L);
		cstr_trim(line);

		/* skip empty lines and comments: */
		if ((line[0] == '\0') || (line[0] == '#'))
			continue;

		/* add this argument: */
		arglist_add(line);
	}

	/* success: */
	if (fd)
		close(fd);
}


int
main(int argc, char *argv[], char *envp[])
{
	int opt_insert;
	const char *argfile, *prog;

	setprogname(argv[0]);

	opt_insert = 0;

	ARGBEGIN {
	case 'i':
		opt_insert = 1;
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		fatal_usage("missing required argument(s)");

	argfile = *argv++;
	prog    = *argv++;

	/* set argv[0] of new arglist to prog: */
	arglist_add(prog);

	/*
	 * "opt_insert"  :  prog args before argfile args
	 * "!opt_insert" :  argfile args before prog args
	 *
	 * "!opt_insert" (opt_insert == 0) is the default
	 */
	if (opt_insert)
		for (; *argv; argv++)
			arglist_add(*argv);

	/* process argfile: */
	do_argfile(argfile);

	if (!opt_insert)
		for (; *argv; argv++)
			arglist_add(*argv);

	/* append NULL terminal to arglist: */
	arglist_add(NULL);

	/* execvx() provides shell-like path search for prog */
	execvx(prog, (char **) dynstuf_STUF(&arglist), envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
