#include "kernel/types.h"
#include "user/user.h"
#include "covtrack.h"

int
main(void)
{
  struct cov_tracker tracker;

  uint points1[] = {1, 2, 3};
  uint points2[] = {1, 2, 3};
  uint points3[] = {2, 4};
  uint points4[] = {256, 257, 280};

  int result;

  covtrack_init(&tracker);

  printf("=== COVERAGE TRACKER TEST ===\n");

  result = covtrack_process(&tracker, points1, 3);
  printf("TC1: new points = %d (expected 3)\n", result);

  result = covtrack_process(&tracker, points2, 3);
  printf("TC2: new points = %d (expected 0)\n", result);

  result = covtrack_process(&tracker, points3, 2);
  printf("TC3: new points = %d (expected 1)\n", result);

  result = covtrack_process(&tracker, points4, 3);
  printf("TC4: new points = %d (expected 3)\n", result);

  printf("Total unique points = %d (expected 7)\n",
         (int)tracker.total_seen);

  if (tracker.total_seen == 7)
    printf("COVERAGE TRACKER: PASS\n");
  else
    printf("COVERAGE TRACKER: FAIL\n");

  exit(0);
}
