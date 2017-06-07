/* ssdeep
 * Copyright (C) 2002 Andrew Tridgell <tridge@samba.org>
 * Copyright (C) 2006 ManTech International Corporation
 * Copyright (C) 2013 Helmut Grohne <helmut@subdivi.de>
 * Copyright (C) 2017 Tsukasa OI <floss_ssdeep@irq.a4lg.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Earlier versions of this code were named fuzzy.c and can be found at:
 *     http://www.samba.org/ftp/unpacked/junkcode/spamsum/
 *     http://ssdeep.sf.net/
 */

#include "main.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzy.h"
#include "edit_dist.h"

#if defined(__GNUC__) && __GNUC__ >= 3
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#define ROLLING_WINDOW 7
#define MIN_BLOCKSIZE 3
#define HASH_INIT 0x27
#define NUM_BLOCKHASHES 31

// Enable bit-parallel string processing only if bit-parallel algorithms
// are enabled and considered to be efficient.
#if !defined(SSDEEP_DISABLE_POSITION_ARRAY) || !SSDEEP_DISABLE_POSITION_ARRAY
#if SPAMSUM_LENGTH <= 64 && CHAR_MIN >= -256 && CHAR_MAX <= 256 && (CHAR_MAX - CHAR_MIN + 1) <= 256
#define SSDEEP_ENABLE_POSITION_ARRAY
#endif
#endif

struct roll_state
{
  unsigned char window[ROLLING_WINDOW];
  uint32_t h1, h2, h3;
  uint32_t n;
};

static void roll_init(/*@out@*/ struct roll_state *self)
{
  memset(self, 0, sizeof(struct roll_state));
}

/*
 * a rolling hash, based on the Adler checksum. By using a rolling hash
 * we can perform auto resynchronisation after inserts/deletes

 * internally, h1 is the sum of the bytes in the window and h2
 * is the sum of the bytes times the index

 * h3 is a shift/xor based rolling hash, and is mostly needed to ensure that
 * we can cope with large blocksize values
 */
static void roll_hash(struct roll_state *self, unsigned char c)
{
  self->h2 -= self->h1;
  self->h2 += ROLLING_WINDOW * (uint32_t)c;

  self->h1 += (uint32_t)c;
  self->h1 -= (uint32_t)self->window[self->n];

  self->window[self->n] = c;
  self->n++;
  if (self->n == ROLLING_WINDOW)
    self->n = 0;

  /* The original spamsum AND'ed this value with 0xFFFFFFFF which
   * in theory should have no effect. This AND has been removed
   * for performance (jk) */
  self->h3 <<= 5;
  self->h3 ^= c;
}

static uint32_t roll_sum(const struct roll_state *self)
{
  return self->h1 + self->h2 + self->h3;
}

/* A simple non-rolling hash, based on the FNV hash. */
#include "sum_table.h"
static unsigned char sum_hash(unsigned char c, unsigned char h)
{
  return sum_table[h][c & 0x3f];
}

/* A blockhash contains a signature state for a specific (implicit) blocksize.
 * The blocksize is given by SSDEEP_BS(index). The h and halfh members are the
 * partial FNV hashes, where halfh stops to be reset after digest is
 * SPAMSUM_LENGTH/2 long. The halfh hash is needed be able to truncate digest
 * for the second output hash to stay compatible with ssdeep output. */
struct blockhash_context
{
  unsigned int dindex;
  char digest[SPAMSUM_LENGTH];
  char halfdigest;
  char h, halfh;
};

struct fuzzy_state
{
  uint_least64_t total_size;
  uint_least64_t fixed_size;
  uint_least64_t reduce_border;
  unsigned int bhstart, bhend, bhendlimit;
  unsigned int flags;
  uint32_t rollmask;
  struct blockhash_context bh[NUM_BLOCKHASHES];
  struct roll_state roll;
  unsigned char lasth;
};

#define FUZZY_STATE_NEED_LASTHASH  1u
#define FUZZY_STATE_SIZE_FIXED     2u

