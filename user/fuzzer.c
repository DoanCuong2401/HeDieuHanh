
#include "kernel/types.h"
#include "user/user.h"

// Week 3: deterministic syscall fuzzer prototype

#define DEFAULT_SEED 1
#define DEFAULT_COUNT 20
#define MAX_COUNT 100

// xorshift32 PRNG.
// The same non-zero seed produces the same sequence.
static uint
random_next(uint *state)
{
  uint x = *state;

  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  *state = x;
  return x;
}

static void
run_testcase(int id, uint *state)
{
  uint choice;
  int result;

  choice = random_next(state) % 3;

  switch (choice) {
  case 0:
    result = getpid();
    printf("TC %d: getpid() = %d\n", id, result);
    break;

  case 1:
    result = uptime();
    printf("TC %d: uptime() = %d\n", id, result);
    break;

  case 2: {
    int ticks = random_next(state) % 3;

    result = pause(ticks);
    printf("TC %d: pause(%d) = %d\n",
           id, ticks, result);
    break;
  }

  default:
    printf("TC %d: invalid operation\n", id);
    break;
  }
}

int
main(int argc, char *argv[])
{
  uint seed;
  uint state;
  int count;
  int i;

  seed = DEFAULT_SEED;
  count = DEFAULT_COUNT;

  if (argc > 1)
    seed = (uint)atoi(argv[1]);

  if (argc > 2)
    count = atoi(argv[2]);

  if (seed == 0)
    seed = DEFAULT_SEED;

  if (count <= 0 || count > MAX_COUNT)
    count = DEFAULT_COUNT;

  state = seed;

  printf("=== XV6 SYSCALL FUZZER ===\n");
  printf("Seed: %d\n", (int)seed);
  printf("Testcases: %d\n", count);

  for (i = 0; i < count; i++)
    run_testcase(i + 1, &state);

  printf("=== FUZZING COMPLETED ===\n");

  exit(0);
}

