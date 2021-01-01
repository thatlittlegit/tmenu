/* redraw.c: redrawing routines for tmenu
 *
 *   (c) 2020 thatlittlegit
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#include "redraw.h"
#include "terminal.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void tmenu_redraw_reset_suggestion(char** last_input, const char* buffer,
    size_t buffer_len, size_t* selected_suggestion);
static int tmenu_redraw_suggestions(size_t width, const char* buffer,
    size_t buffer_len, size_t selected, tmenu_suggestions_t* suggestions,
    FILE* tty);

void
tmenu_redraw(struct tmenu_input_state input, void* data)
{
	struct tmenu_redraw_state* redraw = (struct tmenu_redraw_state*)data;
	FILE* tty = redraw->tty;
	tmenu_suggestions_t* suggestions = redraw->suggestions;
	char** last_input = &redraw->last_input;
	size_t* selected = redraw->selected_suggestion;
	int* matching_suggestions = &redraw->matching_suggestions;

	char* buffer = input.buffer;
	size_t end = input.end;
	size_t point = input.point;
	size_t mark = input.mark;
	char* prompt = input.prompt;
	(void)mark;

	size_t width = tmenu_term_width(tty);

	if (!*last_input || strcmp(*last_input, buffer) != 0)
		/* the buffer changed, reset the suggestion */
		tmenu_redraw_reset_suggestion(
		    last_input, buffer, end, selected);

	tmenu_term_startofline(tty);
	tmenu_term_clearrest(tty, width);

	/* if there is a prompt, make sure to print it */
	if (prompt) {
		fputs(prompt, tty);
		fputc(' ', tty);
		width -= strlen(prompt) + 1;
	}

	/* is the input too long? if so, truncate it */
	/* NOTE should we allow scroll? dmenu doesn't */
	if (end > width) {
		fwrite(buffer, sizeof(char), width - 3, tty);
		fputs("...", tty);
		fflush(tty);
		return;
	}

	/* write out the input normally */
	fwrite(buffer, sizeof(char), end, tty);

	if (!prompt)
		*matching_suggestions = tmenu_redraw_suggestions(
		    width, buffer, end, *selected, suggestions, tty);

	tmenu_term_startofline(tty);
	for (size_t i = 0; i < point; i++)
		tmenu_term_right(tty);

	fflush(tty);
}

static void
tmenu_redraw_reset_suggestion(char** last_input, const char* buffer,
    size_t buffer_len, size_t* selected_suggestion)
{
	*selected_suggestion = 0;
	free(*last_input);

	*last_input = malloc(buffer_len + 1);
	strncpy(*last_input, buffer, buffer_len + 1);
	(*last_input)[buffer_len] = 0;
}

static int
tmenu_redraw_suggestions(size_t width, const char* buffer, size_t buffer_len,
    size_t selected, tmenu_suggestions_t* suggestions, FILE* tty)
{
	int total_width = 0;
	int matching = 0;

	size_t min_offset = width / 5;
	int offset_amount
	    = (int)(buffer_len > min_offset ? buffer_len + 2 : min_offset);
	total_width += offset_amount + buffer_len;

	for (int i = buffer_len; i < offset_amount; i++)
		fputc(' ', tty);

	for (int i = 0; total_width < (int)width && suggestions[i] != NULL;
	     i++) {
		const char* suggestion = suggestions[i];

		if (buffer_len > 1 && !strstr(suggestion, buffer))
			continue;

		if (matching++ == (int)selected)
			tmenu_term_invert(tty);

		total_width += strlen(suggestion) + 2;
		if (total_width > (int)width - 2) {
			fputs(" >", tty);
			tmenu_term_normal(tty);
			break;
		}

		fprintf(tty, " %s ", suggestion);
		tmenu_term_normal(tty);
	}

	return matching;
}