#define SSDEEP_BS(index) (((uint32_t)MIN_BLOCKSIZE) << (index))
#define SSDEEP_TOTAL_SIZE_MAX \
  ((uint_least64_t)SSDEEP_BS(NUM_BLOCKHASHES-1) * SPAMSUM_LENGTH)

/*@only@*/ /*@null@*/ struct fuzzy_state *fuzzy_new(void)
{
  struct fuzzy_state *self;
  if(NULL == (self = malloc(sizeof(struct fuzzy_state))))
    /* malloc sets ENOMEM */
    return NULL;
  self->bhstart = 0;
  self->bhend = 1;
  self->bhendlimit = NUM_BLOCKHASHES - 1;
  self->bh[0].h = HASH_INIT;
  self->bh[0].halfh = HASH_INIT;
  self->bh[0].digest[0] = '\0';
  self->bh[0].halfdigest = '\0';
  self->bh[0].dindex = 0;
  self->total_size = 0;
  self->reduce_border = (uint_least64_t)MIN_BLOCKSIZE * SPAMSUM_LENGTH;
  self->flags = 0;
  self->rollmask = 0;
  roll_init(&self->roll);
  return self;
}

/*@only@*/ /*@null@*/ struct fuzzy_state *fuzzy_clone(const struct fuzzy_state *state)
{
  struct fuzzy_state *newstate;
  if (NULL == (newstate = malloc(sizeof(struct fuzzy_state))))
    /* malloc sets ENOMEM */
    return NULL;
  memcpy(newstate, state, sizeof(struct fuzzy_state));
  return newstate;
}

#ifdef S_SPLINT_S
extern const int EOVERFLOW;
#endif

int fuzzy_set_total_input_length(struct fuzzy_state *state, uint_least64_t total_fixed_length)
{
  unsigned int bi = 0;
  if (total_fixed_length > SSDEEP_TOTAL_SIZE_MAX)
  {
    errno = EOVERFLOW;
    return -1;
  }
  if ((state->flags & FUZZY_STATE_SIZE_FIXED) &&
      state->fixed_size != total_fixed_length)
  {
    errno = EINVAL;
    return -1;
  }
  state->flags |= FUZZY_STATE_SIZE_FIXED;
  state->fixed_size = total_fixed_length;
  while ((uint_least64_t)SSDEEP_BS(bi) * SPAMSUM_LENGTH < total_fixed_length)
  {
    ++bi;
    if (bi == NUM_BLOCKHASHES - 2)
      break;
  }
  ++bi;
  state->bhendlimit = bi;
  return 0;
}


static void fuzzy_try_fork_blockhash(struct fuzzy_state *self)
{
  struct blockhash_context *obh, *nbh;
  assert(self->bhend > 0);
  obh = self->bh + (self->bhend - 1);
  if (self->bhend <= self->bhendlimit)
  {
    nbh = obh + 1;
    nbh->h = obh->h;
    nbh->halfh = obh->halfh;
    nbh->digest[0] = '\0';
    nbh->halfdigest = '\0';
    nbh->dindex = 0;
    ++self->bhend;
  }
  else if (self->bhend == NUM_BLOCKHASHES &&
	   !(self->flags & FUZZY_STATE_NEED_LASTHASH))
  {
    self->flags |= FUZZY_STATE_NEED_LASTHASH;
    self->lasth = obh->h;
  }
}

static void fuzzy_try_reduce_blockhash(struct fuzzy_state *self)
{
  assert(self->bhstart < self->bhend);
  if (self->bhend - self->bhstart < 2)
    /* Need at least two working hashes. */
    return;
  if (self->reduce_border >= ((self->flags & FUZZY_STATE_SIZE_FIXED) ? self->fixed_size : self->total_size))
    /* Initial blocksize estimate would select this or a smaller
     * blocksize. */
    return;
  if (self->bh[self->bhstart + 1].dindex < SPAMSUM_LENGTH / 2)
    /* Estimate adjustment would select this blocksize. */
    return;
  /* At this point we are clearly no longer interested in the
   * start_blocksize. Get rid of it. */
  ++self->bhstart;
  self->reduce_border *= 2;
  self->rollmask = self->rollmask * 2 + 1;
}

