#ifndef __FILEDATA_H
#define __FILEDATA_H

/// Copyright (C) 2012 Kyrus.

// $Id$

#include <set>
#include <string>
#include <iostream>
#include "tchar-local.h"

class Filedata
{
 public:
 Filedata() : m_has_match_file(false) {}
  Filedata(const TCHAR * fn, const char * sum);
  Filedata(const std::string sig);

  std::string get_signature(void) const { return m_signature; }

  TCHAR * get_filename(void) const { return m_filename; }

  bool has_cluster(void) const { return (m_cluster != NULL); }
  void set_cluster(std::set<Filedata *> *c) { m_cluster = c; }
  std::set<Filedata* >* get_cluster(void) const { return m_cluster; }

  bool has_match_file(void) const { return m_has_match_file; }
  std::string get_match_file(void) const { return m_match_file; }

  // RBF - Should this be a TCHAR?
  void set_match_file(const std::string& fn);

 private:
  std::set<Filedata *> * m_cluster;

  /// Original signature in the form [blocksize]:[sig1]:[sig2]
  std::string m_signature;

  TCHAR * m_filename;

  /// File of hashes where we got this known file from, if any
  std::string m_match_file;
  bool m_has_match_file;
};

std::ostream& operator<<(std::ostream& o, const Filedata& f);





#endif  // ifndef __FILEDATA_H
