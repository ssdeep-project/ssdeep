/**
 * Modified levenshtein distance calculation
 *
 * This program can be used, redistributed or modified under any of
 * Boost Software License 1.0, GPL v2 or GPL v3
 * See the file COPYING for details.
 *
 * $Id$
 *
 * Copyright (C) 2014 kikairoya <kikairoya@gmail.com>
 * Copyright (C) 2014 Jesse Kornblum <research@jessekornblum.com>
 */
#include <stddef.h>

#define EDIT_DISTN_MAXLEN 64 /* MAX_SPAMSUM */
#define EDIT_DISTN_INSERT_COST 1
#define EDIT_DISTN_REMOVE_COST 1
#define EDIT_DISTN_REPLACE_COST 2

#define MIN(x,y) ((x)<(y)?(x):(y))

int edit_distn(const char *s1, size_t s1len, const char *s2, size_t s2len) {
  int t[2][EDIT_DISTN_MAXLEN+1];
  int *t1 = t[0];
  int *t2 = t[1];
  int *t3;
  size_t i1, i2;
  for (i2 = 0; i2 <= s2len; i2++)
    t[0][i2] = i2 * EDIT_DISTN_REMOVE_COST;
  for (i1 = 0; i1 < s1len; i1++) {
    t2[0] = (i1 + 1) * EDIT_DISTN_INSERT_COST;
    for (i2 = 0; i2 < s2len; i2++) {
      int cost_a = t1[i2+1] + EDIT_DISTN_INSERT_COST;
      int cost_d = t2[i2] + EDIT_DISTN_REMOVE_COST;
      int cost_r = t1[i2] + (s1[i1] == s2[i2] ? 0 : EDIT_DISTN_REPLACE_COST);
      t2[i2+1] = MIN(MIN(cost_a, cost_d), cost_r);
    }
    t3 = t1;
    t1 = t2;
    t2 = t3;
  }
  return t1[s2len];
}


#ifdef __UNITTEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELLOWORLD "Hello World!"

// Convenience method for getting the edit distance of two strings
int edit_dist(const char *a, const char *b) {
  unsigned int a_len = 0, b_len = 0;
  if (a)
    a_len = strlen(a);
  if (b)
    b_len = strlen(b);

  return edit_distn(a, a_len, b, b_len);
}

// Exercises edit_dist on a and b. If the result matches the expected value,
// returns 0. If not, displays the message and returns 1.
int run_test(char *a, char *b, int expected, char *msg) {
  int actual = edit_dist(a,b);
  if (actual == expected)
    return 0;

  printf ("FAIL: Expected %d, got %d for %s:%s, %s\n",
          expected,
          actual,
          a,
          b,
          msg);
  return 1;
}

#define RUN_TEST(A,B,EXPECTED,MSG)   failures += run_test(A,B,EXPECTED,MSG)

int main(void) {
  unsigned int failures = 0;

  RUN_TEST(NULL, HELLOWORLD, 12, "Null source");
  RUN_TEST(HELLOWORLD, NULL, 12, "Null dest");
  RUN_TEST("", HELLOWORLD, 12, "Empty source");
  RUN_TEST(HELLOWORLD, "", 12, "Empty destination");
  RUN_TEST(HELLOWORLD, HELLOWORLD, 0, "Equal strings");
  RUN_TEST("Hello world", "Hell world", 1, "Delete");
  RUN_TEST("Hell world", "Hello world", 1, "Insert");
  RUN_TEST("Hello world", "Hello owrld", 2, "Swap");
  RUN_TEST("Hello world", "HellX world", 2, "Change");

  if (failures) {
    printf ("\n%u tests failed.\n", failures);
    return EXIT_FAILURE;
  }

  printf ("All tests passed.\n");
  return EXIT_SUCCESS;
}
#endif
