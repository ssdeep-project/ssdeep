// ssdeep
// Copyright (C) 2010 ManTech International Corporation.
//
// $Id$
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef __MAIN_H
#define __MAIN_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif 

#ifdef HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif

#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif


// This allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more.
// Technically it isn't needed in ssdeep as we don't process standard
// input. But it was part of Jesse's template, so in it goes!
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif


#include "fuzzy.h"
#include "tchar-local.h"

#ifndef HAVE_FSEEKO
# define fseeko fseek
# define ftello ftell
#endif

#define FALSE  0
#define TRUE   1

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define MM_INIT  printf

#define SSDEEPV1_0_HEADER        "ssdeep,1.0--blocksize:hash:hash,filename"
#define SSDEEPV1_1_HEADER        "ssdeep,1.1--blocksize:hash:hash,filename"
#define OUTPUT_FILE_HEADER     SSDEEPV1_1_HEADER

// We print a warning for files smaller than this size
#define SSDEEP_MIN_FILE_SIZE   4096

#define MD5DEEP_ALLOC(TYPE,VAR,SIZE)     \
VAR = (TYPE *)malloc(sizeof(TYPE) * SIZE);  \
if (NULL == VAR)  \
   return EXIT_FAILURE; \
memset(VAR,0,SIZE * sizeof(TYPE));


// These are the types of files we can encounter while hashing
#define file_regular    0
#define file_directory  1
#define file_door       2
#define file_block      3
#define file_character  4
#define file_pipe       5
#define file_socket     6
#define file_symlink    7
#define file_unknown  254

typedef struct _lsh_node {
  char    *hash;
  TCHAR   *fn;
  char    *match_file;
  struct _lsh_node *next;
} lsh_node;

typedef struct {
  lsh_node  *top, *bottom;
} lsh_list;


typedef struct {
  int       first_file_processed;
  uint64_t  mode;
  lsh_list  *known_hashes;
  uint8_t   threshold;
  int       found_meaningful_file;
  int       processed_file;
  int       argc;
  TCHAR     **argv;
} state;



// Things required when cross compiling for Microsoft Windows
#ifdef _WIN32

// We create macros for the Windows equivalent UNIX functions.
// No worries about lstat to stat; Windows doesn't have symbolic links 
#define lstat(A,B)      stat(A,B)
#define realpath(A,B)   _fullpath(B,A,PATH_MAX)
#define snprintf        _snprintf

char *basename(char *a);
extern char *optarg;
extern int optind;
int getopt(int argc, char *const argv[], const char *optstring);

// Although newlines on Win32 are supposed to \r\n, when this program
// is compiled with mingw and run on WindowsXP, the program seems to 
// use \r\n anyway. I'm not complaining, just curious what's going on.
// In the meantime, however, we can just use \n for the NEWLINE 
#define NEWLINE        "\n"
#define DIR_SEPARATOR  '\\'

#else   // ifdef _WIN32
// For all other operating systems

#define NEWLINE       "\n"
#define DIR_SEPARATOR '/'

#endif  // ifdef _WIN32/else





// Because the modes are stored in a uint64_t variable, they must
// be less than or equal to 1<<63
#define mode_none            0
#define mode_recursive       1
#define mode_match        1<<1
#define mode_barename     1<<2
#define mode_relative     1<<3
#define mode_silent       1<<4 
#define mode_directory    1<<5
#define mode_match_pretty 1<<6
#define mode_verbose      1<<7
#define mode_csv          1<<8
#define mode_threshold    1<<9
#define mode_sigcompare   1<<10
#define mode_display_all  1<<11
#define mode_compare_unknown 1<<12

#define MODE(A)   (s->mode & A)

#define BLANK_LINE   \
"                                                                               "



// *********************************************************************
// Checking for cycles
// *********************************************************************
int done_processing_dir(TCHAR *fn);
int processing_dir(TCHAR *fn);
int have_processed_dir(TCHAR *fn);

int process_win32(state *s, TCHAR *fn);
int process_normal(state *s, TCHAR *fn);
int process_stdin(state *s);


// *********************************************************************
// Fuzzy Hashing Engine
// *********************************************************************
int hash_file(state *s, TCHAR *fn);
void display_result(state *s, TCHAR * fn, char * sum);


// *********************************************************************
// Helper functions
// *********************************************************************
void try_msg(void);
void sanity_check(state *s, int condition, char *msg);

// The basename function kept misbehaving on OS X, so I rewrote it.
// This function isn't perfect, nor is it designed to be. Because
// we're guarenteed to be working with a filename here, there's no way
// that s will end with a DIR_SEPARATOR (e.g. /foo/bar/). This function
// will not work properly for a string that ends in a DIR_SEPARATOR 
int my_basename(TCHAR *s);
int my_dirname(TCHAR *s);

// Remove the newlines, if any, from the string. Works with both
// \r and \r\n style newlines 
void chop_line_tchar(TCHAR *s);
void chop_line(char *s);

int find_comma_separated_string_tchar(TCHAR *s, unsigned int n);
void shift_string_tchar(TCHAR *fn, unsigned int start, unsigned int new_start);

int find_comma_separated_string(char *s, unsigned int n);
void shift_string(char *fn, size_t start, size_t new_start);

int remove_escaped_quotes(char * str);

void prepare_filename(state *s, TCHAR *fn);

// Returns the size of the given file, in bytes.
off_t find_file_size(FILE *h);


// *********************************************************************
// User Interface Functions
// *********************************************************************
void print_status(char *fmt, ...);
void print_error(state *s, char *fmt, ...);
void print_error_unicode(state *s, TCHAR *fn, char *fmt, ...);
void internal_error(char *fmt, ... );
void fatal_error(char *fmt, ... );
void display_filename(FILE *out, TCHAR *fn, int escape_quotes);


// *********************************************************************
// Matching functions
// *********************************************************************

int match_init(state *s);

// See if the existing file and hash are in the set of known hashes
int match_compare(state *s, char * match_file, TCHAR *fn, char *sum);

// Load a file of known hashes
int match_load(state *s, char *fn);

// Add a single new hash to the set of known hashes
int match_add(state *s, char * match_file, TCHAR *fn, char *hash);

// Display all matches in the set of known hashes nicely
int match_pretty(state *s);

// Load the known hashes from the file fn and compare them to the
// set of known hashes
int match_compare_unknown(state *s, char * fn);

#endif   // #ifndef __MAIN_H
