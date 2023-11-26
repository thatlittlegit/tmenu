/* arguments-getopt.c: arguments-parsing with getopt
 *
 *   (c) 2021, 2023 Duncan McIntosh
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#include "arguments.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char* progname = "tmenu";
const char* const TMENU_USAGE_TEXT = _TMENU_USAGE_TEXT;

int
tmenu_arguments_parse(int argc, char** argv, struct tmenu_arguments* args)
{
	args->location = TMENU_LOCATION_DEFAULT;

	int val;

	while ((val = getopt(argc, argv, "tbT")) > 0) {
		if (val == 't')
			args->location = TMENU_LOCATION_TOP;

		if (val == 'b')
			args->location = TMENU_LOCATION_BOTTOM;

		if (val == 'T')
			args->location = TMENU_LOCATION_DEFAULT;
	}

	if (optind != argc) {
		fprintf(stderr, TMENU_USAGE_TEXT, progname);
		return -1;
	}

	return 0;
}
