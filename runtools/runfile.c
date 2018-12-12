/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

#define fatal_alloc() fatal(111, "allocation failure");

/* arglist in scope: */
static dynstuf_t arglist = dynstuf_INIT();

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
do_argvfile(const char *argvfile)
{
	dynstr_t L;
	ioq_t q;
	int eof, fd, r;
	char *line;
	uchar_t qbuf[IOQ_BUFSIZE];

	fd = 0;

	if (cstr_cmp(argvfile, "<stdin>"))
		if ((fd = open(argvfile, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open ", argvfile);

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	eof = 0;
	L = dynstr_INIT();

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading ", argvfile);

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
	if (fd != 0)
		close(fd);
}

static void
usage(void)
{
	eputs("usage: ", getprogname(), " argvfile [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	const char *argvfile, *prog;

	if (argc)
		fatal_usage("missing required filename");

	argvfile = *argv++;

	if (!cstr_cmp(argvfile, "-"))
		argvfile = "<stdin>";

	/* process argvfile: */
	do_argvfile(argvfile);

	/* require argv[0] in argvfile: */
	if (!(prog = (const char *)dynstuf_get(&arglist, 0)))
		fatal_usage("empty argvfile: no argv[0] element found in ",
		            argvfile);

	/* append any additional command line arguments: */
	for (; *argv; argv++)
		arglist_add(*argv);

	/* append NULL terminal to arglist: */
	arglist_add(NULL);

	/* execvx() provides shell-like path search for prog */
	execvx(prog, (char **)dynstuf_STUF(&arglist), envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