static const char *b64 =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void fuzzy_engine_step(struct fuzzy_state *self, unsigned char c)
{
  uint32_t horg, h;
  unsigned int i;
  /* At each character we update the rolling hash and the normal hashes.
   * When the rolling hash hits a reset value then we emit a normal hash
   * as a element of the signature and reset the normal hash. */
  roll_hash(&self->roll, c);
  horg = roll_sum(&self->roll) + 1;
  h = horg / (uint32_t)MIN_BLOCKSIZE;

  for (i = self->bhstart; i < self->bhend; ++i)
  {
    self->bh[i].h = sum_hash(c, self->bh[i].h);
    self->bh[i].halfh = sum_hash(c, self->bh[i].halfh);
  }
  if (self->flags & FUZZY_STATE_NEED_LASTHASH)
    self->lasth = sum_hash(c, self->lasth);

  /* 0xffffffff !== -1 (mod 3) */
  if (!horg)
    return;
  /* With growing blocksize almost no runs fail the next test. */
  if (likely(h & self->rollmask))
    return;
  /* Delay computation of modulo as possible. */
  if (horg % (uint32_t)MIN_BLOCKSIZE)
    return;
  h >>= self->bhstart;

  i = self->bhstart;
  do
  {
    /* We have hit a reset point. We now emit hashes which are
     * based on all characters in the piece of the message between
     * the last reset point and this one */
    if (unlikely(0 == self->bh[i].dindex)) {
      /* Can only happen 30 times. */
      /* First step for this blocksize. Clone next. */
      fuzzy_try_fork_blockhash(self);
    }
    self->bh[i].digest[self->bh[i].dindex] =
      b64[self->bh[i].h];
    self->bh[i].halfdigest = b64[self->bh[i].halfh];
    if (self->bh[i].dindex < SPAMSUM_LENGTH - 1) {
      /* We can have a problem with the tail overflowing. The
       * easiest way to cope with this is to only reset the
       * normal hash if we have room for more characters in
       * our signature. This has the effect of combining the
       * last few pieces of the message into a single piece
       * */
      self->bh[i].digest[++(self->bh[i].dindex)] = '\0';
      self->bh[i].h = HASH_INIT;
      if (self->bh[i].dindex < SPAMSUM_LENGTH / 2) {
	self->bh[i].halfh = HASH_INIT;
	self->bh[i].halfdigest = '\0';
      }
    }
    else
      fuzzy_try_reduce_blockhash(self);
    if (h & 1)
      break;
    h >>= 1;
  } while (++i < self->bhend);
}

int fuzzy_update(struct fuzzy_state *self,
		 const unsigned char *buffer,
		 size_t buffer_size)
{
  if (unlikely(buffer_size > SSDEEP_TOTAL_SIZE_MAX ||
      SSDEEP_TOTAL_SIZE_MAX - buffer_size < self->total_size )) {
    self->total_size = SSDEEP_TOTAL_SIZE_MAX + 1;
  }
  else
    self->total_size += buffer_size;
  for ( ;buffer_size > 0; ++buffer, --buffer_size)
    fuzzy_engine_step(self, *buffer);
  return 0;
}

static int memcpy_eliminate_sequences(char *dst,
				      const char *src,
				      int n)
{
  const char *srcend = src + n;
  assert(n >= 0);
  if (src < srcend) *dst++ = *src++;
  if (src < srcend) *dst++ = *src++;
  if (src < srcend) *dst++ = *src++;
  while (src < srcend)
  {
    if (*src == dst[-1] && *src == dst[-2] && *src == dst[-3])
    {
      ++src;
      --n;
    }
    else
      *dst++ = *src++;
  }
  return n;
}

