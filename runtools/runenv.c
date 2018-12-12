/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

static void
val_unescape(char *s)
{
	char *c, *new;

	c   = s;
	new = s;

	for (; *c; c++) {
		if (*c != '\\') {
			*new = *c;
			continue;
		}
		c++;
		switch (*c) {
		case '\\':
			*new++ = '\\';
			break;
		case 'n':
			*new++ = '\n';
			break;
		case 't':
			*new++ = '\t':
			break;
			/* "protected" space (use to preserve from cstr_trim()): */
		case '_':
			*new++ = ' ';
			break;
			/* if string ends with single backslash: */
		case '\0':
			*new++ = '\\';
			continue;
		default:
			/* escape sequence not defined, leave verbatim: */
			*new++ = '\\';
			*new++ = *c;
			break;
		}
	}

	*new = '\0';
}


static void
do_envfile(const char *envfile)
{
	dynstr_t L;
	ioq_t q;
	size_t split;
	int eof, fd, r;
	char *line, *key, *val;
	uchar_t qbuf[IOQ_BUFSIZE];

	if (!cstr_cmp(envfile, "-")) {
		fd = 0;
		envfile = "<stdin>";
	} else {
		if ((fd = open(envfile, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open ", envfile);
	}

	ioq_init(&q, fd, qbuf, sizeof qbuf, &read);

	eof = 0;
	L = dynstr_INIT();

	while (!eof) {
		/* recycle any allocated dynstr: */
		dynstr_CLEAR(&L);

		/* fetch next line: */
		if ((r = ioq_getln(&q, &L)) < 0)
			fatal_syserr("error reading ", envfile);

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

		/* parse line into key, value: */
		key = line;
		split = cstr_pos(key, '=');
		if (key[split] == '=') {
			val = &line[split + 1];
			key[split] = '\0';
			/* trim whitespace around '=': */
			cstr_rtrim(key);
			cstr_ltrim(val);
			/* process escape sequences in val: */
			val_unescape(val);
		} else {
			/* no value sets up delete of existing variable: */
			val = NULL;
		}

		/* skip empty key: */
		if (key[0] == '\0')
			continue;

		/* add new environment variable
		 ** (null value sets up delete of existing variable)
		 */
		if (newenv_set(key, val) < 0)
			fatal_syserr("failure allocating new environment");
	}

	/* success: */
	if (fd)
		close(fd);
}


static void
do_envdir(const char *envdir)
{
	dynstr_t L;
	DIR *dir;
	ioq_t q;
	struct dirent *d;
	int err, fd, fd_orig;
	char *line;
	uchar_t qbuf[IOQ_BUFSIZE];

	if ((fd_orig = open(".", O_RDONLY | O_NONBLOCK)) < 0)
		fatal_syserr("unable to open current directory");

	if ((err = chdir(envdir)) < 0)
		fatal_syserr("unable to chdir to ", envdir);

	if (!(dir = opendir(".")))
		fatal_syserr("unable to open directory ", envdir);

	L = dynstr_INIT();

	for (;;) {
		errno = 0;
		if (!(d = readdir(dir))) {
			if (errno)
				fatal_syserr("unable to read directory ",
				             envdir);
			/* else all done: */
			break;
		}

		/* skip any dot files: */
		if (d->d_name[0] == '.')
			continue;

		((fd = open(d->d_name, O_RDONLY | O_NONBLOCK)) < 0)
			fatal_syserr("unable to open ", envdir, "/",
			             d->d_name);

		/* prepare ioq buffer and recycle line buffer: */
		ioq_init(&q, fd, qbuf, sizeof qbuf, &read);
		dynstr_CLEAR(&L);

		/* one line read: */
		if ((err = ioq_getln(&q, &L)) < 0)
			fatal_syserr("unable to read ", envdir, "/",
			             d->d_name);
		close(fd);

		/* work directly on line buffer: */
		if ((line = dynstr_STR(&L))) {
			cstr_trim(line);
			val_unescape(line);
		}

		/* add new environmental variable
		 ** (null value sets up delete of existing variable)
		 */
		err = newenv_set(d->d_name, ((line && line[0]) ? line : NULL));
		if (err < 0)
			fatal_syserr("failure allocating new environment");
	}

	closedir(dir);
	if (fchdir(fd_orig) < 0)
		fatal_syserr("failure changing to original directory");
	close(fd_orig);
}


static void
do_envuid(const char *account)
{
	struct passwd *pw;
	int err;
	char nfmt[NFMT_SIZE];

	if (!(pw = getpwnam(account)))
		fatal(111, "no account for user ", account);

	if ((err = newenv_set("GID", nfmt_uint32(nfmt, (uint32_t) pw->pw_gid))))
		fatal_syserr("failure allocating new environment");

	if ((err = newenv_set("UID", nfmt_uint32(nfmt, (uint32_t) pw->pw_uid))))
		fatal_syserr("failure allocating new environment");
}

static void
usage(void)
{
	eputs("usage: ", getprogname(),
	      "[-i] [-U account] newenv program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	struct stat sb;
	int opt_merge;
	const char *arg_envuid, *newenv;

	setprogname(argv[0]);

	opt_merge = 1;

	ARGBEGIN {
	case 'i':
		opt_merge = 0;
		break;
	case 'U':
		arg_envuid = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		fatal_usage("missing required argument(s)");

	newenv = *argv++;

	/* newenv may be file or directory: */
	if (!cstr_cmp(newenv, "-")) {
		do_envfile(newenv);
	} else {
		if (stat(newenv, &sb) < 0)
			fatal_syserr("unable to stat ", newenv);
		if (S_ISREG(sb.st_mode))
			do_envfile(newenv);
		else if (S_ISDIR(sb.st_mode))
			do_envdir(newenv);
		else
			fatal_usage("argument is not a file or directory: ",
			            newenv);
	}

	if (arg_envuid)
		do_envuid(arg_envuid);

	/* execvx() provides shell-like path search for argv[0] */
	newenv_run(argv, (opt_merge ? envp : NULL));

	/* uh oh: */
	fatal_syserr("unable to run ", argv[0]);

	/* not reached: */
	return 0;
}
