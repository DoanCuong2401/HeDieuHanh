#ifndef COVTRACK_H
#define COVTRACK_H

#include "kernel/types.h"

#define COVTRACK_MAX_POINTS 1024
#define COVTRACK_WORD_BITS 32
#define COVTRACK_WORDS (COVTRACK_MAX_POINTS / COVTRACK_WORD_BITS)

struct cov_tracker {
  uint bitmap[COVTRACK_WORDS];
  uint total_seen;
};

static void
covtrack_init(struct cov_tracker *tracker)
{
  int i;

  for (i = 0; i < COVTRACK_WORDS; i++)
    tracker->bitmap[i] = 0;

  tracker->total_seen = 0;
}

/*
 * Record one coverage point.
 * Return:
 *   1: new coverage point
 *   0: already seen
 *  -1: invalid point ID
 */
static int
covtrack_add(struct cov_tracker *tracker, uint point_id)
{
  uint word;
  uint bit;
  uint mask;

  if (point_id >= COVTRACK_MAX_POINTS)
    return -1;

  word = point_id / COVTRACK_WORD_BITS;
  bit = point_id % COVTRACK_WORD_BITS;
  mask = 1U << bit;

  if (tracker->bitmap[word] & mask)
    return 0;

  tracker->bitmap[word] |= mask;
  tracker->total_seen++;

  return 1;
}

/*
 * Process a batch of coverage point IDs.
 * Return the number of newly discovered points.
 * Return -1 if an invalid point ID is encountered.
 */
static int
covtrack_process(struct cov_tracker *tracker,
                 const uint *points, int count)
{
  int i;
  int new_points = 0;

  if (count < 0 || (count > 0 && points == 0))
    return -1;

  // Validate all point IDs before changing the bitmap.
  for (i = 0; i < count; i++) {
    if (points[i] >= COVTRACK_MAX_POINTS)
      return -1;
  }

  // All IDs are valid. Now update the tracker.
  for (i = 0; i < count; i++) {
    if (covtrack_add(tracker, points[i]) == 1)
      new_points++;
  }

  return new_points;
}

#endif
