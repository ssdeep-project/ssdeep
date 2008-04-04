
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

// The longest line we should encounter when reading files of known hashes
#define MAX_STR_LEN  2048

int lsh_list_init(lsh_list *l)
{
  l->top    = NULL;
  l->bottom = NULL;
  return FALSE;
}

#define MAX(A,B)            (A>B)?A:B
#define STRINGS_EQUAL(A,B)  !strncmp(A,B,MAX(strlen(A),strlen(B)))

int match_compare(state *s, char *fn, char *sum)
{
  int status = FALSE;
  uint32_t score;
  lsh_node *tmp = s->known_hashes->top;

  while (tmp != NULL)
  {
    if (s->mode & mode_match_pretty)
    {
      // Prevent printing the redundant "A matches A"
      if (STRINGS_EQUAL(fn,tmp->fn) && STRINGS_EQUAL(sum,tmp->hash))
      {
	tmp = tmp->next;
	continue;
      }
    }

    score = spamsum_match(s,sum,tmp->hash);
    if (score > 0)
    {
      printf ("%s matches %s (%"PRIu32")%s", fn, tmp->fn, score, NEWLINE);

      // We don't return right away as this file could match more than
      // one signature. 
      status = TRUE;
    }

    tmp = tmp->next;
  }

  return status;
}


static int lsh_list_insert(state *s, lsh_list *l, char *fn, char *sum)
{
  lsh_node *new;

  if ((new = (lsh_node *)malloc(sizeof(lsh_node))) == NULL)
  {
    print_error(s,fn,"out of memory");
    return TRUE;
  }

  new->next = NULL;
  if (((new->hash = strdup(sum)) == NULL) ||
      ((new->fn   = strdup(fn))  == NULL))
  {
    print_error(s,fn,"out of memory");
    return TRUE;
  }

  if (l->bottom == NULL)
  {
    if (l->top != NULL)
      fatal_error(s,fn,"internal data structure inconsistency");

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
      printf ("\n");

    tmp = tmp->next;
  }

  return FALSE;
}


int match_add(state *s, char *fn, char *hash)
{
  return (lsh_list_insert(s,s->known_hashes,fn,hash));
}


int match_load(state *s, char *fn)
{
  unsigned char *str, *known_file_name;
  FILE *handle;

  if ((handle = fopen(fn,"rb")) == NULL)
  {
    if (!(s->mode & mode_silent))
      perror(fn);
    return TRUE;
  }

  str = (unsigned char *)malloc(sizeof(unsigned char) * MAX_STR_LEN);
  if (str == NULL)
  {
    print_error(s,fn, "out of memory");
    return TRUE;
  }
  
  // The first line should be the header. We don't need to chop it
  // as we're only comparing it to the length of the known header.
  fgets(str,MAX_STR_LEN,handle);
  if (strncmp(str,SSDEEPV1_HEADER,strlen(SSDEEPV1_HEADER)))
  {
    free(str);
    print_error(s,fn, "invalid file header");
    return TRUE;
  }
  
  known_file_name = (unsigned char *)malloc(sizeof(unsigned char)*MAX_STR_LEN);
  if (known_file_name == NULL)
  {
    free(str);
    print_error(s,fn, "out of memory");
    return TRUE;
  }

  while (fgets(str,MAX_STR_LEN,handle))
  {
    chop_line(str);

    strncpy(known_file_name,str,MAX_STR_LEN);

    // The file format is:  hash,filename
    find_comma_separated_string(str,0);
    find_comma_separated_string(known_file_name,1);

      //    if (lsh_list_insert(s,s->known_hashes,known_file_name,str))
    if (match_add(s,known_file_name,str))
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

