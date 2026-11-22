#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/coverage.h"
#include "user/user.h"
#include "covtrack.h"
#include "corpus.h"

#define DEFAULT_SEED 1
#define DEFAULT_COUNT 20
#define MAX_COUNT 100
#define DATA_SIZE 16

/*
 * Static buffer: struct cov_result is 4112 bytes,
 * so do not place it on the small user stack.
 */
static struct cov_result result;

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

/*
 * Testcase functions must not print anything.
 * They run while kernel coverage collection is enabled.
 */

static void
test_file_roundtrip(int id, uint *state)
{
  char path[32];
  char data[DATA_SIZE];
  char out[DATA_SIZE];
  int fd;

  path[0] = 'w';
  path[1] = '6';
  path[2] = 'f';
  path[3] = '0' + (id / 100) % 10;
  path[4] = '0' + (id / 10) % 10;
  path[5] = '0' + id % 10;
  path[6] = 0;

  for (int i = 0; i < DATA_SIZE; i++)
    data[i] = 'A' + (random_next(state) % 26);

  fd = open(path, O_CREATE | O_RDWR);
  if (fd >= 0) {
    write(fd, data, DATA_SIZE);
    close(fd);

    fd = open(path, O_RDONLY);
    if (fd >= 0) {
      read(fd, out, DATA_SIZE);
      close(fd);
    }

    unlink(path);
  }
}

static void
test_invalid_fd(int id, uint *state)
{
  int badfd;
  int r;

  (void)id;

  badfd = 100 + (random_next(state) % 8);
  close(badfd);
  r = read(badfd, (char *)&r, 0);
  (void)r;
}

static void
test_pipe(int id, uint *state)
{
  int fd[2];
  char msg[8];
  char buf[8];

  (void)id;

  for (int i = 0; i < 8; i++)
    msg[i] = 'a' + (random_next(state) % 26);

  if (pipe(fd) == 0) {
    write(fd[1], msg, sizeof(msg));
    read(fd[0], buf, sizeof(buf));
    close(fd[0]);
    close(fd[1]);
  }
}

static void
test_fork_wait(int id)
{
  int pid;
  int status;

  (void)id;

  pid = fork();

  if (pid == 0) {
    getpid();
    exit(0);
  }

  if (pid > 0)
    wait(&status);
}

static void
test_argument_edges(int id, uint *state)
{
  int fd;
  int r;
  int size;

  (void)id;

  size = (random_next(state) & 1) ? 0 : 1;

  fd = open("/no-such-fuzzer-file", O_RDONLY);
  if (fd >= 0)
    close(fd);

  fd = open("README", O_RDONLY);
  if (fd >= 0) {
    r = read(fd, (char *)&r, size);
    (void)r;
    close(fd);
  }
}

static uint
run_testcase(int id, uint *state)
{
  uint choice = random_next(state) % 5;

  switch (choice) {
  case 0:
    test_file_roundtrip(id, state);
    break;
  case 1:
    test_invalid_fd(id, state);
    break;
  case 2:
    test_pipe(id, state);
    break;
  case 3:
    test_fork_wait(id);
    break;
  case 4:
    test_argument_edges(id, state);
    break;
  }

  return choice;
}

/*
 * Hash the ordered coverage point list.
 * Used to check whether corpus replay reproduces
 * the same kernel coverage sequence.
 */
static uint
coverage_hash(const uint *points, uint count)
{
  uint hash = 2166136261U;

  for (uint i = 0; i < count; i++) {
    hash ^= points[i];
    hash *= 16777619U;
  }

  hash ^= count;
  hash *= 16777619U;

  return hash;
}

/*
 * Execute one testcase with real kernel coverage.
 *
 * Returns:
 *   0  success
 *  -1  coverage setup, collection, or validation error
 */
static int
run_measured_case(int id, uint seed,
                  uint *type, int *point_count,
                  uint *signature, uint *final_state)
{
  uint state = seed;
  int bytes;

  if (cov_ctl(COV_RESET) != COV_OK)
    return -1;

  if (cov_ctl(COV_ENABLE) != COV_OK)
    return -1;

  *type = run_testcase(id, &state);

  if (final_state != 0)
    *final_state = state;

  if (cov_ctl(COV_DISABLE) != COV_OK) {
    cov_ctl(COV_DISABLE);
    return -1;
  }

  bytes = cov_read(0, &result, sizeof(result));

  if (bytes < 0)
    return -1;

  if (result.header.version != COV_VERSION ||
      result.header.capacity != COV_CAPACITY ||
      result.header.count > COV_CAPACITY ||
      result.header.overflow != 0)
    return -1;

  if (bytes != (int)(sizeof(struct cov_header) +
                     result.header.count * sizeof(uint)))
    return -1;

  *point_count = (int)result.header.count;

  *signature = coverage_hash(
      result.point_ids, result.header.count);

  return 0;
}

