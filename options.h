/* options.h: options-related header file
 *
 *   (c) 2020-2021 thatlittlegit
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once
#include <stddef.h>
#include <stdio.h>

extern size_t selected_option;
void tmenu_options_navigation_initialize(int* occ);
/* generic because needs to be usable by any backend */
int tmenu_options_back();
int tmenu_options_forward();

char** tmenu_options_read(FILE* input);
void tmenu_options_free(char**);
