// Fuzzy Hashing by Jesse Kornblum
// Copyright (C) ManTech International Corporation 2010
//
// $Id$ 

/// @mainpage
/// This is the documentation for the fuzzy hashing API from ssdeep.
///
/// There is a complete function reference in fuzzy.h.
///
/// The most recent version of this documentation can be found
/// at http://ssdeep.sourceforge.net/. 
///
/// @copydoc fuzzy.h
///
/// @version 2.6
/// @date 28 Sep 2010
///
/// @author Jesse Kornblum, research {at jessekornblum dot} com. 

/// @file fuzzy.h
/// @brief
/// These functions allow a programmer to compute the fuzzy hashes
/// (also called the context-triggered piecewise hashes) of 
/// @link fuzzy_hash_buf() a buffer
/// of text @endlink, 
/// @link fuzzy_hash_filename() the contents of a file on the disk @endlink, 
/// and 
/// @link fuzzy_hash_file() the contents of
/// an open file handle @endlink . 
/// There is also a function to 
/// @link fuzzy_compare() compute the
/// similarity between any two fuzzy signatures @endlink.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _INTTYPES_H_
# include <inttypes.h>
#endif

#ifndef _FUZZY_H_
# define _FUZZY_H_


/// @brief Compute the fuzzy hash of a buffer
///
/// The computes the fuzzy hash of the first buf_len bytes of the buffer.
/// It is the caller's responsibility to append the filename,
/// if any, to result after computation. 
/// @param buf The data to be fuzzy hashed
/// @param buf_len The length of the data being hashed
/// @param result Where the fuzzy hash of buf is stored. This variable
/// must be allocated to hold at least FUZZY_MAX_RESULT bytes.
/// @return Returns zero on success, non-zero on error.
extern int fuzzy_hash_buf(const unsigned char *buf,
			  uint32_t      buf_len,
			  char          *result);


/// @brief Compute the fuzzy hash of a file using an open handle
///
/// Computes the fuzzy hash of the contents of the open file, starting
/// at the beginning of the file. When finished, the file pointer is
/// returned to its original position. If an error occurs, the file 
/// pointer's value is undefined.
/// It is the callers's responsibility to append the filename
/// to the result after computation.
/// @param handle Open handle to the file to be hashed
/// @param result Where the fuzzy hash of the file is stored. This 
/// variable must be allocated to hold at least FUZZY_MAX_RESULT bytes.
/// @return Returns zero on success, non-zero on error
extern int fuzzy_hash_file(FILE *handle,
			   char *result);

/// @brief Compute the fuzzy hash of a file
///
/// Opens, reads, and hashes the contents of the file 'filename' 
/// The result must be allocated to hold FUZZY_MAX_RESULT characters. 
/// It is the caller's responsibility to append the filename
/// to the result after computation. 
/// @param filename The file to be hashed
/// @param result Where the fuzzy hash of the file is stored. This 
/// variable must be allocated to hold at least FUZZY_MAX_RESULT bytes.
/// @return Returns zero on success, non-zero on error. 
extern int fuzzy_hash_filename(const char * filename,
			       char * result);



/// Computes the match score between two fuzzy hash signatures.
/// @return Returns a value from zero to 100 indicating the
/// match score of the 
/// two signatures. A match score of zero indicates the sigantures
/// did not match. When an error occurs, such as if one of the
/// inputs is NULL, returns -1.
extern int fuzzy_compare(const char *sig1, const char *sig2);



/// The longest possible length for a fuzzy hash signature (without the filename)
#define FUZZY_MAX_RESULT    (SPAMSUM_LENGTH + (SPAMSUM_LENGTH/2 + 20))

/// Length of an individual fuzzy hash signature component
#define SPAMSUM_LENGTH 64


// To end our 'extern "C" {'
#ifdef __cplusplus
} 
#endif


#endif   // ifndef _FUZZY_H_
