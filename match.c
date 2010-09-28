// ssdeep
// (C) Copyright 2010 ManTech International Corporation
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
//You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


#include "ssdeep.h"


// The longest line we should encounter when reading files of known hashes 
#define MAX_STR_LEN  2048


// ------------------------------------------------------------------
// LINKED LIST FUNCTIONS
// ------------------------------------------------------------------

static
int lsh_list_init(lsh_list *l)
{
  if (NULL == l)
    return TRUE;

  l->top    = NULL;
  l->bottom = NULL;
  return FALSE;
}

// Insert a signature from the file match_file into the list l.
// The value consists of the filename fn and the hash value sum.
static 
int lsh_list_insert(state    * s, 
		    char     * match_file, 
		    lsh_list * l, 
		    TCHAR    * fn, 
		    char     * sum)
{
  lsh_node *new;

  if (NULL == s || NULL == l || NULL == fn || NULL == sum)
    return TRUE;

  if ((new = (lsh_node *)malloc(sizeof(lsh_node))) == NULL)
    fatal_error("%s: Out of memory", __progname);

  new->next = NULL;
  if (((new->hash = strdup(sum)) == NULL) ||
      ((new->fn   = _tcsdup(fn))  == NULL))
  {
    print_error(s,"%s: out of memory", __progname);
    return TRUE;
  }
  if (match_file != NULL)
  {
    new->match_file = strdup(match_file);
    if (NULL == new->match_file)
    {
      print_error(s,"%s: out of memory", __progname);
      return TRUE;
    }
  }
  else
    new->match_file = NULL;

  if (l->bottom == NULL)
  {
    if (l->top != NULL)
      fatal_error("%s: internal data structure inconsistency", fn);

    l->top = new;
    l->bottom = new;
    return FALSE;
  }
  
  l->bottom->next = new;
  l->bottom = new;
  return FALSE;
}

// ------------------------------------------------------------------
// SIGNATURE FILE FUNCTIONS
// ------------------------------------------------------------------

typedef struct _file_info_t
{  
  FILE  * handle;
  TCHAR known_file_name[MAX_STR_LEN];
  char  known_hash[MAX_STR_LEN];
} file_info_t;

// Open a signature file and determine if it contains a valid header
// Returns TRUE on an error, otherwise FALSE.
static
int sig_file_open(state *s, char * fn, file_info_t * info)
{
  char str[MAX_STR_LEN];

  if (NULL == s || NULL == fn || NULL == info)
    return TRUE;

  info->handle = fopen(fn,"rb");
  if (NULL == info->handle)
  {
    if (!(MODE(mode_silent)))
      perror(fn);
    return TRUE;
  }

  // The first line should be the header. We don't need to chop it
  // as we're only comparing it to the length of the known header.
  if (NULL == fgets(str,MAX_STR_LEN,info->handle))
  {
    if (!(MODE(mode_silent)))
      perror(fn);
    fclose(info->handle);
    return TRUE;
  }

  if (strncmp(str,SSDEEPV1_0_HEADER,strlen(SSDEEPV1_0_HEADER)) &&
      strncmp(str,SSDEEPV1_1_HEADER,strlen(SSDEEPV1_1_HEADER)))
  {
    if (!MODE(mode_silent))
      print_error(s,"%s: invalid file header: %s!!", fn, str);
    fclose(info->handle);
    return TRUE;
  }

  return FALSE;
}
  

// Close a signature file
static
void sig_file_close(file_info_t * info)
{
  if (NULL == info)
    return;

  fclose(info->handle);
}


// Read the next entry in a signature file and store it in the structure 'info'
// If there is no next entry (EOF) or an error occurs, returns TRUE,
// otherwise FALSE.
static
int sig_file_next(state *s, file_info_t * info)
{
  char str[MAX_STR_LEN];

  if (NULL == s || NULL == info)
    return TRUE;

  if (NULL == fgets(str,MAX_STR_LEN,info->handle))
    return TRUE;
  
  chop_line(str);

  // The file format is:
  //     hash,"filename"

  strncpy(info->known_hash,str,MIN(MAX_STR_LEN,strlen(str)));
  find_comma_separated_string(info->known_hash,0);
  find_comma_separated_string(str,1);

  // Remove escaped quotes from the filename
  remove_escaped_quotes(str);
  
  // On Win32 we have to do a kludgy cast from ordinary char 
  // values to the TCHAR values we use internally. Because we may have
  // reset the string length, get it again.
  size_t i, sz = strlen(str);
  for (i = 0 ; i < sz ; i++)
  {
#ifdef _WIN32
    info->known_file_name[i] = (TCHAR)(str[i]);
#else
    info->known_file_name[i] = str[i];
#endif
  }
  info->known_file_name[i] = 0;
   
  return FALSE;
}


