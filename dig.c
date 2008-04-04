
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
#include <dirent.h>

/* These are the types of files we can encounter while hashing */
#define file_regular    0
#define file_directory  1
#define file_door       3
#define file_block      4
#define file_character  5
#define file_pipe       6
#define file_socket     7
#define file_symlink    8
#define file_unknown  254

int process(state *s, char *fn);


static int file_type_helper(struct stat sb)
{
  if (S_ISREG(sb.st_mode))
    return file_regular;

  if (S_ISDIR(sb.st_mode))
    return file_directory;

  if (S_ISBLK(sb.st_mode))
    return file_block;

  if (S_ISCHR(sb.st_mode))
    return file_character;

  if (S_ISFIFO(sb.st_mode))
    return file_pipe;

  /* These file types do not exist in Win32 */
#ifndef _WIN32

  if (S_ISSOCK(sb.st_mode))
    return file_socket;

  if (S_ISLNK(sb.st_mode))
    return file_symlink;
#endif   /* ifndef _WIN32 */

  return file_unknown;
}

static int file_type(state *s, char *fn)
{
  struct stat sb;
  
  if (lstat(fn,&sb))
  {
    print_error(s,fn,strerror(errno));
    return file_unknown;
  }
  
  return file_type_helper(sb);
}


static int is_special_dir(char *d)
{
  return ((!strncmp(d,".",1) && (strlen(d) == 1)) ||
          (!strncmp(d,"..",2) && (strlen(d) == 2)));
}


static int process_directory(state *s, char *dir_name)
{
  char *fn;
  DIR *d;
  struct dirent *entry;

#ifndef _WIN32
  if (have_processed_dir(dir_name))
  {
    print_error(s,dir_name,"symlink creates cycle");
    return TRUE;
  }

  if (!processing_dir(dir_name))
    fatal_error(s,dir_name,"cycle checking failed to register directory");
#endif

  if ((d = opendir(dir_name)) == NULL)
  {
    print_error(s,dir_name,strerror(errno));
    return TRUE;
  }

  fn = (char *)malloc(sizeof(char) * PATH_MAX);
  while ((entry = readdir(d)) != NULL)
  {
    if (is_special_dir(entry->d_name))
      continue;

    snprintf(fn,PATH_MAX,"%s%c%s", dir_name, DIR_SEPARATOR, entry->d_name);
    process(s,fn);
  }
  free(fn);
  closedir(d);

#ifndef _WIN32
  if (!done_processing_dir(dir_name))
  {
    print_error(s,dir_name,"cycle checking failed to unregister directory");
    return TRUE;
  }
#endif

  return FALSE;
}


#ifndef _WIN32
static int should_process_symlink(state *s, char *fn)
{
  int type;
  struct stat sb;

  /* We must look at what this symlink points to before we process it.
     Symlinks to directories can cause cycles */
  if (stat(fn,&sb))
  {
    print_error(s,fn,strerror(errno));
    return FALSE;
  }

  type = file_type_helper(sb);
  
  if (type != file_directory)
    return TRUE;
  
  if (s->mode & mode_recursive)
    process_directory(s,fn);
  else
    print_error(s,fn,"Is a directory");    
  
  if (type == file_unknown)
    return FALSE;

  return FALSE;
}
#endif


static int should_process(state *s, char *fn)
{
  int type = file_type(s,fn);

  if (type == file_directory)
  {
    if (s->mode & mode_recursive)
      process_directory(s,fn);
    else
      print_error(s,fn,"Is a directory");
    return FALSE;
  }
  
#ifndef _WIN32
  if (type == file_symlink)
    return should_process_symlink(s,fn);
#endif

  // This handles non-existant files
  if (type == file_unknown)
    return FALSE;

  return TRUE;
}


static void remove_double_slash(char *fn)
{
  size_t pos;
  for (pos = 0 ; pos < strlen(fn) ; pos++)
  {
   if (fn[pos] == DIR_SEPARATOR && fn[pos+1] == DIR_SEPARATOR)
   {

     /* On Windows, we have to allow the first two characters to be slashes
	to account for UNC paths. e.g. \\foo\bar\cow */
#ifdef _WIN32
     if (pos == 0)
       continue;
#endif
     
     shift_string(fn,pos,pos+1);
     
     /* If we have leading double slashes, decrementing pos will make
	the value negative, but only for a short time. As soon as we
	end the loop we increment it again back to zero. */
     --pos;
   }
  }
}


