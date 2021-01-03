/* arguments.h: arguments-parsing abstraction
 *
 *   (c) 2021 thatlittlegit
 *   This file is part of the tmenu project.
 *   Licensed under the GNU GPL 3.0 only.
 *   SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

extern char* progname;

enum tmenu_location {
	TMENU_LOCATION_DEFAULT,
	TMENU_LOCATION_TOP,
	TMENU_LOCATION_BOTTOM,
};

struct tmenu_arguments {
	enum tmenu_location location;
};

#define _TMENU_USAGE_TEXT "usage: %s [-tbT]"
extern const char* const TMENU_USAGE_TEXT;

struct tmenu_arguments* tmenu_arguments_parse(int argc, char** argv);