int fuzzy_digest(const struct fuzzy_state *self,
		 /*@out@*/ char *result,
		 unsigned int flags)
{
  unsigned int bi = self->bhstart;
  uint32_t h = roll_sum(&self->roll);
  int i, remain = FUZZY_MAX_RESULT - 1; /* Exclude terminating '\0'. */
  /* Verify that our elimination was not overeager. */
  assert(bi == 0 || (uint_least64_t)SSDEEP_BS(bi) / 2 * SPAMSUM_LENGTH <
	 self->total_size);

  if (self->total_size > SSDEEP_TOTAL_SIZE_MAX) {
    /* The input exceeds data types. */
    errno = EOVERFLOW;
    return -1;
  }
  /* Fixed size optimization. */
  if ((self->flags & FUZZY_STATE_SIZE_FIXED) &&
      self->fixed_size != self->total_size) {
    errno = EINVAL;
    return -1;
  }
  /* Initial blocksize guess. */
  while ((uint_least64_t)SSDEEP_BS(bi) * SPAMSUM_LENGTH < self->total_size)
    ++bi;
  /* Adapt blocksize guess to actual digest length. */
  if (bi >= self->bhend)
    bi = self->bhend - 1;
  while (bi > self->bhstart && self->bh[bi].dindex < SPAMSUM_LENGTH / 2)
    --bi;
  assert (!(bi > 0 && self->bh[bi].dindex < SPAMSUM_LENGTH / 2));

  i = snprintf(result, (size_t)remain, "%lu:", (unsigned long)SSDEEP_BS(bi));
  if (i <= 0)
    /* Maybe snprintf has set errno here? */
    return -1;
  assert(i < remain);
  remain -= i;
  result += i;
  i = (int)self->bh[bi].dindex;
  assert(i <= remain);
  if ((flags & FUZZY_FLAG_ELIMSEQ) != 0)
    i = memcpy_eliminate_sequences(result, self->bh[bi].digest, i);
  else
    memcpy(result, self->bh[bi].digest, (size_t)i);
  result += i;
  remain -= i;
  if (h != 0)
  {
    assert(remain > 0);
    *result = b64[self->bh[bi].h];
    if((flags & FUZZY_FLAG_ELIMSEQ) == 0 || i < 3 ||
       *result != result[-1] ||
       *result != result[-2] ||
       *result != result[-3]) {
      ++result;
      --remain;
    }
  }
  else if (self->bh[bi].digest[self->bh[bi].dindex] != '\0') {
    assert(remain > 0);
    *result = self->bh[bi].digest[self->bh[bi].dindex];
    if((flags & FUZZY_FLAG_ELIMSEQ) == 0 || i < 3 ||
       *result != result[-1] ||
       *result != result[-2] ||
       *result != result[-3]) {
      ++result;
      --remain;
    }
  }
  assert(remain > 0);
  *result++ = ':';
  --remain;
  if (bi < self->bhend - 1)
  {
    ++bi;
    i = (int)self->bh[bi].dindex;
    if ((flags & FUZZY_FLAG_NOTRUNC) == 0 &&
	i > SPAMSUM_LENGTH / 2 - 1)
      i = SPAMSUM_LENGTH / 2 - 1;
    assert(i <= remain);
    if ((flags & FUZZY_FLAG_ELIMSEQ) != 0)
      i = memcpy_eliminate_sequences(result,
				     self->bh[bi].digest, i);
    else
      memcpy(result, self->bh[bi].digest, (size_t)i);
    result += i;
    remain -= i;
    if (h != 0) {
      assert(remain > 0);
      h = (flags & FUZZY_FLAG_NOTRUNC) != 0 ? self->bh[bi].h :
	self->bh[bi].halfh;
      *result = b64[h];
      if ((flags & FUZZY_FLAG_ELIMSEQ) == 0 || i < 3 ||
	  *result != result[-1] ||
	  *result != result[-2] ||
	  *result != result[-3])
      {
	++result;
	--remain;
      }
    }
    else {
      i = (flags & FUZZY_FLAG_NOTRUNC) != 0 ?
	  self->bh[bi].digest[self->bh[bi].dindex] : self->bh[bi].halfdigest;
      if (i != '\0') {
	assert(remain > 0);
	*result = i;
	if ((flags & FUZZY_FLAG_ELIMSEQ) == 0 || i < 3 ||
	    *result != result[-1] ||
	    *result != result[-2] ||
	    *result != result[-3])
	{
	  ++result;
	  --remain;
	}
      }
    }
  }
  else if (h != 0)
  {
    assert(bi == 0 || bi == NUM_BLOCKHASHES - 1);
    assert(remain > 0);
    if (bi == 0)
      *result++ = b64[self->bh[bi].h];
    else
      *result++ = b64[self->lasth];
    /* No need to bother with FUZZY_FLAG_ELIMSEQ, because this
     * digest has length 1. */
    --remain;
  }
  *result = '\0';
  return 0;
}