static void remove_single_dirs(char *fn)
{
  unsigned int pos, chars_found = 0;
  for (pos = 0 ; pos < strlen(fn) ; pos++)
  {
    /* Catch strings that end with /. (e.g. /foo/.)  */
    if (pos > 0 && 
	fn[pos-1] == DIR_SEPARATOR && 
	fn[pos]   == '.' &&
	fn[pos+1] == 0)
      fn[pos] = 0;
    
    if (fn[pos] == '.' && fn[pos+1] == DIR_SEPARATOR)
    {
      if (chars_found && fn[pos-1] == DIR_SEPARATOR)
      {
	shift_string(fn,pos,pos+2);
	
	/* In case we have ././ we shift back one! */
	--pos;
      }
    }
    else 
      ++chars_found;
  }
}

/* Removes all "../" references from the absolute path fn */
static void remove_double_dirs(char *fn)
{
  unsigned int pos, next_dir;

  for (pos = 0; pos < strlen(fn) ; pos++)
  {
    if (fn[pos] == '.' && fn[pos+1] == '.')
    {
      if (pos > 0)
      {

	/* We have to keep this next if and the one above separate.
	   If not, we can't tell later on if the pos <= 0 or
	   that the previous character was a DIR_SEPARATOR.
	   This matters when we're looking at ..foo/ as an input */
	
	if (fn[pos-1] == DIR_SEPARATOR)
	{
	  
	  next_dir = pos + 2;
	  
	  /* Back up to just before the previous DIR_SEPARATOR
	     unless we're already at the start of the string */
	  if (pos > 1)
	    pos -= 2;
	  else
	    pos = 0;
	  
	  while (fn[pos] != DIR_SEPARATOR && pos > 0)
	    --pos;
	  
	  switch(fn[next_dir])
	  {
	  case DIR_SEPARATOR:
	    shift_string(fn,pos,next_dir);
	    break;
	    
	  case 0:
	    /* If we have /.. ending the filename */
	    fn[pos+1] = 0;
	    break;
	    
	    /* If we have ..foo, we should do nothing, but skip 
	       over these double dots */
	  default:
	    pos = next_dir;
	  }
	}
      }
      
      /* If we have two dots starting off the string, we should prepend
	 a DIR_SEPARATOR and ignore the two dots. That is:
	 from the root directory the path ../foo is really just /foo */
    
      else 
      {
	fn[pos] = DIR_SEPARATOR;
	shift_string(fn,pos+1,pos+3);
      }
    }
  }
}


/* On Win32 systems directories are handled... differently
   Attempting to process d: causes an error, but d:\ does not.
   Conversely, if you have a directory "foo",
   attempting to process d:\foo\ causes an error, but d:\foo does not.
   The following turns d: into d:\ and d:\foo\ into d:\foo */

#ifdef _WIN32
static void clean_name_win32(char *fn)
{
  unsigned int length = strlen(fn);

  if (length < 2)
    return;

  if (length == 2 && fn[1] == ':')
  {
    fn[length+1] = 0;
    fn[length]   = DIR_SEPARATOR;
    return;
  }

  if (fn[length-1] == DIR_SEPARATOR && length != 3)
  {
    fn[length - 1] = 0;
  }
}
#endif



static void clean_name(state *s, char *fn)
{
  /* If we're using a relative path, we don't want to clean the filename */
  if (!(s->mode & mode_relative))
  {
    remove_double_slash(fn);
    remove_single_dirs(fn);
    remove_double_dirs(fn);
  }

#ifdef _WIN32
  clean_name_win32(fn);
#endif
}


int process(state *s, char *fn)
{
  /* We have to clean the filename before we decide if we're going
     to process it. Even if we're not going to process, we may have
     to display an error message as to why we're not going to do so. */
  clean_name(s,fn);

  if (should_process(s,fn))
    return hash_file(s,fn);

  return FALSE;
}
