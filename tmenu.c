/* tmenu.c: dmenu on the terminal
 *
 *   (c) 2020-2021, 2023 Duncan McIntosh
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#include "arguments.h"
#include "input.h"
#include "options.h"
#include "redraw.h"
#include "terminal.h"
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static FILE* tty = NULL;
static char** options = NULL;

static void exiting(void);

static void
exiting(void)
{
	tmenu_term_deprep(tty);
	tmenu_options_free(options);
}

int
main(int argc, char** argv)
{
	if (argc > 0)
		progname = argv[0];

	setlocale(LC_ALL, "");

	struct tmenu_arguments args;
	if (tmenu_arguments_parse(argc, argv, &args) < 0)
		return EXIT_FAILURE;

	/* this could be done before the options, but we might as well tell
	 * the user about invalid termcap before they've written their
	 * essay
	 */
	tty = tmenu_term_initialize();

	if (!tty) /* TODO warn */
		return EXIT_FAILURE;

	struct tmenu_redraw_state data;
	data.tty = tty;
	data.suggestions = NULL;
	data.last_input = NULL;
	data.selected_suggestion = &selected_option;
	data.matching_suggestions = 0;

	tmenu_input_initialize(tty, tmenu_redraw, &data);
	tmenu_options_navigation_initialize(&data.matching_suggestions);

	if (!(options = tmenu_options_read(stdin)))
		return EXIT_FAILURE;

	data.suggestions = options;

	atexit(exiting);
	tmenu_term_prep(tty, args.location);

	char* result = tmenu_input_ask();
	tmenu_term_startofline(tty);
	tmenu_term_clearrest(tty, 0);

	if (result)
		fputs(result, stdout);

	fputc('\n', stdout);
	return EXIT_SUCCESS;
}
