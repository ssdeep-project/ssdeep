
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

#include "main.h"

static int initialize_state(state *s)
{
  s->known_hashes = (lsh_list *)malloc(sizeof(lsh_list));
  if (s->known_hashes == NULL)
    return TRUE;
  
  lsh_list_init(s->known_hashes);  

  s->first_file_processed = TRUE;
  s->mode                 = mode_none;

  return FALSE;
}


static void usage(void)
{
  printf ("%s version %s by Jesse Kornblum%s", __progname, VERSION, NEWLINE);
  printf ("Copyright (C) 2006 ManTech CFIA%s", NEWLINE);
  printf ("Usage: %s [-V|h] [-m file] [-vprdsbl] [FILES]%s%s", 
	  __progname, NEWLINE, NEWLINE);

  printf ("-v - Verbose mode.%s", NEWLINE);
  printf ("-p - Pretty matching mode. Similar to -d but includes all matches%s", NEWLINE);
  printf ("-r - Recursive mode%s", NEWLINE);
  printf ("-d - Directory mode, compare all files in a directory%s", NEWLINE);
  printf ("-s - Silent mode; all errors are supressed%s", NEWLINE);
  printf ("-b - Uses only the bare name of files; all path information omitted%s", NEWLINE);
  printf ("-l - Uses relative paths for filenames%s", NEWLINE);
  printf ("-m - Match FILES against known hashes in file%s", NEWLINE);
  printf ("-h - Display this help message%s", NEWLINE);
  printf ("-V - Display version number and exit%s", NEWLINE);
}


static void try(void)
{  
  /* There's no point in checking for silent mode here. The user has 
     done something really wrong and needs all the help they can get. */
  fprintf (stderr,"Try `%s -h` for more information%s", __progname, NEWLINE);
  exit (EXIT_FAILURE);
}


static void sanity_check(state *s, int condition, char *msg)
{
  if (condition)
  {
    print_error(s, __progname, msg);
    try();
  }
}
  


static void process_cmd_line(state *s, int argc, char **argv)
{
  int i, match_files_loaded = FALSE;
  while ((i=getopt(argc,argv,"vhVpdsblrm:")) != -1) {
    switch(i) {

    case 'v': 
      if (MODE(mode_verbose))
      {
	print_error(s,NULL,"Already at maximum verbosity");
	print_error(s,NULL,"Error message display to user correctly");
      }
      else
	s->mode |= mode_verbose;
      break;
      
    case 'p':
      s->mode |= mode_match_pretty;
      break;

    case 'd':
      s->mode |= mode_directory; 
      break;

    case 's':
      s->mode |= mode_silent; break;

    case 'b':
      s->mode |= mode_barename; break;

    case 'l':
      s->mode |= mode_relative; break;

    case 'r':
      s->mode |= mode_recursive; break;
      
    case 'm':
      s->mode |= mode_match;
      if (!match_load(s,optarg))
	match_files_loaded = TRUE;
      break;
      
    case 'h':
      usage(); 
      exit (EXIT_SUCCESS);
      
    case 'V':
      printf ("%s%s", VERSION, NEWLINE);
      exit (EXIT_SUCCESS);
      
    default:
      try();
    }
  }

  sanity_check(s,
	       ((s->mode & mode_match) && !match_files_loaded),
	       "No matching files loaded");
  
  sanity_check(s,
	       ((s->mode & mode_barename) && (s->mode & mode_relative)),
	       "Relative paths and bare names are mutually exclusive");

  sanity_check(s,
	       ((s->mode & mode_match_pretty) && (s->mode & mode_directory)),
	       "Directory mode and pretty matching are mutallty exclusive");
}


int is_absolute_path(char *fn)
{
#ifdef _WIN32
  /* Windows has so many ways to make absolute paths (UNC, C:\, etc)
     that it's hard to keep track. It doesn't hurt us
     to call realpath as there are no symbolic links to lose. */
  return FALSE;
#else
  return (fn[0] == DIR_SEPARATOR);
#endif
}


void generate_filename(state *s, char *argv, char *fn, char *cwd)
{
  if ((s->mode & mode_relative) || is_absolute_path(argv))
    strncpy(fn,argv,PATH_MAX);
  else
  {
    /* Windows systems don't have symbolic links, so we don't
       have to worry about carefully preserving the paths
       they follow. Just use the system command to resolve the paths */
#ifdef _WIN32
    realpath(argv,fn);
#else
    if (cwd == NULL)
      /* If we can't get the current working directory, we're not
         going to be able to build the relative path to this file anyway.
         So we just call realpath and make the best of things */
      realpath(argv,fn);
    else
      snprintf(fn,PATH_MAX,"%s%c%s",cwd,DIR_SEPARATOR,argv);
#endif
  }
}


int main(int argc, char **argv)
{
  char *fn, *cwd;
  state *s = (state *)malloc(sizeof(state));

#ifndef __GLIBC__
  __progname = basename(argv[0]);
#endif

  if (s == NULL)
    fatal_error(s,NULL,"Unable to allocate memory for state");

  if (initialize_state(s))
    fatal_error(s,NULL,"Unable to initialize state");

  process_cmd_line(s,argc,argv);

  argv += optind;

  /* We can't process standard input because the algorithm may require
     us to restart the computation. There's no way to rewind standard 
     input */
  if (*argv == NULL)
    fatal_error(s,NULL,"No input files");

  fn  = (char *)malloc(sizeof(char) * PATH_MAX);
  cwd = (char *)malloc(sizeof(char) * PATH_MAX);
  if (fn == NULL || cwd == NULL)
    fatal_error(s,NULL,
		"Unable to allocate memory for current working directory");

  cwd = getcwd(cwd,PATH_MAX);
  if (cwd == NULL)
  {
    snprintf(cwd,PATH_MAX,
	     "%s: Unable to find current working directory", __progname);
    perror(cwd);
    return EXIT_FAILURE;
  }

  while (*argv != NULL)
  {
    generate_filename(s,*argv,fn,cwd);
    process(s,fn);
    if (s->mode & mode_verbose)
      fprintf(stderr,"%s\r", BLANK_LINE);
    ++argv;
  }

  if (s->mode & mode_match_pretty)
    match_pretty(s);

  /* We don't bother cleaning up the state, to include the list of 
     matching files, as we're about to exit. All of the memory we have
     allocated is going to be returned to the operating system, so 
     there's no point in our explicitly free'ing it. */

  return EXIT_SUCCESS;
}
