/* terminal.h: terminal interface abstraction
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

FILE* tmenu_term_initialize(void);

void tmenu_term_prep(FILE* tty);
void tmenu_term_deprep(FILE* tty);

void tmenu_term_clearrest(FILE* tty, unsigned spaces);
void tmenu_term_invert(FILE* tty);
void tmenu_term_normal(FILE* tty);
void tmenu_term_right(FILE* tty);
void tmenu_term_startofline(FILE* tty);
size_t tmenu_term_width(FILE* tty);
