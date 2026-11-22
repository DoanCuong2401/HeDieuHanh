#ifndef CORPUS_H
#define CORPUS_H

#include "kernel/types.h"

#define CORPUS_CAPACITY 64

struct fuzz_case {
  uint id;          // Original testcase ID
  uint seed;        // RNG state before testcase execution
  uint type;        // Testcase type
  uint new_points;  // Newly discovered coverage points
};

struct corpus {
  struct fuzz_case cases[CORPUS_CAPACITY];
  uint count;
};

static void
corpus_init(struct corpus *c)
{
  c->count = 0;
}

/*
 * Add a testcase that discovered new coverage.
 * Return: 1 added, 0 skipped, -1 corpus full.
 */
static int __attribute__((unused))
corpus_add_replay(struct corpus *c, uint id, uint seed,
                  uint type, uint new_points)
{
  uint i;

  if (c == 0)
    return -1;

  if (new_points == 0)
    return 0;

  for (i = 0; i < c->count; i++) {
    if (c->cases[i].seed == seed &&
        c->cases[i].type == type)
      return 0;
  }

  if (c->count >= CORPUS_CAPACITY)
    return -1;

  c->cases[c->count].id = id;
  c->cases[c->count].seed = seed;
  c->cases[c->count].type = type;
  c->cases[c->count].new_points = new_points;
  c->count++;

  return 1;
}

/* Compatibility wrapper for the original corpus test. */
static int __attribute__((unused))
corpus_add(struct corpus *c, uint seed,
           uint type, int new_points)
{
  if (new_points <= 0)
    return 0;

  return corpus_add_replay(c, 0, seed, type,
                           (uint)new_points);
}

static int __attribute__((unused))
corpus_get(struct corpus *c, uint index,
           struct fuzz_case *out)
{
  if (c == 0 || out == 0 || index >= c->count)
    return -1;

  *out = c->cases[index];
  return 0;
}

#endif
