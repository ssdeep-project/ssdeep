
// ssdeep
// Copyright (C) 2012 Kyrus
// Copyright (C) 2006 ManTech International Corporation
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 
// The code in this file, and this file only, is based on SpamSum, part 
// of the Samba project: 
//   http://www.samba.org/ftp/unpacked/junkcode/spamsum/
//
// Because of where this file came from, any program that contains it
// must be licensed under the terms of the General Public License (GPL).
// See the file COPYING for details. The author's original comments
// about licensing are below:
// 
//
//
// This is a checksum routine that is specifically designed for spam. 
// Copyright Andrew Tridgell <tridge@samba.org> 2002
//
// This code is released under the GNU General Public License version 2
// or later.  Alteratively, you may also use this code under the terms
// of the Perl Artistic license.
//
// If you wish to distribute this code under the terms of a different
// free software license then please ask me. If there is a good reason
// then I will probably say yes.
//

#include "main.h"
#include "fuzzy.h"


#define MIN_BLOCKSIZE  3
#define ROLLING_WINDOW 7

#define HASH_PRIME     0x01000193
#define HASH_INIT      0x28021967

// Our input buffer when reading files to hash
#define BUFFER_SIZE  8192

static struct {
  unsigned char window[ROLLING_WINDOW];
  uint32_t h1, h2, h3;
  uint32_t n;
} roll_state;


//
// A rolling hash, based on the Adler checksum. By using a rolling hash
// we can perform auto resynchronisation after inserts/deletes
//
// internally, h1 is the sum of the bytes in the window and h2
// is the sum of the bytes times the index
//
// h3 is a shift/xor based rolling hash, and is mostly needed to ensure that
// we can cope with large blocksize values
//
static inline uint32_t roll_hash(unsigned char c)
{
  roll_state.h2 -= roll_state.h1;
  roll_state.h2 += ROLLING_WINDOW * c;
  
  roll_state.h1 += c;
  roll_state.h1 -= roll_state.window[roll_state.n % ROLLING_WINDOW];
  
  roll_state.window[roll_state.n % ROLLING_WINDOW] = c;
  roll_state.n++;
  
  // The original spamsum AND'ed this value with 0xFFFFFFFF which
  // in theory should have no effect. This AND has been removed 
  // for performance (jk)
  roll_state.h3 = (roll_state.h3 << 5); //& 0xFFFFFFFF;
  roll_state.h3 ^= c;
  
  return roll_state.h1 + roll_state.h2 + roll_state.h3;
}

//
// reset the state of the rolling hash and return the initial rolling hash value
//
static uint32_t roll_reset(void)
{	
  memset(&roll_state, 0, sizeof(roll_state));
  return 0;
}

// a simple non-rolling hash, based on the FNV hash 
static inline uint32_t sum_hash(unsigned char c, uint32_t h)
{
  h *= HASH_PRIME;
  h ^= c;
  return h;
}

typedef struct _ss_context {
  char *ret, *p;
  // This is the file size, which should be uint64_t, but we
  // generally do not process files that large here.
  uint32_t total_chars;
  uint32_t h, h2, h3;
  uint32_t j, n, i, k;
  uint32_t block_size;
  char ret2[SPAMSUM_LENGTH/2 + 1];
} ss_context;


static void ss_destroy(ss_context *ctx)
{
  if (ctx->ret != NULL)
    free(ctx->ret);
}


static int ss_init(ss_context *ctx, FILE *handle)
{
  if (NULL == ctx)
    return TRUE;

  ctx->ret = (char *)malloc(sizeof(char) * FUZZY_MAX_RESULT);
  if (ctx->ret == NULL)
    return TRUE;

  if (handle != NULL)
    ctx->total_chars = find_file_size(handle);

  ctx->block_size = MIN_BLOCKSIZE;
  while (ctx->block_size * SPAMSUM_LENGTH < ctx->total_chars) {
    ctx->block_size = ctx->block_size * 2;
  }

  return FALSE;
}

