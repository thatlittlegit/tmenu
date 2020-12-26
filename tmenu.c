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
static struct termios original_termios;

static const char* progname = "tmenu";

static void exiting(void);
static int main_loop(void);
static bool read_in_options(void);
static void redraw();
static void setup_resize_handler(void);
static bool setup_termcap(void);
static void setup_terminal(void);
static void strins(char* target, char chr, size_t offset);
static bool strprefix(char* str, size_t len, char* other, size_t other_len);
static void term_clearrest(unsigned spaces);
static void term_right(void);
static void term_startofline(void);
static size_t term_width(void);
static bool term_write(const char* msg, int len);
static ssize_t writeall(int fd, const void* buf, size_t count);

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
	char* fgets_ret = NULL;

	while ((fgets_ret = fgets(readingbuf, sizeof(readingbuf), stdin))
	    != NULL) {
		buffer_len += strnlen(readingbuf, sizeof(readingbuf));
		buffer = realloc(buffer, buffer_len + 1);
		strncat(buffer, readingbuf, buffer_len);
	}

	buffer[buffer_len] = 0;

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
	if (RL_ISSTATE(RL_STATE_DISPATCHING)) {
		rl_clear_visible_line();
		rl_redisplay();
		return;
	}

	size_t width = term_width();
	char* input = rl_line_buffer;
	size_t input_len = rl_end;

	term_startofline();
	term_clearrest(width);

	if (strlen(input) > width) {
		term_write(input, width - 3);
		term_write("...", 3);
		return;
	}

	term_write(input, input_len);

	size_t suggestion_start = 30;

	if (input_len > suggestion_start - 2)
		suggestion_start = input_len + 2;

	int suggestion_width = width - suggestion_start - 1;

	for (size_t i = input_len; i < suggestion_start; i++)
		term_write(" ", 1);

	int suggestion_sum = 0;
	for (char* suggestion = buffer; suggestion_sum < suggestion_width
	     && suggestion != NULL && suggestion[1] != 0;) {
		char* suggestion_next = strchr(suggestion, '\n');
		int suggestion_len = suggestion_next - suggestion;
		int new_sum = suggestion_sum + suggestion_len;

		if (input_len <= 1
		    || strprefix(
			suggestion, suggestion_len, input, input_len)) {
			if (new_sum > suggestion_width) {
				term_write(
				    suggestion, new_sum - suggestion_width);
				term_write(" >", 2);
			} else {
				term_write(suggestion, suggestion_len);
				term_write("  ", 2);
			}
			suggestion_sum = new_sum + 2;
		}

		suggestion = suggestion_next + 1;
	}

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
			    progname,
			    term);
			return false;
		}

		fprintf(stderr,
		    "%s: couldn't access the terminal database for %s\n",
		    progname,
		    term);
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
	fprintf(stderr,
	    "\n%llu '%c' {%d} %p (%s %d)\n",
	    offset,
	    chr,
	    chr,
	    target,
	    target,
	    strlen(target));

	if (offset >= strlen(target)) {
		target[offset] = chr;
		target[offset + 1] = 0;
	}

	memmove(
	    target + offset + 1, target + offset, strlen(target) - offset - 1);
	target[offset] = chr;
}

static bool
strprefix(char* str, size_t len, char* other, size_t other_len)
{
	for (size_t i = 0; i < len && i < other_len; i++)
		if (other[i] != str[i])
			return false;

	return true;
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

	char* result = readline("-> ");
	term_startofline();
	term_clearrest(0);
	fputs(result, stdout);
	fputc('\n', stdout);
	return EXIT_SUCCESS;
}
