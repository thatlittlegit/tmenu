/* tmenu.c: dmenu on the terminal
 * (c) 2020 thatlittlegit
 * Licensed under the GPL 3.0 only.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <term.h>
#include <termios.h>
#include <unistd.h>

static int ttyfd = -1;
static char* buffer = NULL;
static size_t buffer_len = 0;
static size_t buffer_current_count = 0;
static size_t selected_suggestion = 0;
static char* last_input = NULL;
static struct termios original_termios;

static const char* progname = "tmenu";

static int back_suggestion(int, int);
static int forward_suggestion(int, int);
static void exiting(void);
static int main_loop(void);
static bool read_in_options(void);
static void redraw();
static void setup_resize_handler(void);
static bool setup_termcap(void);
static void setup_terminal(void);
static void strins(char* target, char chr, size_t offset);
static void term_clearrest(unsigned spaces);
static void term_invert(void);
static void term_normal(void);
static void term_right(void);
static void term_startofline(void);
static size_t term_width(void);
static bool term_write(const char* msg, int len);
static ssize_t writeall(int fd, const void* buf, size_t count);

static int
back_suggestion(int count, int key)
{
	if (selected_suggestion > 0)
		selected_suggestion -= (count ? count : 1);

	return 0;
}

static int
forward_suggestion(int count, int key)
{
	if (selected_suggestion < buffer_current_count - 1)
		selected_suggestion += (count ? count : 1);

	return 0;
}

static void
exiting(void)
{
	tcsetattr(ttyfd, 0, &original_termios);
}

static bool
read_in_options(void)
{
	/* 1024 picked arbitrarily */
	char readingbuf[1024] = { 0 };

	while (fgets(readingbuf, sizeof(readingbuf), stdin) != NULL) {
		size_t len = strnlen(readingbuf, sizeof(readingbuf));

		if (len == sizeof(readingbuf) - 1)
			readingbuf[sizeof(readingbuf)] = 0;
		else /* it must be newline-delimited */
			readingbuf[len - 1] = 0;

		buffer_len += len;
		buffer = realloc(buffer, buffer_len + 1);
		strncpy(buffer + buffer_len - len - 1, readingbuf, len);
	}

	if (feof(stdin))
		return true;

	perror("failed to read options");
	return false;
}

/* we can't use redraw(void) because we use it as a signal handler */
/* and thus it needs to be able to take an int argument */
static void
redraw()
{
	if (!last_input || strcmp(last_input, rl_line_buffer) != 0) {
		selected_suggestion = 0;
		free(last_input);
		last_input = malloc(rl_end);
		strncpy(last_input, rl_line_buffer, rl_end);
	}

	size_t width = term_width();
	char* input = rl_line_buffer;
	size_t input_len = rl_end;

	term_startofline();
	term_clearrest(width);

	if (RL_ISSTATE(RL_STATE_DISPATCHING)) {
		term_write(rl_display_prompt, -1);
		width -= strlen(rl_display_prompt);
	}

	if (strlen(input) > width) {
		term_write(input, width - 3);
		term_write("...", 3);
		return;
	}

	term_write(input, input_len);

	size_t suggestion_start = 30;

	if (input_len > suggestion_start - 2)
		suggestion_start = input_len + 2;

	int suggestion_width = width - suggestion_start - 3;

	for (size_t i = input_len; i < suggestion_start; i++)
		term_write(" ", 1);

	int suggestion_sum = 0;
	int i = 0;
	for (char* suggestion = buffer; !RL_ISSTATE(RL_STATE_DISPATCHING)
	     && suggestion_sum < suggestion_width && suggestion != NULL;) {
		char* suggestion_next = suggestion + strlen(suggestion) + 1;
		int suggestion_len = suggestion_next - suggestion;
		int new_sum = suggestion_sum + suggestion_len;

		if (input_len < 1 || strstr(suggestion, input)) {
			if (i == selected_suggestion)
				term_invert();

			term_write(" ", 1);

			if (new_sum > suggestion_width - 3) {
				term_write(
				    suggestion, new_sum - suggestion_width);
				term_write(" ", 1);
				term_normal();
				term_write(" >", 1);
			} else {
				term_write(suggestion, suggestion_len);
				term_write(" ", 1);
				term_normal();
			}

			suggestion_sum = new_sum + 2;
			i += 1;
		}

		if ((suggestion_next - buffer) < (long)buffer_len)
			suggestion = suggestion_next;
		else
			break;
	}

	buffer_current_count = i;

	term_startofline();
	for (int i = 0; i < rl_point; i++)
		term_right();
}

