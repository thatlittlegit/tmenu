/* input.h: standard input API
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stddef.h>
#include <stdio.h>

struct tmenu_input_state {
	char* buffer;
	size_t end;
	size_t point;
	size_t mark;
	char* prompt;
};

typedef void (*tmenu_input_redraw_t)(struct tmenu_input_state, void* data);

char* tmenu_input_ask(void);
void tmenu_input_initialize(FILE* stream, tmenu_input_redraw_t, void* data);
void tmenu_input_redraw(void);
