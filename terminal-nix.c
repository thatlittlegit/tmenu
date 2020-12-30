/* terminal-nix.c: terminal abstraction for UNIX machines
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#define _POSIX_C_SOURCE

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <term.h>
#include <termios.h>

static struct termios original_termios;

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
}

void
tmenu_term_clearrest(FILE* tty, unsigned len)
{
	if (fputs(clr_eol, tty))
		return;

	char* spaces = alloca(len);
	for (unsigned i = 0; i < len; i++)
		spaces[i] = ' ';
	fwrite(spaces, sizeof(char), len, tty);
}

void
tmenu_term_invert(FILE* tty)
{
	fputs(enter_standout_mode, tty);
}

void
tmenu_term_normal(FILE* tty)
{
	fputs(exit_standout_mode, tty);
}

void
tmenu_term_right(FILE* tty)
{
	fputs(cursor_right, tty);
}

void
tmenu_term_startofline(FILE* tty)
{
	if (!fputs(carriage_return, tty))
		fputc('\r', tty);
}

size_t
tmenu_term_width(FILE* tty)
{
	(void)tty;
	return (size_t)tigetnum("cols");
}
