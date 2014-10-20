#ifndef __EDIT_DIST_H
#define __EDIT_DIST_H
/**
 * Modified levenshtein distance calculation
 *
 * This program can be used, redistributed or modified under any of
 * Boost Software License 1.0, GPL v2 or GPL v3
 * See the file COPYING for details.
 *
 * $Id$
 *
 * Copyright (C) 2014 kikairoya <kikairoya@gmail.com>
 * Copyright (C) 2014 Jesse Kornblum <research@jessekornblum.com>
 */

int edit_distn(const char *s1, size_t s1len, const char *s2, size_t s2len);

#endif  // ifndef __EDIT_DIST_H