void fuzzy_free(/*@only@*/ struct fuzzy_state *self)
{
  free(self);
}

int fuzzy_hash_buf(const unsigned char *buf,
		   uint32_t buf_len,
		   /*@out@*/ char *result)
{
  struct fuzzy_state *ctx;
  int ret = -1;
  if (NULL == (ctx = fuzzy_new()))
    return -1;
  if (fuzzy_set_total_input_length(ctx, buf_len) < 0)
    goto out;
  if (fuzzy_update(ctx, buf, buf_len) < 0)
    goto out;
  if (fuzzy_digest(ctx, result, 0) < 0)
    goto out;
  ret = 0;
 out:
  fuzzy_free(ctx);
  return ret;
}

static int fuzzy_update_stream(struct fuzzy_state *state,
			       FILE *handle)
{
  unsigned char buffer[4096];
  size_t n;
  for(;;)
  {
    n = fread(buffer, 1, 4096, handle);
    if (0 == n)
      break;
    if (fuzzy_update(state, buffer, n) < 0)
      return -1;
  }
  if (ferror(handle) != 0)
    return -1;
  return 0;
}

int fuzzy_hash_stream(FILE *handle, /*@out@*/ char *result)
{
  struct fuzzy_state *ctx;
  int ret = -1;
  if (NULL == (ctx = fuzzy_new()))
    return -1;
  if (fuzzy_update_stream(ctx, handle) < 0)
    goto out;
  if (fuzzy_digest(ctx, result, 0) < 0)
    goto out;
  ret = 0;
 out:
  fuzzy_free(ctx);
  return ret;
}

#ifdef S_SPLINT_S
typedef size_t off_t;
int fseeko(FILE *, off_t, int);
off_t ftello(FILE *);
#endif

int fuzzy_hash_file(FILE *handle, /*@out@*/ char *result)
{
  off_t fpos, fposend;
  int status = -1;
  struct fuzzy_state *ctx;
  fpos = ftello(handle);
  if (fpos < 0)
    return -1;
  if (fseeko(handle, 0, SEEK_END) < 0)
    return -1;
  fposend = ftello(handle);
  if (fposend < 0)
    return -1;
  if (fseeko(handle, 0, SEEK_SET) < 0)
    return -1;
  if (NULL == (ctx = fuzzy_new()))
    return -1;
  if (fuzzy_set_total_input_length(ctx, (uint_least64_t)fposend) < 0)
    goto out;
  if (fuzzy_update_stream(ctx, handle) < 0)
    goto out;
  status = fuzzy_digest(ctx, result, 0);
out:
  if (status == 0)
  {
    if (fseeko(handle, fpos, SEEK_SET) < 0)
      return -1;
  }
  fuzzy_free(ctx);
  return status;
}

int fuzzy_hash_filename(const char *filename, /*@out@*/ char *result)
{
  int status;
  FILE *handle = fopen(filename, "rb");
  if (NULL == handle)
    return -1;
  status = fuzzy_hash_stream(handle, result);
  /* We cannot do anything about an fclose failure. */
  (void)fclose(handle);
  return status;
}