static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void ss_engine(ss_context *ctx, 
		      const unsigned char *buffer, 
		      uint32_t buffer_size)
{
  uint32_t i;

  if (NULL == ctx || NULL == buffer)
    return;

  for ( i = 0 ; i < buffer_size ; ++i)
  {

    // 
    // at each character we update the rolling hash and
    // the normal hash. When the rolling hash hits the
    // reset value then we emit the normal hash as a
    // element of the signature and reset both hashes
    // 
    ctx->h  = roll_hash(buffer[i]);
    ctx->h2 = sum_hash(buffer[i], ctx->h2);
    ctx->h3 = sum_hash(buffer[i], ctx->h3);
    
    if (ctx->h % ctx->block_size == (ctx->block_size-1)) {
      // we have hit a reset point. We now emit a
      // hash which is based on all chacaters in the
      // piece of the message between the last reset
      // point and this one 
      ctx->p[ctx->j] = b64[ctx->h2 % 64];
      if (ctx->j < SPAMSUM_LENGTH-1) {
	// we can have a problem with the tail
	// overflowing. The easiest way to
	// cope with this is to only reset the
	// second hash if we have room for
	// more characters in our
	// signature. This has the effect of
	// combining the last few pieces of
	// the message into a single piece 

	ctx->h2 = HASH_INIT;
	(ctx->j)++;
      }
    }
    
    // this produces a second signature with a block size
    // of block_size*2. By producing dual signatures in
    // this way the effect of small changes in the message
    // size near a block size boundary is greatly reduced. 
    if (ctx->h % (ctx->block_size*2) == ((ctx->block_size*2)-1)) {
      ctx->ret2[ctx->k] = b64[ctx->h3 % 64];
      if (ctx->k < SPAMSUM_LENGTH/2-1) {
	ctx->h3 = HASH_INIT;
	(ctx->k)++;
      }
    }
  }
}

static int ss_update(ss_context *ctx, FILE *handle)
{
  uint32_t bytes_read;
  unsigned char *buffer; 

  if (NULL == ctx || NULL == handle)
    return TRUE;

  buffer = (unsigned char *)malloc(sizeof(unsigned char) * BUFFER_SIZE);
  if (buffer == NULL)
    return TRUE;

  snprintf(ctx->ret, 12, "%u:", ctx->block_size);
  ctx->p = ctx->ret + strlen(ctx->ret);
  
  memset(ctx->p, 0, SPAMSUM_LENGTH+1);
  memset(ctx->ret2, 0, sizeof(ctx->ret2));
  
  ctx->k  = ctx->j  = 0;
  ctx->h3 = ctx->h2 = HASH_INIT;
  ctx->h  = roll_reset();

  while ((bytes_read = fread(buffer,sizeof(unsigned char),BUFFER_SIZE,handle)) > 0)
  {
    ss_engine(ctx,buffer,bytes_read);
  }

  if (ctx->h != 0) 
  {
    ctx->p[ctx->j] = b64[ctx->h2 % 64];
    ctx->ret2[ctx->k] = b64[ctx->h3 % 64];
  }
  
  strcat(ctx->p+ctx->j, ":");
  strcat(ctx->p+ctx->j, ctx->ret2);

  free(buffer);
  return FALSE;
}


int fuzzy_hash_file(FILE *handle,
		    char *result)
{
  ss_context *ctx;  
  uint64_t filepos;
  int done = FALSE;
  
  if (NULL == handle || NULL == result)
    return TRUE;
  
  ctx = (ss_context *)malloc(sizeof(ss_context));
  if (ctx == NULL)
    return TRUE;

  filepos = ftello(handle);

  ss_init(ctx, handle);

  while (!done)
  {
    if (fseeko(handle,0,SEEK_SET))
      return TRUE;

    ss_update(ctx,handle);

    // our blocksize guess may have been way off - repeat if necessary
    if (ctx->block_size > MIN_BLOCKSIZE && ctx->j < SPAMSUM_LENGTH/2) 
      ctx->block_size = ctx->block_size / 2;
    else
      done = TRUE;
  }

  strncpy(result,ctx->ret,FUZZY_MAX_RESULT);

  ss_destroy(ctx);
  free(ctx);

  if (fseeko(handle,filepos,SEEK_SET))
    return TRUE;

  return FALSE;
}


extern int fuzzy_hash_filename(const char * filename,
			       char * result)
{
  int status;

  if (NULL == filename || NULL == result)
    return TRUE;

  FILE * handle = fopen(filename,"rb");
  if (NULL == handle)
    return TRUE;

  status = fuzzy_hash_file(handle,result);
  
  fclose(handle);

  return status;
}