int
main(int argc, char *argv[])
{
  uint seed;
  uint state;
  uint case_seed;
  uint type;
  uint signature;

  uint replay_type;
  uint replay_signature;

  uint corpus_hash[CORPUS_CAPACITY];

  int count;
  int point_count;
  int new_points;
  int added;
  int replay_point_count;
  int replay_ok;
  int failed = 0;

  struct cov_tracker tracker;
  struct corpus corpus;
  struct fuzz_case saved;

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

  covtrack_init(&tracker);
  corpus_init(&corpus);

  printf("=== XV6 STATEFUL SYSCALL FUZZER WEEK 6 ===\n");
  printf("Seed: %d\n", (int)seed);
  printf("Testcases: %d\n", count);
  printf("Coverage source: REAL KERNEL COVERAGE API\n");

  /*
   * Main fuzzing loop.
   */
  for (int i = 0; i < count; i++) {
    case_seed = state;

    printf("\n--- Testcase %d ---\n", i + 1);

    if (run_measured_case(
          i + 1, case_seed,
          &type, &point_count, &signature, &state) < 0) {
      printf("Coverage collection: FAIL\n");
      failed = 1;
      continue;
    }

    new_points = covtrack_process(
        &tracker, result.point_ids, point_count);

    if (new_points < 0) {
      printf("Coverage tracking: FAIL\n");
      failed = 1;
      continue;
    }

    added = corpus_add_replay(
        &corpus, (uint)(i + 1),
        case_seed, type, (uint)new_points);

    if (added == 1)
      corpus_hash[corpus.count - 1] = signature;

    printf("Testcase type: %d\n", (int)type);
    printf("Kernel coverage points: %d\n", point_count);
    printf("New coverage points: %d\n", new_points);

    if (added == 1)
      printf("Corpus: testcase saved\n");
    else if (added == 0)
      printf("Corpus: testcase skipped (no new coverage or duplicate)\n");
    else
      printf("Corpus: full\n");
  }

  /*
   * Replay every testcase saved in the corpus.
   */
  printf("\n=== CORPUS REPLAY ===\n");
  replay_ok = 1;

  for (int j = 0; j < (int)corpus.count; j++) {
    if (corpus_get(&corpus, (uint)j, &saved) < 0) {
      printf("Replay %d: cannot read corpus entry\n", j + 1);
      replay_ok = 0;
      continue;
    }

    printf("\nReplay %d: original TC %d, seed=%d\n",
           j + 1, (int)saved.id, (int)saved.seed);

    if (run_measured_case(
          (int)saved.id, saved.seed,
          &replay_type, &replay_point_count,
          &replay_signature, 0) < 0) {
      printf("  Replay: FAIL (coverage collection)\n");
      replay_ok = 0;
      continue;
    }

    if (replay_type == saved.type &&
        replay_signature == corpus_hash[j]) {
      printf("  Type: %d (expected %d)\n",
             (int)replay_type, (int)saved.type);
      printf("  Kernel coverage points: %d\n",
             replay_point_count);
      printf("  Coverage signature: MATCH\n");
      printf("  Replay: PASS\n");
    } else {
      printf("  Type: %d (expected %d)\n",
             (int)replay_type, (int)saved.type);
      printf("  Coverage signature: MISMATCH\n");
      printf("  Replay: FAIL\n");
      replay_ok = 0;
    }
  }

  printf("\nCorpus replay result: %s\n",
         replay_ok ? "PASS" : "FAIL");

  printf("\n=== WEEK 6 SUMMARY ===\n");
  printf("Total unique kernel coverage points: %d\n",
         (int)tracker.total_seen);
  printf("Corpus entries: %d\n", (int)corpus.count);
  printf("=== FUZZING COMPLETED ===\n");

  if (!replay_ok)
    failed = 1;

  exit(failed);
}
