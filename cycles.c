
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

#include "main.h"

typedef struct dir_table {
  char *name;
  struct dir_table *next;
} dir_table;

dir_table *my_table = NULL;

#ifdef __DEBUG
void dump_table(void)
{
  struct dir_table *t = my_table;
  while (t != NULL)
  {
    printf ("* %s%s", t->name, NEWLINE);
    t = t->next;
  }
  printf ("-- end of table --\n");
}
#endif


int done_processing_dir(char *fn)
{
  dir_table *last, *temp;

  char *d_name = (char*)malloc(sizeof(char) * PATH_MAX);
  realpath(fn,d_name);

  if (my_table == NULL)
  {
    free(d_name);
    return FALSE;
  }

  temp = my_table;

  if (!strncmp(d_name,temp->name,PATH_MAX))
  {
    my_table = my_table->next;
    free(temp->name);
    free(temp);
    free(d_name);
    return TRUE;
  }

  while (temp->next != NULL)
  {
    last = temp;
    temp = temp->next;
    if (!strncmp(d_name,temp->name,PATH_MAX))
    {
      last->next = temp->next;
      free(temp->name);
      free(temp);
      free(d_name);
      return TRUE;
    }
  }

  free (d_name);
  return FALSE;
}




int processing_dir(char *fn)
{
  dir_table *new, *temp;
  char *d_name = (char*)malloc(sizeof(char) * PATH_MAX);
  realpath(fn,d_name);

  if (my_table == NULL)
  {
    my_table = (dir_table*)malloc(sizeof(dir_table));
    my_table->name = strdup(d_name);
    my_table->next = NULL;

    free(d_name);
    return TRUE;
  }
 
  temp = my_table;

  while (temp->next != NULL)
  {
    /* We should never be adding a directory that is already here */
    if (!strncmp(temp->name,d_name,PATH_MAX))
    {
      free(d_name);
      return FALSE;
    }
    temp = temp->next;       
  }

  new = (dir_table*)malloc(sizeof(dir_table));
  new->name = strdup(d_name);
  new->next = NULL;  
  temp->next = new;

  free(d_name);
  return TRUE;
}


int have_processed_dir(char *fn)
{
  dir_table *temp;
  char *d_name = (char*)malloc(sizeof(char) * PATH_MAX);
  realpath(fn,d_name);

  if (my_table == NULL)
  {
    free(d_name);
    return FALSE;
  }

  temp = my_table;
  while (temp != NULL)
  {
    if (!strncmp(temp->name,d_name,PATH_MAX))
    {
      free(d_name);
      return TRUE;
    }

    temp = temp->next;       
  }

  free(d_name);
  return FALSE;
}




