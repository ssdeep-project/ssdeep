
/* ssdeep
   (C) Copyright 2006 ManTech International Corporation

   $Id$

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


#include "ssdeep.h"


/* The longest line we should encounter when reading files of known hashes */
#define MAX_STR_LEN  2048




int lsh_list_init(lsh_list *l)
{
  l->top    = NULL;
  l->bottom = NULL;
  return FALSE;
}


int match_init(state *s)
{
  s->known_hashes = (lsh_list *)malloc(sizeof(lsh_list));
  if (s->known_hashes == NULL)
    return TRUE;
  
  lsh_list_init(s->known_hashes);  
  return FALSE;
}



#define STRINGS_EQUAL(A,B)    !_tcsncmp(A,B,MAX(_tcslen(A),_tcslen(B)))

int match_compare(state *s, TCHAR *fn, char *sum)
{
  size_t fn_len  = _tcslen(fn);
  size_t sum_len = strlen(sum);

  int status = FALSE;
  int score;
  lsh_node *tmp = s->known_hashes->top;

  while (tmp != NULL)
  {
    if (s->mode & mode_match_pretty)
    {
      /* Prevent printing the redundant "A matches A" */
      if (!(_tcsncmp(fn,tmp->fn,MAX(fn_len,_tcslen(tmp->fn)))) &&
	  !(strncmp(sum,tmp->hash,MAX(sum_len,strlen(tmp->hash)))))
      {
	tmp = tmp->next;
	continue;
      }
    }

    score = fuzzy_compare(sum,tmp->hash);
    if (score > s->threshold)
    {
      if (s->mode & mode_csv)
	{
	  display_filename(stdout,fn);
	  printf(",");
	  display_filename(stdout,tmp->fn);
	  print_status(",%"PRIu32, score);
	}
      else
	{
	  display_filename(stdout,fn);
	  printf(" matches ");
	  display_filename(stdout,tmp->fn);
	  print_status(" (%"PRIu32")", score);
	}
      
      /* We don't return right away as this file could match more than
	 one signature.  */
      status = TRUE;
    }
    
    tmp = tmp->next;
  }

  return status;
}


static int lsh_list_insert(state *s, lsh_list *l, TCHAR *fn, char *sum)
{
  lsh_node *new;

  if ((new = (lsh_node *)malloc(sizeof(lsh_node))) == NULL)
    fatal_error("%s: Out of memory", __progname);

  new->next = NULL;
  if (((new->hash = strdup(sum)) == NULL) ||
      ((new->fn   = _tcsdup(fn))  == NULL))
  {
    print_error(s,"%s: out of memory", __progname);
    return TRUE;
  }

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


int match_pretty(state *s)
{
  lsh_node *tmp = s->known_hashes->top;

  while (tmp != NULL)
  {
    if (match_compare(s,tmp->fn,tmp->hash))
      print_status("");

    tmp = tmp->next;
  }

  return FALSE;
}


int match_add(state *s, TCHAR *fn, char *hash)
{
  return (lsh_list_insert(s,s->known_hashes,fn,hash));
}


int match_load(state *s, char *fn)
{
  size_t tchar_sz = sizeof(TCHAR);
  TCHAR *known_file_name;
  char *str, *known_hash;
  FILE *handle;

  if ((handle = fopen(fn,"rb")) == NULL)
  {
    if (!(s->mode & mode_silent))
      perror(fn);
    return TRUE;
  }

  str = (char *)malloc(sizeof(char) * MAX_STR_LEN);
  if (str == NULL)
  {
    print_error(s,"%s: out of memory", __progname);
    return TRUE;
  }
  
  // The first line should be the header. We don't need to chop it
  // as we're only comparing it to the length of the known header.
  fgets(str,MAX_STR_LEN,handle);
  if (strncmp(str,SSDEEPV1_HEADER,strlen(SSDEEPV1_HEADER)))
  {
    free(str);
    print_error(s,"%s: invalid file header", fn);
    return TRUE;
  }
  
  known_file_name = (TCHAR *)malloc(tchar_sz * MAX_STR_LEN);
  if (known_file_name == NULL)
    fatal_error("%s: Out of memory", __progname);

  known_hash = (char *)malloc(sizeof(char) * MAX_STR_LEN);
  if (NULL == known_hash)
    fatal_error("%s: Out of memory", __progname);

  while (fgets(str,MAX_STR_LEN,handle))
  {
    chop_line(str);

    /* The file format is:
         hash,filename 
    */

    strncpy(known_hash,str,MIN(MAX_STR_LEN,strlen(str)));
    find_comma_separated_string(known_hash,0);

    //    memset(known_file_name,0,tchar_sz * MAX_STR_LEN);
    find_comma_separated_string(str,1);

    size_t i, sz = strlen(str);
    for (i = 0 ; i < sz ; i++)
    {
#ifdef _WIN32
      known_file_name[i] = (TCHAR)(str[i]);
#else
      known_file_name[i] = str[i];
#endif
    }
    known_file_name[i] = 0;
    
    if (match_add(s,known_file_name,known_hash))
    {
      // If we can't insert this value, we're probably out of memory.
      // There's no sense trying to read the rest of the file.
      free(known_file_name);
      free(str);
      print_error(s,fn,"unable to insert hash");
      fclose(handle);
      return TRUE;
    }
  }

  free(known_file_name);
  free(str);
  fclose(handle);
  return FALSE;
}

