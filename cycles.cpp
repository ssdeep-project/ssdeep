
/* MD5DEEP
 *
 * By Jesse Kornblum
 *
 * This is a work of the US Government. In accordance with 17 USC 105,
 * copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

// $Id$

#include "ssdeep.h"


typedef struct dir_table {
  TCHAR *name;
  struct dir_table *next;
} dir_table;

dir_table *my_table = NULL;


/* This function was used in the dark ages for debugging
static void dump_table(void)
{
  struct dir_table *t = my_table;
  while (t != NULL)
  {
    print_status (_TEXT("* %s"), t->name);
    t = t->next;
  }
  print_status ("-- end of table --");
}
*/



bool done_processing_dir(TCHAR *fn)
{
  dir_table *last, *temp;
  TCHAR *d_name = (TCHAR *)malloc(sizeof(TCHAR) * SSDEEP_PATH_MAX);
  if (!d_name)
    internal_error("%s: Out of memory", __progname);

#ifdef _WIN32
  _wfullpath(d_name,fn,SSDEEP_PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  if (my_table == NULL)
  {
    internal_error("Table is NULL in done_processing_dir");

    // This code never gets executed... 
    free(d_name);
    return false;
  }

  temp = my_table;

  if (!_tcsncmp(d_name,temp->name,SSDEEP_PATH_MAX))
  {
    my_table = my_table->next;
    free(temp->name);
    free(temp);
    free(d_name);
    return true;
  }

  while (temp->next != NULL)
  {
    last = temp;
    temp = temp->next;
    if (!_tcsncmp(d_name,temp->name,SSDEEP_PATH_MAX))
    {
      last->next = temp->next;
      free(temp->name);
      free(temp);
      free(d_name);
      return true;
    }
  }

  internal_error("%s: Directory %s not found in done_processing_dir",
		 __progname, d_name);

  // This code never gets executed... 
  //  free (d_name);
  return false;
}




bool processing_dir(TCHAR *fn)
{
  dir_table *new_dir, *temp;
  TCHAR *d_name = (TCHAR *)malloc(sizeof(TCHAR) * SSDEEP_PATH_MAX);
  if (!d_name)
    internal_error("%s: Out of memory", __progname);

#ifdef _WIN32
  _wfullpath(d_name,fn,SSDEEP_PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  if (my_table == NULL)
  {
    my_table = (dir_table*)malloc(sizeof(dir_table));
    if (!my_table)
      internal_error("%s: Out of memory", __progname);
    my_table->name = _tcsdup(d_name);
    my_table->next = NULL;

    free(d_name);
    return true;
  }
 
  temp = my_table;

  while (temp->next != NULL)
  {
    /* We should never be adding a directory that is already here */
    if (!_tcsncmp(temp->name,d_name,SSDEEP_PATH_MAX))
    {
      internal_error("%s: Attempt to add existing %s in processing_dir",
		     __progname, d_name);
      // Does not execute
      free(d_name);
      return false;
    }
    temp = temp->next;       
  }

  new_dir = (dir_table*)malloc(sizeof(dir_table));
  if (!new_dir)
    internal_error("%s: Out of memory", __progname);
  new_dir->name = _tcsdup(d_name);
  new_dir->next = NULL;  
  temp->next = new_dir;

  free(d_name);
  return true;
}


bool have_processed_dir(TCHAR *fn)
{
  dir_table *temp;
  TCHAR *d_name;

  if (my_table == NULL)
    return false;

  d_name = (TCHAR *)malloc(sizeof(TCHAR) * SSDEEP_PATH_MAX);
  if (!d_name)
    internal_error("%s: Out of memory", __progname);
#ifdef _WIN32
  _wfullpath(d_name,fn,SSDEEP_PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  temp = my_table;
  while (temp != NULL)
  {
    if (!_tcsncmp(temp->name,d_name,SSDEEP_PATH_MAX))
    {
      free(d_name);
      return true;
    }

    temp = temp->next;       
  }

  free(d_name);
  return false;
}




