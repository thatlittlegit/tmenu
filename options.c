/* options.c: reads in options to an array
 *
 *   (c) 2020-2021 thatlittlegit
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#include "options.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OPTION_LEN_UNIT
#define OPTION_LEN_UNIT 8
#endif

size_t selected_option;
int* options_current_count;

int
tmenu_options_back(int count, int key)
{
	(void)key;

	if (selected_option > 0)
		selected_option -= (count ? count : 1);

	return 0;
}

int
tmenu_options_forward(int count, int key)
{
	(void)key;

	if ((int)selected_option < *options_current_count - 1)
		selected_option += (count ? count : 1);

	return 0;
}

void
tmenu_options_navigation_initialize(int* occ)
{
	options_current_count = occ;
}

char**
tmenu_options_read(FILE* input)
{
	char** outbuffer = NULL;
	size_t outbuffer_len = 0;

	char* buf = malloc(OPTION_LEN_UNIT);
	size_t option_start = 0;
	size_t buf_len = OPTION_LEN_UNIT;

	while (fgets(buf + option_start, buf_len - option_start, input)) {
		size_t max_len = buf_len - option_start;

		char* newline;
		if (feof(input))
			newline = memchr(buf + option_start, '\0', max_len);
		else
			newline = memchr(buf + option_start, '\n', max_len);

		if (!newline) {
			option_start += max_len - 1;
			buf_len += OPTION_LEN_UNIT;
			buf = realloc(buf, buf_len);
			continue;
		}

		/* TODO reallocarray? */
		*newline = 0;
		outbuffer = realloc(outbuffer, ++outbuffer_len * sizeof(char*));
		outbuffer[outbuffer_len - 1] = buf;

		option_start = 0;
		buf = malloc(OPTION_LEN_UNIT);
		buf_len = OPTION_LEN_UNIT;
	}

	outbuffer = realloc(outbuffer, ++outbuffer_len * sizeof(char*));
	outbuffer[outbuffer_len - 1] = NULL;
	free(buf);

	if (feof(input))
		return outbuffer;

	perror("failed to read options");
	free(outbuffer);
	free(buf);
	return NULL;
}

void
tmenu_options_free(char** options)
{
	for (size_t i = 0; options[i] != NULL; i++)
		free(options[i]);

	free(options);
}
