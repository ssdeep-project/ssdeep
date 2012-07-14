/// SSDEEP
/// Copyright (C) 2012 Kyrus.

// $Id$


#include "filedata.h"

Filedata::Filedata(const TCHAR *fn, const char * sum)
{
  m_has_match_file = false;

  m_filename = _tcsdup(fn);

  // RBF - SIGNATURE DOES NOT INCLUDE FILENAME
  // RBF - Is that a bad thing?
  m_signature = std::string(sum);
}


Filedata::Filedata(const std::string sig)
{
  size_t start, stop;

  m_has_match_file = false;
  m_cluster = NULL;
  m_signature = std::string(sig);

  // The filename should be immediately after the first comma and
  // enclosed in quotation marks.
  start = m_signature.find_first_of(",\"");
  if (std::string::npos == start)
  {
    // RBF - Error handling. Throw an exception
    throw (std::bad_alloc());
  }
  // Advance past the comma and quotation mark.
  start += 2;
  
  // Look for the second quotation mark, which should be at the end
  // of the string. 
  stop = m_signature.find('"', start);
  if (stop != m_signature.size() - 1)
  {
    // RBF - Error handling. Throw an exception
    throw (std::bad_alloc());
  }
  
  // Strip off the final quotation mark in the duplicate
  std::string tmp = m_signature.substr(start,(stop - start));
  
#ifndef _WIN32
  m_filename = strdup(tmp.c_str());
#else
  char * tmp2 = strdup(tmp.c_str());
  
  // On Win32 we have to do a kludgy cast from ordinary char 
  // values to the TCHAR values we use internally. Because we may have
  // reset the string length, get it again.
  // The extra +1 is for the terminating newline
  size_t i, sz = strlen(tmp2);
  m_filename = (TCHAR *)malloc(sizeof(TCHAR) * (sz + 1));
  if (NULL == m_filename)
  {
    // RBF - Error handling
    throw (std::bad_alloc());
  }
  for (i = 0 ; i < sz ; i++)
    m_filename[i] = (TCHAR)(tmp2[i]);
  m_filename[i] = 0;
#endif
}


void Filedata::set_match_file(const std::string& fn)
{
  m_has_match_file = true;
  m_match_file = std::string(fn);
}



std::ostream& operator<<(std::ostream& o, const Filedata& f)
{
  return o << f.get_signature();
}


