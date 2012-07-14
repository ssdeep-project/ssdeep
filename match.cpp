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


#include "match.h"

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
FILE * sig_file_open(const state *s, const char * fn)
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



/// @brief Read the next entry in a file of known hashes and convert 
/// it to a filedata structure
///
/// @param s State variable
/// @param fn Filename of known hashes
/// @param handle File handle to read from. 
/// Should have previously been opened by sig_file_open()

/// @param f Structure where to store the data we read
///
/// @return Returns true if there is no entry to read or on error. 
/// Otherwise, false.
bool sig_file_next(const state *s, 
		   const char * fn,
		   FILE * handle,
		   Filedata ** f)
{
  if (NULL == s or NULL == fn or NULL == handle)
    return true;

  char buffer[MAX_STR_LEN];
  memset(buffer,0,MAX_STR_LEN);
  if (NULL == fgets(buffer,MAX_STR_LEN,handle))
    return true;

  chop_line(buffer);

  // RBF - Catch exceptions
  *f = new Filedata(std::string(buffer));
  (*f)->set_match_file(std::string(fn));

  return false;
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

//void handle_cluster(state *s,

// RBF - Move to top of file		
#include <iostream>
using std::cout;
using std::endl;    

void handle_match(const state *s, const Filedata *a, const Filedata *b, int score)
{
  if (s->mode & mode_csv)
  {
    printf("\"");
    display_filename(stdout,a->get_filename(),TRUE);
    printf("\",\"");
    display_filename(stdout,b->get_filename(),TRUE);
    print_status("\",%u", score);
  }
  if (s->mode & mode_cluster)
  {
    // RBF - Handle clustering
  }
  else
  {
    // The match file names may be empty. If so, we don't print them
    // or the colon which separates them from the filename
    if (a->has_match_file())
      cout << a->get_match_file() << ":";
    display_filename(stdout,a->get_filename(),FALSE);
    cout << " matches "; 
    if (b->has_match_file())
      cout << b->get_match_file() << ":";
    display_filename(stdout,b->get_filename(),FALSE);
    print_status(" (%u)", score);
  }
}


bool match_compare(const state *s, const Filedata * f)
{
  if (NULL == s)
    fatal_error("%s: Null state passed into match_compare", __progname);

  bool status = false;  
  size_t fn_len = _tcslen(f->get_filename());

  std::vector<Filedata* >::const_iterator it;
  for (it = s->all_files.begin() ; it != s->all_files.end() ; ++it)
  {
    // When in pretty mode, we still want to avoid printing
    // A matches A (100).
    if (s->mode & mode_match_pretty)
    {
      if (!(_tcsncmp(f->get_filename(),
		     (*it)->get_filename(),
		     std::max(fn_len,_tcslen((*it)->get_filename())))) and
	  (f->get_signature() != (*it)->get_signature()))
      {
	// Unless these results from different matching files (such as
	// what happens in sigcompare mode). That being said, we have to
	// be careful to avoid NULL values such as when working in 
	// normal pretty print mode.
	if (not(f->has_match_file()) or 
	    f->get_match_file() == (*it)->get_match_file())
	  continue;
      }
    }

    /*
    cout << "Comparing " << 
      f->get_signature() << " and " << (*it)->get_signature() << endl;
    */

    int score =  fuzzy_compare(f->get_signature().c_str(), 
			       (*it)->get_signature().c_str());
    if (-1 == score)
      print_error(s, "%s: Bad hashes in comparison", __progname);
    else
    {
      if (score > s->threshold or MODE(mode_display_all))
      {
	handle_match(s,f,(*it),score);
	status = true;
      }
    }
  }
  
  return status;
}
  

bool match_pretty(const state *s)
{
  if (NULL == s)
    return true;

  // Walk the vector which contains all of the known files
  std::vector<Filedata *>::const_iterator it;
  for (it = s->all_files.begin() ; it != s->all_files.end() ; ++it)
  {
    if (match_compare(s,*it))
      print_status("");
  }

  return false;
}


bool match_add(state *s, Filedata * f)
{
  if (NULL == s)
    return true;

  s->all_files.push_back(f);

  return false;
}


bool match_load(state *s, const char *fn)
{
  if (NULL == s or NULL == fn)
    return true;

  bool status, ret = false;

  FILE * handle = sig_file_open(s,fn);
  if (NULL == handle)
    return true;

  do 
  {
    Filedata * f; 
    // RBF - Catch exceptions?
    status = sig_file_next(s,fn,handle,&f);
    if (not status)
    {
      if (match_add(s,f))
      {
	print_error(s,"%s: unable to insert hash", fn);
	ret = true;
	break;
      }
    }
  } while (not status);

  sig_file_close(handle);

  // RBF _ WHat do we return?
  return false;
}


bool match_compare_unknown(const state *s, const char * fn)
{ 
  if (NULL == s or NULL == fn)
    return true;

  FILE * handle = sig_file_open(s,fn);
  if (NULL == handle)
    return true;

  bool status;
  
  do
  {
    Filedata *f;
    status = sig_file_next(s,fn,handle,&f);
    if (not status)
      match_compare(s,f);
  } while (not status);

  sig_file_close(handle);

  return false;
}

