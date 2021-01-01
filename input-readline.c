/* input-readline.c: Readline driver for tmenu.
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "input.h"
#include <readline/readline.h>

static tmenu_input_redraw_t redraw;
static void* data;

int back_suggestion(int, int);
int forward_suggestion(int, int);

char*
tmenu_input_ask(void)
{
	return readline("-> ");
}

void
tmenu_input_initialize(
    FILE* stream, tmenu_input_redraw_t redraw_func, void* _data)
{
	rl_instream = stream;
	rl_outstream = stream;
	data = _data;

	redraw = redraw_func;
	rl_redisplay_function = tmenu_input_redraw;

	rl_add_defun("forward-suggestion", forward_suggestion, -1);
	rl_bind_keyseq_if_unbound_in_map(
	    "\\M-\\C-f", forward_suggestion, emacs_standard_keymap);
	rl_bind_keyseq_if_unbound_in_map(
	    "L", forward_suggestion, vi_movement_keymap);

	rl_add_defun("back-suggestion", back_suggestion, -1);
	rl_bind_keyseq_if_unbound_in_map(
	    "\\M-\\C-b", back_suggestion, emacs_standard_keymap);
	rl_bind_keyseq_if_unbound_in_map(
	    "H", back_suggestion, vi_movement_keymap);

	rl_set_signals();
}

void
tmenu_input_redraw(void)
{
	char* prompt;
	if (strcmp(rl_display_prompt, rl_prompt) == 0)
		prompt = NULL;
	else
		prompt = rl_display_prompt;

	redraw((struct tmenu_input_state) { rl_line_buffer, rl_end, rl_point,
		   rl_mark, prompt },
	    data);
}