//
// We only accept a match if we have at least one common substring in
// the signature of length ROLLING_WINDOW. This dramatically drops the
// false positive rate for low score thresholds while having
// negligable affect on the rate of spam detection.
//
// return 1 if the two strings do have a common substring, 0 otherwise
//
static int has_common_substring(const char *s1, size_t s1len, const char *s2, size_t s2len)
{
  size_t i, j;
  uint32_t hashes[SPAMSUM_LENGTH - (ROLLING_WINDOW - 1)];

  if (s1len < ROLLING_WINDOW)
    return 0;
  if (s2len < ROLLING_WINDOW)
    return 0;

  // there are many possible algorithms for common substring
  // detection. In this case I am re-using the rolling hash code
  // to act as a filter for possible substring matches

  memset(hashes, 0, sizeof(hashes));

  // first compute the windowed rolling hash at each offset in
  // the first string
  struct roll_state state;
  roll_init (&state);

  for (i = 0; i < ROLLING_WINDOW - 1; i++)
    roll_hash(&state, (unsigned char)s1[i]);
  for (i = ROLLING_WINDOW - 1; i < s1len; i++)
  {
    roll_hash(&state, (unsigned char)s1[i]);
    hashes[i - (ROLLING_WINDOW - 1)] = roll_sum(&state);
  }
  s1len -= (ROLLING_WINDOW - 1);

  roll_init(&state);

  // now for each offset in the second string compute the
  // rolling hash and compare it to all of the rolling hashes
  // for the first string. If one matches then we have a
  // candidate substring match. We then confirm that match with
  // a direct string comparison */
  for (j = 0; j < ROLLING_WINDOW - 1; j++)
    roll_hash(&state, (unsigned char)s2[j]);
  for (j = 0; j < s2len - (ROLLING_WINDOW - 1); j++)
  {
    roll_hash(&state, (unsigned char)s2[j + (ROLLING_WINDOW - 1)]);
    uint32_t h = roll_sum(&state);
    for (i = 0; i < s1len; i++)
    {
      // confirm the match after checking potential match
      if (hashes[i] == h && !memcmp(s1 + i, s2 + j, ROLLING_WINDOW))
	  return 1;
    }
  }

  return 0;
}



#ifdef SSDEEP_ENABLE_POSITION_ARRAY

// position array-based version of has_common_substring
static int has_common_substring_pa(const unsigned long long *parray, const char *s2, size_t s2len)
{
  unsigned long long D;
  // ROLLING_WINDOW <= s2len <= 64
  size_t r = ROLLING_WINDOW - 1;
  size_t l;
  const char *ch;
  while (r < s2len)
  {
    // because we want to reuse position array for s1,
    // both s1 and s2 (in the original pseudocode) are reversed.
    l = r - (ROLLING_WINDOW - 1);
    ch = &s2[s2len - 1 - r];
    D = parray[*ch - CHAR_MIN];
    while (D)
    {
      r--;
      D = (D << 1) & parray[*++ch - CHAR_MIN];
      if (r == l && D)
	return 1;
    }
    // Boyer-Moore-like skipping
    r += ROLLING_WINDOW;
  }
  return 0;
}

// position array-based version of edit_distn
static int edit_distn_pa(const unsigned long long *parray, size_t s1len, const char *s2, size_t s2len)
{
  unsigned long long pv, nv, ph, nh, zd, mt, x, y;
  unsigned long long msb;
  size_t i;
  // 0 < s1len <= 64
  int cur = s1len;
  msb = 1ull << (s1len - 1);
  pv = -1;
  nv = 0;
  for (i = 0; i < s2len; i++)
  {
    mt = parray[s2[i] - CHAR_MIN];
    zd = (((mt & pv) + pv) ^ pv) | mt | nv;
    nh = pv & zd;
    if (nh & msb)
      --cur;
    x  = nv | ~(pv | zd) | (pv & ~mt & 1ull);
    y  = (pv - nh) >> 1;
    ph = (x + y) ^ y;
    if (ph & msb)
      ++cur;
    x  = (ph << 1) | 1ull;
    nv = x & zd;
    pv = (nh << 1) | ~(x | zd) | (x & (pv - nh));
  }
  return cur;
}