int fuzzy_hash_buf(const unsigned char *buf,
		   uint32_t      buf_len,
		   char          *result)
{
  ss_context *ctx;
  int done = FALSE;

  if (NULL == buf || NULL == result)
    return TRUE;

  ctx = (ss_context *)malloc(sizeof(ss_context));  
  if (ctx == NULL)
    return TRUE;

  ctx->total_chars = buf_len;
  ss_init(ctx, NULL);


  while (!done)
  {
    snprintf(ctx->ret, 12, "%u:", ctx->block_size);
    ctx->p = ctx->ret + strlen(ctx->ret);
    
    memset(ctx->p, 0, SPAMSUM_LENGTH+1);
    memset(ctx->ret2, 0, sizeof(ctx->ret2));
    
    ctx->k  = ctx->j  = 0;
    ctx->h3 = ctx->h2 = HASH_INIT;
    ctx->h  = roll_reset();

    ss_engine(ctx,buf,buf_len);

    // our blocksize guess may have been way off - repeat if necessary 
    if (ctx->block_size > MIN_BLOCKSIZE && ctx->j < SPAMSUM_LENGTH/2) 
      ctx->block_size = ctx->block_size / 2;
    else
      done = TRUE;

    if (ctx->h != 0) 
      {
	ctx->p[ctx->j] = b64[ctx->h2 % 64];
	ctx->ret2[ctx->k] = b64[ctx->h3 % 64];
      }
    
    strcat(ctx->p+ctx->j, ":");
    strcat(ctx->p+ctx->j, ctx->ret2);
  }


  strncpy(result,ctx->ret,FUZZY_MAX_RESULT);

  ss_destroy(ctx);
  free(ctx);
  return FALSE;
}




//
// We only accept a match if we have at least one common substring in
// the signature of length ROLLING_WINDOW. This dramatically drops the
// false positive rate for low score thresholds while having
// negligable affect on the rate of spam detection.
//
// return 1 if the two strings do have a common substring, 0 otherwise
//
static int has_common_substring(const char *s1, const char *s2)
{
  int i, j;
  int num_hashes;
  uint32_t hashes[SPAMSUM_LENGTH];
  
  // there are many possible algorithms for common substring
  // detection. In this case I am re-using the rolling hash code
  // to act as a filter for possible substring matches 
  
  roll_reset();
  memset(hashes, 0, sizeof(hashes));
  
  // first compute the windowed rolling hash at each offset in
  // the first string 
  for (i=0;s1[i];i++) 
  {
    hashes[i] = roll_hash((unsigned char)s1[i]);
  }
  num_hashes = i;
  
  roll_reset();
  
  // now for each offset in the second string compute the
  // rolling hash and compare it to all of the rolling hashes
  // for the first string. If one matches then we have a
  // candidate substring match. We then confirm that match with
  // a direct string comparison */
  for (i=0;s2[i];i++) {
    uint32_t h = roll_hash((unsigned char)s2[i]);
    if (i < ROLLING_WINDOW-1) continue;
    for (j=ROLLING_WINDOW-1;j<num_hashes;j++) 
    {
      if (hashes[j] != 0 && hashes[j] == h) 
      {
	// we have a potential match - confirm it 
	if (strlen(s2+i-(ROLLING_WINDOW-1)) >= ROLLING_WINDOW && 
	    strncmp(s2+i-(ROLLING_WINDOW-1), 
		    s1+j-(ROLLING_WINDOW-1), 
		    ROLLING_WINDOW) == 0) 
	{
	  return 1;
	}
      }
    }
  }
  
  return 0;
}


// eliminate sequences of longer than 3 identical characters. These
// sequences contain very little information so they tend to just bias
// the result unfairly
static char *eliminate_sequences(const char *str)
{
  char *ret;
  int i, j, len;
  
  ret = strdup(str);
  if (!ret) 
    return NULL;
  
  len = strlen(str);
  
  for (i=j=3;i<len;i++) {
    if (str[i] != str[i-1] ||
	str[i] != str[i-2] ||
	str[i] != str[i-3]) {
      ret[j++] = str[i];
    }
  }
  
  ret[j] = 0;
  
  return ret;
}

