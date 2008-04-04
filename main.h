
/* ssdeep
   (C) Copyright 2006 ManTech CFIA.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef __LINUX
// These are used to find file sizes of block devices. See helpers.c
#include <sys/ioctl.h>
#include <sys/mount.h>
#endif


#define FALSE  0
#define TRUE   1


#define MM_INIT  printf

#define SSDEEPV1_HEADER        "ssdeep,1.0--blocksize:hash:hash,filename"
#define OUTPUT_FILE_HEADER     SSDEEPV1_HEADER


typedef struct _lsh_node {
  char             *hash, *fn;
  struct _lsh_node *next;
} lsh_node;

typedef struct {
  lsh_node         *top, *bottom;
} lsh_list;

typedef struct {
  int       first_file_processed;
  uint64_t  mode;
  lsh_list  *known_hashes;

  // These are used when computing where matched files are different
  uint32_t  block_size;

} state;



// Things required when cross compiling for Microsoft Windows
#ifdef _WIN32

/* We create macros for the Windows equivalent UNIX functions.
   No worries about lstat to stat; Windows doesn't have symbolic links */
#define lstat(A,B)      stat(A,B)
#define realpath(A,B)   _fullpath(B,A,PATH_MAX)
#define snprintf        _snprintf

char *basename(char *a);
extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);

/* Although newlines on Win32 are supposed to \r\n, when this program
   is compiled with mingw and run on WindowsXP, the program seems to 
   use \r\n anyway. I'm not complaining, just curious what's going on.
   In the meantime, however, we can just use \n for the NEWLINE */
#define NEWLINE        "\n"
#define DIR_SEPARATOR  '\\'

#else   // ifdef _WIN32
// For all other operating systems

#define NEWLINE       "\n"
#define DIR_SEPARATOR '/'

#endif  // ifdef _WIN32/else

#ifdef __GLIBC__
extern char *__progname;
#else 
char *__progname;
#endif


/* Because the modes are stored in a uint64_t variable, they must
   be less than or equal to 1<<63 */
#define mode_none            0
#define mode_recursive       1
#define mode_match        1<<1
#define mode_barename     1<<2
#define mode_relative     1<<3
#define mode_silent       1<<4 
#define mode_directory    1<<5
#define mode_match_pretty 1<<6
#define mode_verbose      1<<7

#define MODE(A)   (s->mode & A)
#define BLANK_LINE   \
"                                                                        "



// *********************************************************************
// Checking for cycles
// *********************************************************************
int done_processing_dir(char *fn);
int processing_dir(char *fn);
int have_processed_dir(char *fn);

int process(state *s, char *fn);

// *********************************************************************
// Engine functions
// *********************************************************************
int hash_file(state *s, char *fn);

// Returns a score from 0-100 about how well the two sums match
uint32_t spamsum_match(state *s, const char *str1, const char *str2);



// *********************************************************************
// Helper functions
// *********************************************************************

/* The basename function kept misbehaving on OS X, so I rewrote it.
   This function isn't perfect, nor is it designed to be. Because
   we're guarenteed to be working with a filename here, there's no way
   that s will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
   will not work properly for a string that ends in a DIR_SEPARATOR */
int my_basename(char *s);

void print_error(state *s, char *fn, char *msg);
void fatal_error(state *s, char *fn, char *msg);

/* Remove the newlines, if any, from the string. Works with both
   \r and \r\n style newlines */
void chop_line(char *s);

int find_comma_separated_string(char *s, unsigned int n);
void shift_string(char *fn, unsigned int start, unsigned int new_start);
void prepare_filename(state *s, char *fn);

/* Returns the size of the given file, in bytes. */
off_t find_file_size(FILE *h);

// *********************************************************************
// Matching functions
// *********************************************************************
int lsh_list_init(lsh_list *l);


// See if the existing file and hash are in the set of known hashes
int match_compare(state *s, char *fn, char *sum);

// Load a file of known hashes
int match_load(state *s, char *fn);

// Add a single new hash to the set of known hashes
int match_add(state *s, char *fn, char *hash);

// Display all matches in the set of known hashes nicely
int match_pretty(state *s);

#endif   // #ifndef __MAIN_H