static void
setup_resize_handler(void)
{
	struct sigaction action;
	action.sa_handler = redraw;
	sigaction(SIGWINCH, &action, NULL);
}

static bool
setup_termcap(void)
{
	const char* term = getenv("TERM");

	/* term can be null, 'cause then setupterm will get $TERM itself, see
	 * it's null, and report an error
	 */
	int errret;
	if (setupterm(term, ttyfd, &errret) == -1) {
		if (errret == 0) {
			fprintf(stderr,
			    "%s: terminal type %s can't work at present\n",
			    progname, term);
			return false;
		}

		fprintf(stderr,
		    "%s: couldn't access the terminal database for %s\n",
		    progname, term);
		return false;
	}

	return true;
}

static void
setup_terminal()
{
	struct termios termios;

	tcgetattr(ttyfd, &original_termios);
	tcgetattr(ttyfd, &termios);
	termios.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(ttyfd, 0, &termios);
}

static void
strins(char* target, char chr, size_t offset)
{
	if (offset >= strlen(target)) {
		target[offset] = chr;
		target[offset + 1] = 0;
	}

	memmove(
	    target + offset + 1, target + offset, strlen(target) - offset - 1);
	target[offset] = chr;
}

static void
term_clearrest(unsigned len)
{
	if (term_write(clr_eol, -1))
		return;

	char* spaces = alloca(len);
	for (unsigned i = 0; i < len; i++)
		spaces[i] = ' ';
	writeall(ttyfd, spaces, len);
}

static void
term_invert(void)
{
	term_write(enter_standout_mode, -1);
}

static void
term_normal(void)
{
	term_write(exit_standout_mode, -1);
}

static void
term_right(void)
{
	term_write(cursor_right, -1);
}

static void
term_startofline(void)
{
	if (!term_write(carriage_return, -1))
		term_write("\r", 1);
}

static size_t
term_width(void)
{
	return (size_t)tigetnum("cols");
}

static bool
term_write(const char* msg, int len)
{
	/* todo: error handling */
	return writeall(ttyfd, msg, len < 0 ? strlen(msg) : (size_t)len);
}

static ssize_t
writeall(int fd, const void* buf, size_t count)
{
	ssize_t sum = 0;
	ssize_t retd;

	if (buf == NULL)
		return 0;

	while ((count - sum) > 0
	    && (retd = write(fd, (const char*)(buf) + sum, count - sum)) > 0)
		sum += retd;

	if (retd < 0)
		return -1;

	return sum;
}

int
main(int argc, char** argv)
{
	if (argc > 0)
		progname = argv[0];

	setlocale(LC_ALL, "");

	buffer = malloc((buffer_len = 1));
	buffer[0] = 0;

	/* this could be done before the options, but we might as well tell
	 * the user about invalid termcap before they've written their
	 * essay
	 */
	ttyfd = open("/dev/tty", O_RDWR);
	FILE* tty = fdopen(ttyfd, "a+");
	rl_instream = tty;
	rl_outstream = tty;

	if (!setup_termcap())
		return EXIT_FAILURE;

	if (!read_in_options())
		return EXIT_FAILURE;

	atexit(exiting);
	setup_resize_handler();
	setup_terminal();

	rl_redisplay_function = redraw;

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

	char* result = readline("-> ");
	term_startofline();
	term_clearrest(0);
	fputs(result, stdout);
	fputc('\n', stdout);
	return EXIT_SUCCESS;
}
