#ifndef __MATCH_H
#define __MATCH_H

// SSDEEP
// $Id$
// Copyright (C) 2012 Kyrus.

#include "ssdeep.h"
#include "filedata.h"

// *********************************************************************
// Matching functions
// *********************************************************************

/// @brief Match the file f against the set of knowns
///
/// @return Returns false if there are no matches, true if at least one match
/// @param s State variable
/// @param f Filedata structure for the file.
bool match_compare(const state *s, const Filedata * f);

/// @brief Load a file of known hashes
///
/// @return Returns false on success, true on error
bool match_load(state *s, const char *fn);

/// @brief Add a single new hash to the set of known hashes
///
/// @return Returns false on success, true on error
bool match_add(state *s, Filedata * f);

/// Display all matches in the set of known hashes nicely
bool match_pretty(const state *s);

/// Load the known hashes from the file fn and compare them to the
/// set of known hashes
bool match_compare_unknown(const state *s, const char * fn);

/// Display the results of clustering operations
bool display_clusters(state *s);



#endif   // ifndef __MATCH_H
