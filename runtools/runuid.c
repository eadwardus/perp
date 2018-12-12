/* See LICENSE file for copyright and license details. */
#include <sys/types.h>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "lasagna.h"
#include "runtools.h"

/* XXX, no relation to sysconf values: */
#define MY_NGROUPS 32
#define MY_GRPBUF  255

static void
do_grplist(gid_t gid, const char *grplist)
{
	struct group *gr;
	gid_t gidset[MY_NGROUPS];
	int n, n_max;
	int k, k_max;
	char grpbuf[MY_GRPBUF];
	char c;
	const char *s;

	gr = NULL;
	k = 0;
	k_max = MY_GRPBUF;
	n = 0;
	n_max = MY_NGROUPS;
	s = grplist;

	/* base gid entered as first group in gidset: */
	gidset[n++] = gid;

	/* parse colon-delimited grplist: */
	for (;;) {
		c = *s;
		if ((c == ':') || (c == '\0')) {
			grpbuf[k] = '\0';
			if (!grpbuf[0])
				fatal(100,
				      "empty group name found in group list ",
				      grplist);

			if (!(gr = getgrnam(grpbuf)))
				fatal(111, "no group found for \"", grpbuf,
				      "\" in grplist ", grplist);

			if (n > n_max)
				fatal(111,
				      "too many group names in group list ",
				      grplist);

			gidset[n++] = gr->gr_gid;
			k = 0;

			/* loop terminal: */
			if (!c)
				break;
		} else {
			grpbuf[k++] = c;
			if (k > k_max)
				fatal(111,
				      "excessive length group name found in ",
				      grplist);
		}
		s++;
	}

	/* set: */
	if (setgroups(n, gidset) < 0)
		fatal_syserr(111, "failure setgroups() for group list ",
		             grplist);
}

static void
usage(void)
{
	eputs("usage: ", getprogname(),
	      "[-g grp] [-s | -S grplist] account program [args ...]");
	die(1);
}

int
main(int argc, char *argv[], char *envp[])
{
	struct passwd *pw;
	struct group *gr;
	gid_t gid;
	uid_t uid;
	int opt_grplist, opt_grpsupp;
	const char *arg_grp, *arg_grplist;
	const char *prog, *user_acct;

	setprogname(argv[0]);

	opt_grplist = 0;
	opt_grpsupp = 0;

	ARGBEGIN {
	case 'g':
		arg_grp = EARGF(usage());
		break;
	case 'S':
		opt_grplist = 1;
		arg_grplist = EARGF(usage());
		break;
	case 's':
		opt_grpsupp = 1;
		break;
	default:
		usage();
	} ARGEND

	if (getuid())
		fatal_usage("not running as super user");

	if (argc < 2)
		fatal_usage("missing required argument(s)");

	if (opt_grpsupp && opt_grplist)
		fatal_usage("options -s and -S <grplist> are mutually exclusive");

	user_acct = *argv++;
	prog = *argv;

	if (!(pw = getpwnam(user_acct)))
		fatal(111, "no account found for user ", user_acct);

	gid = pw->pw_gid;
	uid = pw->pw_uid;

	/* alternative base group: */
	if (arg_grp) {
		if (!(gr = getgrnam(arg_grp)))
			fatal(111, "no group found for group ", arg_grp);
		gid = gr->gr_gid;
	}

	if (opt_grplist)
		/* supplemental groups given on command-line: */
		do_grplist(gid, arg_grplist);
	else if (opt_grpsupp)
		/* supplemental groups from /etc/group with initgroups(3): */
		/* XXX, note: initgroups(3) may malloc() */
		if (initgroups(user_acct, gid) < 0)
			fatal_syserr("failure initgroups()");
	else
		if (setgroups(1, &gid) < 0)
			fatal_syserr("failure setgroups()");

	if (setgid(gid) < 0)
		fatal_syserr("failure setgid()");

	if (setuid(uid) < 0)
		fatal_syserr("failure setuid()");

	/* execvx() provides path search for prog */
	execvx(prog, argv, envp, NULL);

	/* uh oh: */
	fatal_syserr("unable to run ", prog);

	/* not reached: */
	return 0;
}
