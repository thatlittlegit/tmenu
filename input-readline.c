/* input-readline.c: Readline driver for tmenu.
 * (c) 2020, 2024 Duncan McIntosh
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "input.h"
#include "options.h"
#include "tmenu.h"
#include <readline/readline.h>

static tmenu_input_redraw_t redraw;
static void* data;

static int
tmenu_options_accept(int count, int key)
{
	rl_replace_line(options[selected_option], 0);
	return rl_newline(count, key);
}

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

	rl_add_defun("forward-suggestion", tmenu_options_forward, -1);
	rl_bind_keyseq_if_unbound_in_map(
	    "\\M-\\C-f", tmenu_options_forward, emacs_standard_keymap);
	rl_bind_keyseq_if_unbound_in_map(
	    "L", tmenu_options_forward, vi_movement_keymap);

	rl_add_defun("back-suggestion", tmenu_options_back, -1);
	rl_bind_keyseq_if_unbound_in_map(
	    "\\M-\\C-b", tmenu_options_back, emacs_standard_keymap);
	rl_bind_keyseq_if_unbound_in_map(
	    "H", tmenu_options_back, vi_movement_keymap);

	rl_add_defun("accept-suggestion", tmenu_options_accept, -1);
	rl_bind_keyseq_if_unbound_in_map(
	    "\\M-\\C-p", tmenu_options_accept, emacs_standard_keymap);

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
