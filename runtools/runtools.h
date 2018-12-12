/* See LICENSE file for copyright and license details. */
#include <errno.h>

/* release version string: */
#ifndef RUNTOOLS_VERSION
#define RUNTOOLS_VERSION "2.07"
#endif

/* stderr: */
#define eputs(...)                   \
{                                    \
 ioq_vputs(ioq2, __VA_ARGS__, "\n"); \
 ioq_flush(ioq2);                    \
}

#define fatal(e, ...)                       \
{                                           \
 eputs(progname, ": fatal: ", __VA_ARGS__); \
 die((e));                                  \
}

#define fatal_syserr(...) \
fatal(111, __VA_ARGS__, ": ", \
      sysstr_errno_mesg(errno), " (", sysstr_errno(errno), ")" )

#define fatal_usage(...)                          \
{                                                 \
 eputs(progname, ": usage error: ", __VA_ARGS__); \
 usage();                                         \
}
