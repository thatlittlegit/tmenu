/* terminal-nix.c: terminal abstraction for UNIX machines
 *
 *   (c) 2020-2021 thatlittlegit
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#define _POSIX_C_SOURCE

#include "terminal.h"
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <term.h>
#include <termios.h>

static struct termios original_termios;
static FILE* current_tty;

static int tmenu_term_putchar(int chr);
static int tmenu_term_tputs(const char* str, FILE* tty);

FILE*
tmenu_term_initialize()
{
	FILE* tty = fopen("/dev/tty", "a+");
	int ttyfd = fileno(tty);

	const char* term = getenv("TERM");

	/* term can be null, 'cause then setupterm will get $TERM itself, see
	 * it's null, and report an error
	 */
	int errret;
	if (setupterm(term, ttyfd, &errret) == -1) {
		fclose(tty);

		if (errret == 0) {
			fprintf(stderr,
			    "terminal type %s can't work at present\n", term);
			return NULL;
		}

		fprintf(stderr,
		    "couldn't access the terminal database for %s\n", term);
		return NULL;
	}

	return tty;
}

void
tmenu_term_prep(FILE* tty)
{
	int ttyfd = fileno(tty);
	struct termios termios;

	tcgetattr(ttyfd, &original_termios);
	tcgetattr(ttyfd, &termios);
	termios.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(ttyfd, 0, &termios);
}

void
tmenu_term_deprep(FILE* tty)
{
	tcsetattr(fileno(tty), 0, &original_termios);
	fclose(tty);
	/* FIXME this still leaks? */
	del_curterm(cur_term);
}

void
tmenu_term_clearrest(FILE* tty, unsigned len)
{
	if (tmenu_term_tputs(clr_eol, tty) != ERR)
		return;

	char* spaces = alloca(len);
	for (unsigned i = 0; i < len; i++)
		spaces[i] = ' ';
	fwrite(spaces, sizeof(char), len, tty);
}

void
tmenu_term_invert(FILE* tty)
{
	tmenu_term_tputs(enter_standout_mode, tty);
}

void
tmenu_term_normal(FILE* tty)
{
	tmenu_term_tputs(exit_standout_mode, tty);
}

void
tmenu_term_right(FILE* tty)
{
	tmenu_term_tputs(cursor_right, tty);
}

void
tmenu_term_startofline(FILE* tty)
{
	if (tmenu_term_tputs(carriage_return, tty) == ERR)
		fputc('\r', tty);
}

size_t
tmenu_term_width(FILE* tty)
{
	struct winsize size;
	ioctl(fileno(tty), TIOCGWINSZ, &size);
	return size.ws_col;
}

static int
tmenu_term_tputs(const char* str, FILE* tty)
{
	current_tty = tty;
	int ret = tputs(str, 1, tmenu_term_putchar);
	current_tty = NULL;
	return ret;
}

static int
tmenu_term_putchar(int chr)
{
	return fputc(chr, current_tty);
}
