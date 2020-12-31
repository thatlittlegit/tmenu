/* tmenu.c: dmenu on the terminal
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "input.h"
#include "options.h"
#include "redraw.h"
#include "terminal.h"
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* tty = NULL;
static char** options = NULL;
static size_t options_current_count = 0;
static size_t selected_suggestion = 0;

static const char* progname = "tmenu";

int back_suggestion(int, int);
int forward_suggestion(int, int);
static void exiting(void);
static void resize_signal_handler(int signum);
static void setup_resize_handler(void);

int
back_suggestion(int count, int key)
{
	(void)key;

	if (selected_suggestion > 0)
		selected_suggestion -= (count ? count : 1);

	return 0;
}

int
forward_suggestion(int count, int key)
{
	(void)key;

	if (selected_suggestion < options_current_count - 1)
		selected_suggestion += (count ? count : 1);

	return 0;
}

static void
exiting(void)
{
	tmenu_term_deprep(tty);
	tmenu_options_free(options);
}

static void
resize_signal_handler(int signum)
{
	(void)signum;
	tmenu_input_redraw();
}

static void
setup_resize_handler(void)
{
	struct sigaction action;
	action.sa_handler = resize_signal_handler;
	sigaction(SIGWINCH, &action, NULL);
}

int
main(int argc, char** argv)
{
	if (argc > 0)
		progname = argv[0];

	setlocale(LC_ALL, "");

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
	data.selected_suggestion = &selected_suggestion;
	data.matching_suggestions = (int*)&options_current_count;

	tmenu_input_initialize(tty, tmenu_redraw, &data);

	if (!(options = tmenu_options_read(stdin)))
		return EXIT_FAILURE;

	data.suggestions = options;

	atexit(exiting);
	setup_resize_handler();
	tmenu_term_prep(tty);

	char* result = tmenu_input_ask();
	tmenu_term_startofline(tty);
	tmenu_term_clearrest(tty, 0);

	if (result)
		fputs(result, stdout);

	fputc('\n', stdout);
	return EXIT_SUCCESS;
}
