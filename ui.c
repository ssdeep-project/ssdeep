
/* $Id$ */

#include "ssdeep.h"
#include <stdarg.h>

void print_status(char *fmt, ...)
{
  va_list(ap);
  
  va_start(ap,fmt); 
  vprintf(fmt,ap); 
  va_end(ap); 
  
  printf ("%s", NEWLINE);
}


void print_error(state *s, char *fmt, ...)
{
  if (NULL == s)
    internal_error("%s: NULL state passed to print_error", __progname);

  if (s->mode & mode_silent)
    return;

  va_list(ap);
  
  va_start(ap,fmt); 
  vprintf(fmt,ap); 
  va_end(ap); 
  
  printf ("%s", NEWLINE);

}

#define MD5DEEP_PRINT_MSG(HANDLE,MSG) \
va_list(ap);  \
va_start(ap,MSG); \
if (vfprintf(HANDLE,MSG,ap) < 0)  \
{ \
   fprintf(stderr, "%s: %s", __progname, strerror(errno)); \
   exit(EXIT_FAILURE);  \
} \
va_end(ap); fprintf (HANDLE,"%s", NEWLINE);


void print_error_unicode(state *s, TCHAR *fn, char *fmt, ...)
{
  if (NULL == s)
    internal_error("%s: NULL state passed to print_error_unicode", __progname);

  if (!(s->mode & mode_silent))
    {
      display_filename(stderr,fn);
      fprintf(stderr,": ");
      MD5DEEP_PRINT_MSG(stderr,fmt);
    }
}



/* Internal errors are so serious that we ignore the user's wishes 
   about silent mode. Our need to debug the program outweighs their
   preferences. Besides, the program is probably crashing anyway... */
void internal_error(char *fmt, ... )
{
  MD5DEEP_PRINT_MSG(stderr,fmt);  
  print_status ("%s: Internal error. Contact developer!", __progname);  
  exit (EXIT_FAILURE);
}



void fatal_error(char *fmt, ... )
{
  va_list(ap);
  
  va_start(ap,fmt); 
  vprintf(fmt,ap); 
  va_end(ap); 
  
  printf ("%s", NEWLINE);
  exit (EXIT_FAILURE);
}


#ifdef _WIN32
void display_filename(FILE *out, TCHAR *fn)
{
  size_t pos,len;

  if (NULL == fn)
    return;

  len = _tcslen(fn);

  for (pos = 0 ; pos < len ; ++pos)
    {
      /* Windows can only display the English (00) code page
	 on the command line. */
      if (0 == (fn[pos] & 0xff00))
	_ftprintf(out, _TEXT("%c"), fn[pos]);
      else
	_ftprintf(out, _TEXT("?"));
    }
}
#else
void display_filename(FILE *out, TCHAR *fn)
{
  fprintf (out,"%s", fn);
}
#endif