#endif



// eliminate sequences of longer than 3 identical characters. These
// sequences contain very little information so they tend to just bias
// the result unfairly
static int copy_eliminate_sequences(char **out, size_t outsize, char **in, char etoken)
{
  size_t seq = 0;
  char prev = **in, curr;
  if (!prev || prev == etoken)
    return 1;
  if (!outsize--)
    return 0;
  *(*out)++ = prev;
  ++(*in);
  while (1)
  {
    curr = **in;
    if (!curr || curr == etoken)
      return 1;
    ++(*in);
    if (curr == prev)
    {
      if (++seq >= 3)
      {
	seq = 3;
	continue;
      }
      if (!outsize--)
	return 0;
      *(*out)++ = curr;
    }
    else
    {
      if (!outsize--)
	return 0;
      *(*out)++ = curr;
      seq = 0;
      prev = curr;
    }
  }
  // unreachable
  return 0;
}

//
// this is the low level string scoring algorithm. It takes two strings
// and scores them on a scale of 0-100 where 0 is a terrible match and
// 100 is a great match. The block_size is used to cope with very small
// messages.
//
static uint32_t score_strings(const char *s1,
			      size_t      s1len,
			      const char *s2,
			      size_t      s2len,
			      unsigned long block_size)
{
  uint32_t score;

#ifdef SSDEEP_ENABLE_POSITION_ARRAY
  unsigned long long parray[CHAR_MAX - CHAR_MIN + 1];
  size_t i;
  // skip short strings
  if (s1len < ROLLING_WINDOW)
    return 0;
  if (s2len < ROLLING_WINDOW)
    return 0;
  // construct position array for faster string algorithms
  memset(parray, 0, sizeof(parray));
  for (i = 0; i < s1len; i++)
    parray[s1[i] - CHAR_MIN] |= 1ull << i;
  // the two strings must have a common substring of length
  // ROLLING_WINDOW to be candidates
  if (!has_common_substring_pa(parray, s2, s2len))
    return 0;
  // compute the edit distance between the two strings. The edit distance gives
  // us a pretty good idea of how closely related the two strings are
  score = edit_distn_pa(parray, s1len, s2, s2len);
#else
  // the two strings must have a common substring of length
  // ROLLING_WINDOW to be candidates
  if (!has_common_substring(s1, s1len, s2, s2len))
    return 0;
  // compute the edit distance between the two strings. The edit distance gives
  // us a pretty good idea of how closely related the two strings are
  score = edit_distn(s1, s1len, s2, s2len);
#endif

  // scale the edit distance by the lengths of the two
  // strings. This changes the score to be a measure of the
  // proportion of the message that has changed rather than an
  // absolute quantity. It also copes with the variability of
  // the string lengths.
  score = (score * SPAMSUM_LENGTH) / (s1len + s2len);

  // at this stage the score occurs roughly on a 0-SPAMSUM_LENGTH scale,
  // with 0 being a good match and SPAMSUM_LENGTH being a complete
  // mismatch

  // rescale to a 0-100 scale (friendlier to humans)
  score = (100 * score) / SPAMSUM_LENGTH;

  // now re-scale on a 0-100 scale with 0 being a poor match and
  // 100 being a excellent match.
  score = 100 - score;

  //  printf ("s1len: %"PRIu32"  s2len: %"PRIu32"\n", (uint32_t)s1len, (uint32_t)s2len);

  // when the blocksize is small we don't want to exaggerate the match size
  if (block_size >= (99 + ROLLING_WINDOW) / ROLLING_WINDOW * MIN_BLOCKSIZE)
    return score;
  if (score > block_size/MIN_BLOCKSIZE * MIN(s1len, s2len))
  {
    score = block_size/MIN_BLOCKSIZE * MIN(s1len, s2len);
  }
  return score;
}

