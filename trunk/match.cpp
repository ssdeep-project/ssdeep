// ssdeep
// (C) Copyright 2012 Kyrus
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

#define MIN_SUBSTR_LEN 7

// ------------------------------------------------------------------
// SIGNATURE FILE FUNCTIONS
// ------------------------------------------------------------------

/// Open a file of known hashes and determine if it's valid
///
/// @param s State variable
/// @param fn filename to open
/// 
/// @return On success, returns the open file handle. On failure, returns NULL.
FILE * sig_file_open(state *s, const char * fn)
{
  if (NULL == s or NULL == fn)
    return NULL;

  FILE * handle = fopen(fn,"rb");
  if (NULL == handle)
  {
    if ( ! (MODE(mode_silent)) )
      perror(fn);
    return NULL;
  }

  // The first line of the file should contain a valid ssdeep header. 
  char buffer[MAX_STR_LEN];
  if (NULL == fgets(buffer,MAX_STR_LEN,handle))
  {
    if ( ! (MODE(mode_silent)) )
      perror(fn);
    fclose(handle);
    return NULL;
  }

  chop_line(buffer);

  if (strncmp(buffer,SSDEEPV1_0_HEADER,MAX_STR_LEN) and 
      strncmp(buffer,SSDEEPV1_1_HEADER,MAX_STR_LEN)) 
  {
    if ( ! (MODE(mode_silent)) )
      print_error(s,"%s: Invalid file header.", fn);
    fclose(handle);
    return NULL;
  }

  return handle;
}


/// Convert a line of text from a file of known hashes into a filedata structure
///
/// @param s State variable
/// @param buffer Buffer of text to parse
/// @param f Where to store the converted data
///
/// @return Returns true on error, false on success.
bool str_to_filedata(state *s, const char * buffer, filedata_t *f)
{
  if (NULL == s or NULL == buffer or NULL == f)
    return true;

  // We do the id number first so that we always advance it, just in case.
  // This code which updates the match_id is NOT THREAD SAFE!
  f->id = s->next_match_id;
  s->next_match_id++;

  f->signature = std::string(buffer);

  // Find the blocksize
  size_t found;
  found = f->signature.find(':');
  if (found == std::string::npos)
    return true;
  f->blocksize = (uint64_t)atoll(f->signature.substr(0,found).c_str());

  // Find the two signature components s1 and s2
  size_t start = found + 1;
  found = f->signature.find(":",found+1);
  if (found == std::string::npos)
    return true;

  f->s1 = f->signature.substr(start,found - start);

  start = found+1;
  found = f->signature.find(",",found+1);
  if (found == std::string::npos)
    return true;

  f->s2 = f->signature.substr(start, found - start);

  // Remove quotes from the ends of strings, if present
  std::string tmp = f->signature.substr(found+1);
  if (tmp[0] == '"')
  {
    // We assume quoted filenames are quoted at both ends,
    // but check just to make sure.
    tmp.erase(0,1);
    if (tmp[tmp.size()-1] == '"')
      tmp.erase(tmp.size()-1,1);
  }

#ifndef _WIN32
  f->filename = strdup(tmp.c_str());
  remove_escaped_quotes(f->filename);
#else  
  char * tmp2 = strdup(tmp.c_str());
  remove_escaped_quotes(tmp2);
  // On Win32 we have to do a kludgy cast from ordinary char 
  // values to the TCHAR values we use internally. Because we may have
  // reset the string length, get it again.
  // The extra +1 is for the terminating newline
  size_t i, sz = strlen(tmp2);
  f->filename = (TCHAR *)malloc(sizeof(TCHAR) * (sz + 1));
  if (NULL == f->filename)
    return true;
  for (i = 0 ; i < sz ; i++)
    f->filename[i] = (TCHAR)(tmp2[i]);
  f->filename[i] = 0;
#endif

  return false;
}


/// @brief Read the next entry in a file of known hashes and convert 
/// it to a filedata structure
///
/// @param s State variable
/// @param handle File handle to read from. 
/// Should have previously been opened by sig_file_open()
/// @param fn Filename of known hashes
/// @param f Structure where to store the data we read
///
/// @return Returns true if there is no entry to read or on error. 
/// Otherwise, false.
bool sig_file_next(state *s, FILE * handle, const char * fn, filedata_t * f)
{
  if (NULL == s or NULL == fn or NULL == f or NULL == handle)
    return true;

  char buffer[MAX_STR_LEN];
  memset(buffer,0,MAX_STR_LEN);
  if (NULL == fgets(buffer,MAX_STR_LEN,handle))
    return true;

  chop_line(buffer);

  f->match_file = std::string(fn);

  return str_to_filedata(s,buffer,f);
}


bool sig_file_close(FILE * handle)
{
  if (handle != NULL) 
    fclose(handle);
  
  return false;
}



