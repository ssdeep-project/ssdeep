
/* Fuzzy Hashing by Jesse Kornblum
   Copyright (C) ManTech International Corporation 2008

   $Id$ */


/* Compute the fuzzy hash of buf. The resulting block size is stored in 
   block_size. You must pass the length of buf in buf_len.
   result MUST be allocated to hold up to FUZZY_MAX_RESULT
   characters. It is the user's responsibility to append the filename,
   if any, to result after computation. */
extern int fuzzy_hash_buf(unsigned char *buf,
			  uint32_t      buf_len,
			  char          *result);


/* Works like the above, but hashes a file instead of a buffer.
   It is the user's responsibility to append the filename,
   if any, to result after computation. */
extern int fuzzy_hash_file(FILE *handle,
			   char *result);


/* Returns a value from 0 to 100 indicating the match score of the 
   two signatures. A match score of zero indicates the sigantures
   did not match. */
extern int fuzzy_compare(const char *sig1, const char *sig2);


/* -----------------------------------------------------------------
   You shouldn't have to mess with anything below this line         */


/* the output is a string of length 64 in base64 */
#define SPAMSUM_LENGTH 64
#define FUZZY_MAX_RESULT    (SPAMSUM_LENGTH + (SPAMSUM_LENGTH/2 + 20))