// ------------------------------------------------------------------
// MATCHING FUNCTIONS
// ------------------------------------------------------------------

int match_init(state *s)
{
  if (NULL == s)
    return TRUE;

  s->known_hashes = (lsh_list *)malloc(sizeof(lsh_list));
  if (s->known_hashes == NULL)
    return TRUE;
  
  lsh_list_init(s->known_hashes);  
  return FALSE;
}

#define STRINGS_EQUAL(A,B)    !_tcsncmp(A,B,MAX(_tcslen(A),_tcslen(B)))

// Match the file named fn with the hash sum against the set of knowns
// Display any matches. 
// Return FALSE is there are no matches, TRUE if at least one match
/// @param s State variable
/// @param match_file Filename where we got the hash of the unknown file.
///                   May be NULL.
/// @param fn Filename of the unknown file we are comparing
/// @param sum Fuzzy hash of the unknown file we are comparing
int match_compare(state *s, char * match_file, TCHAR *fn, char *sum)
{
  if (NULL == s || NULL == fn || NULL == sum)
    fatal_error("%s: Null values passed into match_compare", __progname);

  size_t fn_len  = _tcslen(fn);
  size_t sum_len = strlen(sum);

  int status = FALSE;
  int score;
  lsh_node *tmp = s->known_hashes->top;

  while (tmp != NULL)
  {
    if (s->mode & mode_match_pretty)
    {
      // Prevent printing the redundant "A matches A"
      if (!(_tcsncmp(fn,tmp->fn,MAX(fn_len,_tcslen(tmp->fn)))) &&
	  !(strncmp(sum,tmp->hash,MAX(sum_len,strlen(tmp->hash)))))
      {
	// Unless these results from different matching files (such as
	// what happens in sigcompare mode). That being said, we have to
	// be careful to avoid NULL values such as when working in 
	// normal pretty print mode.
	if (NULL == match_file || NULL == tmp->match_file ||
	    (!(strncmp(match_file, tmp->match_file, MAX(strlen(match_file),strlen(tmp->match_file))))))
	{
	  tmp = tmp->next;
	  continue;
	}
      }
    }

    score = fuzzy_compare(sum,tmp->hash);
    if (-1 == score)
    {
      print_error(s, "%s: Bad hashes in comparison", __progname);
    }
    else
    {
      if (score > s->threshold || MODE(mode_display_all))
      {
	if (s->mode & mode_csv)
	{
	  printf("\"");
	  display_filename(stdout,fn,TRUE);
	  printf("\",\"");
	  display_filename(stdout,tmp->fn,TRUE);
	  print_status("\",%"PRIu32, score);
	}
	else
	{
	  if (match_file != NULL)
	    printf ("%s:", match_file);
	  display_filename(stdout,fn,FALSE);
	  printf(" matches ");
	  if (tmp->match_file != NULL)
	    printf ("%s:", tmp->match_file);
	  display_filename(stdout,tmp->fn,FALSE);
	  print_status(" (%"PRIu32")", score);
	}
      
	// We don't return right away as this file could match more than
	// one signature. 
	status = TRUE;
      }
    }
    
    tmp = tmp->next;
  }

  return status;
}

int match_pretty(state *s)
{
  if (NULL == s)
    return TRUE;

  lsh_node *tmp = s->known_hashes->top;

  while (tmp != NULL)
  {
    if (match_compare(s,tmp->match_file,tmp->fn,tmp->hash))
      print_status("");

    tmp = tmp->next;
  }

  return FALSE;
}

int match_add(state *s, char * match_file, TCHAR *fn, char *hash)
{
  return (lsh_list_insert(s,match_file,s->known_hashes,fn,hash));
}


  
int match_load(state *s, char *fn)
{
  file_info_t info;
  int status = FALSE;

  if (NULL == s || NULL == fn)
    return TRUE;

  if (sig_file_open(s,fn,&info))
    return TRUE;

  while ( ! sig_file_next(s,&info))
  {
    if (match_add(s,fn,info.known_file_name,info.known_hash))
    {
      // If we can't insert this value, we're probably out of memory.
      // There's no sense trying to read the rest of the file.
      print_error(s,"%s: unable to insert hash", fn);
      status = TRUE;
      break;
    }
  }

  sig_file_close(&info);

  return status;
}


int match_compare_unknown(state *s, char * fn)
{ 
  file_info_t info;

  if (NULL == s || NULL == fn)
    return TRUE;

  if (sig_file_open(s,fn,&info))
    return TRUE;

  while ( ! sig_file_next(s,&info))
    match_compare(s,fn,info.known_file_name,info.known_hash);

  sig_file_close(&info);

  return FALSE;
}