// ------------------------------------------------------------------
// MATCHING FUNCTIONS
// ------------------------------------------------------------------

void handle_match(state *s, 
		  const TCHAR * fn, 
		  const char * match_file, 
		  filedata_t * match, 
		  int score)
{
  if (s->mode & mode_csv)
  {
    printf("\"");
    display_filename(stdout,fn,TRUE);
    printf("\",\"");
    display_filename(stdout,match->filename,TRUE);
    print_status("\",%u", score);
  }
  else
  {
    // The match file names may be empty. If so, we don't print them
    // or the colon which separates them from the filename
    if (strlen(match_file) > 0)
      printf ("%s:", match_file);
    display_filename(stdout,fn,FALSE);
    printf(" matches ");
    if (strlen(match->match_file.c_str()) > 0)
      printf ("%s:", match->match_file.c_str());
    display_filename(stdout,match->filename,FALSE);
    print_status(" (%u)", score);
  }
}


bool match_compare(state *s, 
		   const char * match_file, 
		   const TCHAR *fn, 
		   const char *sum)
{
  if (NULL == s or NULL == fn or NULL == sum)
    fatal_error("%s: Null values passed into match_compare", __progname);

  bool status = false;
  
  std::string sig = std::string(sum);
  size_t fn_len = _tcslen(fn);
  size_t sum_len = strlen(sum);

  std::vector<filedata_t *>::const_iterator match_it;
  for (match_it = s->all_files.begin() ; 
       match_it != s->all_files.end() ; 
       ++match_it)
  {
    // When in pretty mode, we still want to avoid printing
    // A matches A (100).
    if (s->mode & mode_match_pretty)
    {
      if (!(_tcsncmp(fn,
		     (*match_it)->filename,
		     std::max(fn_len,_tcslen((*match_it)->filename)))) and
	  !(strncmp(sum,
		    (*match_it)->signature.c_str(),
		    std::max(sum_len,(*match_it)->signature.length()))))
      {
	// Unless these results from different matching files (such as
	// what happens in sigcompare mode). That being said, we have to
	// be careful to avoid NULL values such as when working in 
	// normal pretty print mode.
	if (NULL == match_file or 
	    0 == (*match_it)->match_file.length() or
	    (!(strncmp(match_file, 
		       (*match_it)->match_file.c_str(), 
		       std::max(strlen(match_file),(*match_it)->match_file.length())))))
	  continue;
      }
    }

    int score =  fuzzy_compare(sum, (*match_it)->signature.c_str());
    if (-1 == score)
      print_error(s, "%s: Bad hashes in comparison", __progname);
    else
    {
      if (score > s->threshold or MODE(mode_display_all))
      {
	handle_match(s,fn,match_file,(*match_it),score);
	status = true;
      }
    }
  }
  
  return status;
}
  

bool match_pretty(state *s)
{
  if (NULL == s)
    return true;

  // Walk the vector which contains all of the known files
  std::vector<filedata_t *>::iterator it;
  for (it = s->all_files.begin() ; it != s->all_files.end() ; ++it)
  {
    if (match_compare(s,
		      (*it)->match_file.c_str(),
		      (*it)->filename,
		      (*it)->signature.c_str()))
      print_status("");
  }

  return false;
}


/// Add a file to the set of known files
///
/// @param s State variable
/// @param f File data for the file to add
bool add_known_file(state *s, filedata_t *f)
{
  s->all_files.push_back(f);

  return false;
}


bool match_add(state *s, 
	       const char  * match_file, 
	       const TCHAR * fn, 
	       const char  * hash)
{
  filedata_t * f = new filedata_t;

  str_to_filedata(s,hash,f);
  f->filename = _tcsdup(fn);
  f->match_file = std::string(match_file);

  add_known_file(s,f);

  return false;
}


bool match_load(state *s, char *fn)
{
  if (NULL == s or NULL == fn)
    return true;

  bool status = false;
  FILE * handle = sig_file_open(s,fn);
  if (NULL == handle)
    return true;

  filedata_t * f = new filedata_t;
  while ( ! sig_file_next(s,handle,fn,f) )
  {
    if (add_known_file(s,f))
    {
      print_error(s,"%s: unable to insert hash", fn);
      status = true;
      break;
    }

    f = new filedata_t;
  }

  sig_file_close(handle);

  return status;
}


bool match_compare_unknown(state *s, const char * fn)
{ 
  if (NULL == s or NULL == fn)
    return true;

  FILE * handle = sig_file_open(s,fn);
  if (NULL == handle)
    return true;

  filedata_t f;
  
  while ( ! sig_file_next(s,handle,fn,&f))
    match_compare(s,fn,f.filename,f.signature.c_str());

  sig_file_close(handle);

  return FALSE;
}

