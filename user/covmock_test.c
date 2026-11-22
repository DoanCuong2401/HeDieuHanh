#include "kernel/types.h"
#include "user/user.h"
#include "covmock.h"

int
main(void)
{
  uint points1[8];
  uint points2[8];
  int n1, n2;
  int i;
  int pass = 1;

  printf("=== COVERAGE MOCK TEST ===\n");

  // TC1: Collect simulated coverage.
  n1 = covmock_collect(1, 12345, points1, 8);
  printf("TC1: collected points = %d (expected 2-4)\n", n1);

  if (n1 < 2 || n1 > 4)
    pass = 0;

  // TC2: Same input must produce identical coverage.
  n2 = covmock_collect(1, 12345, points2, 8);
  printf("TC2: replay count = %d (expected %d)\n", n2, n1);

  if (n2 != n1)
    pass = 0;

  for (i = 0; i < n1 && i < n2; i++) {
    if (points1[i] != points2[i])
      pass = 0;
  }

  // TC3: All point IDs must be valid.
  for (i = 0; i < n1; i++) {
    if (points1[i] >= COVMOCK_MAX_POINTS)
      pass = 0;
  }

  printf("TC3: point IDs in valid range: %s\n",
         pass ? "YES" : "NO");

  // TC4: Invalid arguments.
  if (covmock_collect(1, 12345, 0, 8) != -1)
    pass = 0;

  if (covmock_collect(1, 12345, points1, 0) != -1)
    pass = 0;

  printf("TC4: invalid arguments checked\n");

  if (pass)
    printf("COVERAGE MOCK TEST: PASS\n");
  else
    printf("COVERAGE MOCK TEST: FAIL\n");

  exit(0);
}
