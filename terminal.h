/* terminal.h: terminal interface abstraction
 *
 *   (c) 2020-2021 thatlittlegit
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include "arguments.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

FILE* tmenu_term_initialize(void);

void tmenu_term_prep(FILE* tty, enum tmenu_location location);
void tmenu_term_deprep(FILE* tty);

void tmenu_term_clearrest(FILE* tty, unsigned spaces);
void tmenu_term_dimensions(FILE* tty, int* x, int* y);
size_t tmenu_term_height(FILE* tty);
void tmenu_term_invert(FILE* tty);
void tmenu_term_normal(FILE* tty);
void tmenu_term_right(FILE* tty);
void tmenu_term_position_goto(FILE* tty, int x, int y);
void tmenu_term_position_save(FILE* tty);
void tmenu_term_position_restore(FILE* tty);
void tmenu_term_startofline(FILE* tty);
size_t tmenu_term_width(FILE* tty);