//
// this is the low level string scoring algorithm. It takes two strings
// and scores them on a scale of 0-100 where 0 is a terrible match and
// 100 is a great match. The block_size is used to cope with very small
// messages.
//
static unsigned score_strings(const char *s1, const char *s2, uint32_t block_size)
{
  uint32_t score;
  uint32_t len1, len2;
  int edit_distn(const char *from, int from_len, const char *to, int to_len);
  
  len1 = strlen(s1);
  len2 = strlen(s2);
  
  if (len1 > SPAMSUM_LENGTH || len2 > SPAMSUM_LENGTH) {
    // not a real spamsum signature? 
    return 0;
  }
  
  // the two strings must have a common substring of length
  // ROLLING_WINDOW to be candidates 
  if (has_common_substring(s1, s2) == 0) {
    return 0;
  }
  
  // compute the edit distance between the two strings. The edit distance gives
  // us a pretty good idea of how closely related the two strings are 
  score = edit_distn(s1, len1, s2, len2);
 
  // scale the edit distance by the lengths of the two
  // strings. This changes the score to be a measure of the
  // proportion of the message that has changed rather than an
  // absolute quantity. It also copes with the variability of
  // the string lengths. 
  score = (score * SPAMSUM_LENGTH) / (len1 + len2);
  
  // at this stage the score occurs roughly on a 0-64 scale,
  // with 0 being a good match and 64 being a complete
  // mismatch
  
  // rescale to a 0-100 scale (friendlier to humans) 
  score = (100 * score) / 64;
  
  // it is possible to get a score above 100 here, but it is a
  // really terrible match 
  if (score >= 100) 
    return 0;
  
  // now re-scale on a 0-100 scale with 0 being a poor match and
  // 100 being a excellent match.
  score = 100 - score;

  //  printf ("len1: %"PRIu32"  len2: %"PRIu32"\n", len1, len2);
  
  // when the blocksize is small we don't want to exaggerate the match size 
  if (score > block_size/MIN_BLOCKSIZE * MIN(len1, len2)) {
    score = block_size/MIN_BLOCKSIZE * MIN(len1, len2);
  }
  return score;
}

//
// Given two spamsum strings return a value indicating the degree 
// to which they match.
//
int fuzzy_compare(const char *str1, const char *str2)
{
  uint32_t block_size1, block_size2;
  uint32_t score = 0;
  char *s1, *s2;
  char *s1_1, *s1_2;
  char *s2_1, *s2_2;
  
  if (NULL == str1 || NULL == str2)
    return -1;

  // each spamsum is prefixed by its block size
  if (sscanf(str1, "%u:", &block_size1) != 1 ||
      sscanf(str2, "%u:", &block_size2) != 1) {
    return -1;
  }
  
  // if the blocksizes don't match then we are comparing
  // apples to oranges. This isn't an 'error' per se. We could
  // have two valid signatures, but they can't be compared. 
  if (block_size1 != block_size2 && 
      block_size1 != block_size2*2 &&
      block_size2 != block_size1*2) {
    return 0;
  }
  
  // move past the prefix
  str1 = strchr(str1, ':');
  str2 = strchr(str2, ':');
  
  if (!str1 || !str2) {
    // badly formed ... 
    return -1;
  }
  
  // there is very little information content is sequences of
  // the same character like 'LLLLL'. Eliminate any sequences
  // longer than 3. This is especially important when combined
  // with the has_common_substring() test below. 
  s1 = eliminate_sequences(str1+1);
  s2 = eliminate_sequences(str2+1);
  
  if (!s1 || !s2) return 0;
  
  // now break them into the two pieces 
  s1_1 = s1;
  s2_1 = s2;
  
  s1_2 = strchr(s1, ':');
  s2_2 = strchr(s2, ':');
  
  if (!s1_2 || !s2_2) {
    // a signature is malformed - it doesn't have 2 parts 
    free(s1); free(s2);
    return 0;
  }

  *s1_2++ = 0;
  *s2_2++ = 0;
  
  // each signature has a string for two block sizes. We now
  // choose how to combine the two block sizes. We checked above
  // that they have at least one block size in common 
  if (block_size1 == block_size2) {
    uint32_t score1, score2;
    score1 = score_strings(s1_1, s2_1, block_size1);
    score2 = score_strings(s1_2, s2_2, block_size2);

    //    s->block_size = block_size1;

    score = MAX(score1, score2);
  } else if (block_size1 == block_size2*2) {

    score = score_strings(s1_1, s2_2, block_size1);
    //    s->block_size = block_size1;
  } else {

    score = score_strings(s1_2, s2_1, block_size2);
    //    s->block_size = block_size2;
  }
  
  free(s1);
  free(s2);
  
  return (int)score;
}