//
// Given two spamsum strings return a value indicating the degree
// to which they match.
//
int fuzzy_compare(const char *str1, const char *str2)
{
  unsigned long block_size1, block_size2;
  uint32_t score = 0;
  size_t s1b1len, s1b2len, s2b1len, s2b2len;
  char s1b1[SPAMSUM_LENGTH], s1b2[SPAMSUM_LENGTH];
  char s2b1[SPAMSUM_LENGTH], s2b2[SPAMSUM_LENGTH];
  char *s1p, *s2p, *tmp;

  if (NULL == str1 || NULL == str2)
    return -1;

  // each spamsum is prefixed by its block size
  if (sscanf(str1, "%lu:", &block_size1) != 1 ||
      sscanf(str2, "%lu:", &block_size2) != 1) {
    return -1;
  }

  // if the blocksizes don't match then we are comparing
  // apples to oranges. This isn't an 'error' per se. We could
  // have two valid signatures, but they can't be compared.
  if (block_size1 != block_size2 &&
      (block_size1 > ULONG_MAX / 2 || block_size1*2 != block_size2) &&
      (block_size1 % 2 == 1 || block_size1 / 2 != block_size2)) {
    return 0;
  }

  // move past the prefix
  s1p = strchr(str1, ':');
  s2p = strchr(str2, ':');

  if (!s1p || !s2p) {
    // badly formed ...
    return -1;
  }

  // there is very little information content is sequences of
  // the same character like 'LLLLL'. Eliminate any sequences
  // longer than 3 while reading two pieces.
  // This is especially important when combined with the
  // has_common_substring() test at score_strings().

  // read the first digest
  ++s1p;
  tmp = s1b1;
  if (!copy_eliminate_sequences(&tmp, SPAMSUM_LENGTH, &s1p, ':'))
    return -1;
  s1b1len = tmp - s1b1;
  if (!*s1p++) {
    // a signature is malformed - it doesn't have 2 parts
    return -1;
  }
  tmp = s1b2;
  if (!copy_eliminate_sequences(&tmp, SPAMSUM_LENGTH, &s1p, ','))
    return -1;
  s1b2len = tmp - s1b2;

  // read the second digest
  ++s2p;
  tmp = s2b1;
  if (!copy_eliminate_sequences(&tmp, SPAMSUM_LENGTH, &s2p, ':'))
    return -1;
  s2b1len = tmp - s2b1;
  if (!*s2p++) {
    // a signature is malformed - it doesn't have 2 parts
    return -1;
  }
  tmp = s2b2;
  if (!copy_eliminate_sequences(&tmp, SPAMSUM_LENGTH, &s2p, ','))
    return -1;
  s2b2len = tmp - s2b2;

  // Now that we know the strings are both well formed, are they
  // identical? We could save ourselves some work here
  if (block_size1 == block_size2 && s1b1len == s2b1len && s1b2len == s2b2len) {
    if (!memcmp(s1b1, s2b1, s1b1len) && !memcmp(s1b2, s2b2, s1b2len)) {
      return 100;
    }
  }

  // each signature has a string for two block sizes. We now
  // choose how to combine the two block sizes. We checked above
  // that they have at least one block size in common
  if (block_size1 <= ULONG_MAX / 2) {
    if (block_size1 == block_size2) {
      uint32_t score1, score2;
      score1 = score_strings(s1b1, s1b1len, s2b1, s2b1len, block_size1);
      score2 = score_strings(s1b2, s1b2len, s2b2, s2b2len, block_size1*2);
      score = MAX(score1, score2);
    }
    else if (block_size1 * 2 == block_size2) {
      score = score_strings(s2b1, s2b1len, s1b2, s1b2len, block_size2);
    }
    else {
      score = score_strings(s1b1, s1b1len, s2b2, s2b2len, block_size1);
    }
  }
  else {
    if (block_size1 == block_size2) {
      score = score_strings(s1b1, s1b1len, s2b1, s2b1len, block_size1);
    }
    else if (block_size1 % 2 == 0 && block_size1 / 2 == block_size2) {
      score = score_strings(s1b1, s1b1len, s2b2, s2b2len, block_size1);
    }
    else {
      score = 0;
    }
  }

  return (int)score;
}
