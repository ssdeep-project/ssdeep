#include <stdio.h>
#include <string.h>

#include "fuzzy.h"

int main(void)
{
  unsigned char data[8192];
  char buffer_hash[FUZZY_MAX_RESULT];
  char stream_hash[FUZZY_MAX_RESULT];
  char file_hash[FUZZY_MAX_RESULT];
  struct fuzzy_state *state;
  FILE *file;
  unsigned int i;

  if (fuzzy_hash_buf((const unsigned char *)"", 0, buffer_hash) ||
      strcmp(buffer_hash, "3::"))
    return 1;

  for (i = 0; i < sizeof(data); ++i)
    data[i] = (unsigned char)((i * i + i / 7) % 256);
  if (fuzzy_hash_buf(data, sizeof(data), buffer_hash))
    return 1;

  state = fuzzy_new();
  if (!state)
    return 1;
  if (fuzzy_update(state, data, 3000) ||
      fuzzy_update(state, data + 3000, sizeof(data) - 3000) ||
      fuzzy_digest(state, stream_hash, 0))
  {
    fuzzy_free(state);
    return 1;
  }
  fuzzy_free(state);
  if (strcmp(buffer_hash, stream_hash) || fuzzy_compare(buffer_hash, stream_hash) != 100)
    return 1;

  file = tmpfile();
  if (!file)
    return 1;
  if (fwrite(data, 1, sizeof(data), file) != sizeof(data) ||
      fseek(file, 0, SEEK_SET) || fuzzy_hash_file(file, file_hash))
  {
    fclose(file);
    return 1;
  }
  fclose(file);
  return strcmp(buffer_hash, file_hash) != 0;
}
