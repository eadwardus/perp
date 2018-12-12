/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <unistd.h>

#include "lasagna.h"

int
main(int argc, char *argv[])
{
	for (; *argv; argv++)
		ioq_vputs(ioq1, *argv, "\n");

	ioq_flush(ioq1);

	return 0;
}
