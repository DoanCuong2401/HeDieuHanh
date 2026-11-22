#ifndef COVMOCK_H
#define COVMOCK_H

#include "kernel/types.h"

/*
 * Simulated coverage provider for Week 5.
 * This is NOT real kernel coverage.
 */

#define COVMOCK_MAX_POINTS 1024

static int
covmock_collect(uint testcase_type, uint seed,
                uint *points, int capacity)
{
  uint base;
  uint value;
  int count;

  if (points == 0 || capacity <= 0)
    return -1;

  /*
   * Deterministic simulated coverage:
   * Same testcase type and seed produce the same points.
   */
  base = 100 + testcase_type * 50;
  value = seed ^ (seed >> 7) ^ (seed << 5);

  count = 2 + (value % 3);

  if (count > capacity)
    count = capacity;

  for (int i = 0; i < count; i++) {
    points[i] = (base + ((value >> (i * 3)) & 31))
                % COVMOCK_MAX_POINTS;
  }

  return count;
}

#endif
