/* redraw.h: header for redrawing routines
 *
 *   (c) 2020 thatlittlegit
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SDPX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include "input.h"
#include <stdio.h>

/* typedef'd for future constness */
typedef char* tmenu_suggestions_t;

struct tmenu_redraw_state {
	FILE* tty;
	tmenu_suggestions_t* suggestions;
	char* last_input;
	size_t* selected_suggestion;
	int* matching_suggestions;
};

void tmenu_redraw(struct tmenu_input_state state, void* data);
