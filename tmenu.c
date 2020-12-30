/* tmenu.c: dmenu on the terminal
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "input.h"
#include "options.h"
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
static char* last_input = NULL;

static const char* progname = "tmenu";

int back_suggestion(int, int);
int forward_suggestion(int, int);
static void exiting(void);
static void redraw();
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

/* we can't use redraw(void) because we use it as a signal handler */
/* and thus it needs to be able to take an int argument */
static void
redraw(struct tmenu_input_state state, void* data)
{
	(void)data;

	if (!last_input || strcmp(last_input, state.buffer) != 0) {
		selected_suggestion = 0;
		free(last_input);

		if (state.end == 0) {
			last_input = NULL;
		} else {
			last_input = malloc(state.end + 1);
			strncpy(last_input, state.buffer, state.end + 1);
			last_input[state.end] = 0;
		}
	}

	size_t width = tmenu_term_width(tty);
	char* input = state.buffer;
	size_t input_len = state.end;

	tmenu_term_startofline(tty);
	tmenu_term_clearrest(tty, width);

	if (state.prompt) {
		fputs(state.prompt, tty);
		width -= strlen(state.prompt);
	}

	if (strlen(input) > width) {
		fwrite(input, sizeof(char), width - 3, tty);
		fputs("...", tty);
		fflush(tty);
		return;
	}

	fwrite(input, sizeof(char), input_len, tty);

	size_t suggestion_start = 30;

	if (input_len > suggestion_start - 2)
		suggestion_start = input_len + 2;

	int suggestion_width = width - suggestion_start - 3;

	for (size_t i = input_len; i < suggestion_start; i++)
		fputc(' ', tty);

	int suggestion_sum = 0;
	size_t shown = 0;
	for (size_t i = 0; !state.prompt && suggestion_sum < suggestion_width
	     && options[i] != NULL;
	     i++) {
		char* suggestion = options[i];
		int suggestion_len = strlen(suggestion);
		int new_sum = suggestion_sum + suggestion_len;

		if (input_len < 1 || strstr(suggestion, input)) {
			if (shown == selected_suggestion)
				tmenu_term_invert(tty);

			if (new_sum > suggestion_width - 3) {
				fputs(" >", tty);
				break;
			}

			fputc(' ', tty);
			fwrite(suggestion, sizeof(char), suggestion_len, tty);
			fputc(' ', tty);
			tmenu_term_normal(tty);

			suggestion_sum = new_sum + 2;
			shown += 1;
		}
	}

	options_current_count = shown;

	tmenu_term_startofline(tty);
	for (size_t i = 0; i < state.point; i++)
		tmenu_term_right(tty);

	fflush(tty);
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

	tmenu_input_initialize(tty, redraw, NULL);

	if (!(options = tmenu_options_read(stdin)))
		return EXIT_FAILURE;

	atexit(exiting);
	setup_resize_handler();
	tmenu_term_prep(tty);

	char* result = tmenu_input_ask();
	tmenu_term_startofline(tty);
	tmenu_term_clearrest(tty, 0);
	fputs(result, stdout);
	fputc('\n', stdout);
	return EXIT_SUCCESS;
}
