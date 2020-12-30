/* options.h: simple header file for options.c
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once
#include <stdio.h>

/* wow very complex */
char** tmenu_options_read(FILE* input);
void tmenu_options_free(char**);
