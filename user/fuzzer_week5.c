#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "covtrack.h"
#include "corpus.h"
#include "covmock.h"

/*
 * Week 4 - Stateful syscall fuzzer v1.
 * Replay: run the same command with the same non-zero seed and count.
 * The generator is deterministic; syscall return values and timing are not.
 */

#define DEFAULT_SEED 1
#define DEFAULT_COUNT 20
#define MAX_COUNT 100
#define DATA_SIZE 16

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
test_file_roundtrip(int id, uint *state)
{
  char path[32];
  char out[DATA_SIZE];
  char data[DATA_SIZE];
  int fd, n, i;

  /* Unique deterministic name avoids collisions between testcases. */
  path[0] = 'f'; path[1] = 'z'; path[2] = 'z';
  path[3] = '0' + (id / 100) % 10;
  path[4] = '0' + (id / 10) % 10;
  path[5] = '0' + id % 10;
  path[6] = 0;

  for (i = 0; i < DATA_SIZE; i++)
    data[i] = 'A' + (random_next(state) % 26);

  printf("TC %d [FILE]: open(%s, CREATE|RDWR)\n", id, path);
  fd = open(path, O_CREATE | O_RDWR);
  printf("  -> fd=%d\n", fd);
  if (fd < 0)
    return;

  n = write(fd, data, DATA_SIZE);
  printf("  write(fd, %d bytes) = %d\n", DATA_SIZE, n);

  /* Rewind by closing and reopening; xv6 has no lseek syscall. */
  close(fd);
  fd = open(path, O_RDONLY);
  printf("  reopen(%s) = %d\n", path, fd);
  if (fd >= 0) {
    n = read(fd, out, DATA_SIZE);
    printf("  read(fd, %d bytes) = %d\n", DATA_SIZE, n);
    close(fd);
  }
  unlink(path);
}

static void
test_invalid_fd(int id, uint *state)
{
  int badfd = 100 + (random_next(state) % 8);
  int r;

  printf("TC %d [INVALID-FD]: close(%d)\n", id, badfd);
  r = close(badfd);
  printf("  close result = %d (expected -1)\n", r);
  r = read(badfd, (char *)&r, 0);
  printf("  read invalid fd result = %d (expected -1)\n", r);
}

static void
test_pipe(int id, uint *state)
{
  int fd[2];
  char msg[8], buf[8];
  int n, i;

  for (i = 0; i < 8; i++)
    msg[i] = 'a' + (random_next(state) % 26);

  printf("TC %d [PIPE]: pipe()\n", id);
  if (pipe(fd) < 0) {
    printf("  pipe failed\n");
    return;
  }
  printf("  pipe readfd=%d writefd=%d\n", fd[0], fd[1]);
  n = write(fd[1], msg, sizeof(msg));
  printf("  write(pipe, 8) = %d\n", n);
  n = read(fd[0], buf, sizeof(buf));
  printf("  read(pipe, 8) = %d\n", n);
  close(fd[0]);
  close(fd[1]);
}

static void
test_fork_wait(int id)
{
  int pid, status;

  printf("TC %d [PROCESS]: fork()\n", id);
  pid = fork();
  if (pid < 0) {
    printf("  fork failed\n");
    return;
  }
  if (pid == 0) {
    printf("  child: getpid()=%d\n", getpid());
    exit(0);
  }

  printf("  parent: child pid=%d\n", pid);
  status = -1;
  printf("  wait() = %d\n", wait(&status));
}

static void
test_argument_edges(int id, uint *state)
{
  int fd, r;
  int size = (random_next(state) & 1) ? 0 : 1;

  printf("TC %d [ARG-EDGE]: open missing path\n", id);
  r = open("/no-such-fuzzer-file", O_RDONLY);
  printf("  open missing = %d (expected -1)\n", r);

  fd = open("/README", O_RDONLY);
  if (fd >= 0) {
    r = read(fd, (char *)&r, size);
    printf("  read size=%d result=%d\n", size, r);
    close(fd);
  } else {
    printf("  open /README failed\n");
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

int
main(int argc, char *argv[])
{
  uint seed, state, case_seed, type;
  uint points[8];
  int count, i;
  int point_count, new_points, added;
  int j, replay_ok, replay_point_count;
  uint replay_state, replay_type;
  struct fuzz_case saved;

  struct cov_tracker tracker;
  struct corpus corpus;

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

  printf("=== XV6 STATEFUL SYSCALL FUZZER WEEK 5 ===\n");
  printf("Seed: %d\n", (int)seed);
  printf("Testcases: %d\n", count);
  printf("Coverage source: SIMULATED (not kernel coverage)\n");

  for (i = 0; i < count; i++) {
    /*
     * Save RNG state before testcase execution.
     * This state can be used to replay the testcase.
     */
    case_seed = state;

    printf("\n--- Testcase %d ---\n", i + 1);

    type = run_testcase(i + 1, &state);

    point_count = covmock_collect(
        type, case_seed, points, 8);

    if (point_count < 0) {
      printf("Coverage collection failed\n");
      continue;
    }

    new_points = covtrack_process(
        &tracker, points, point_count);

    if (new_points < 0) {
      printf("Coverage tracking failed\n");
      continue;
    }

    added = corpus_add_replay(
        &corpus, (uint)(i + 1), case_seed,
        type, (uint)new_points);

    printf("Testcase type: %d\n", (int)type);
    printf("Simulated coverage points: %d\n", point_count);
    printf("New coverage points: %d\n", new_points);

    if (added == 1)
      printf("Corpus: testcase saved\n");
    else if (added == 0)
      printf("Corpus: testcase skipped\n");
    else
      printf("Corpus: full\n");

    printf("Replay state: %d, type: %d\n",
           (int)case_seed, (int)type);
  }

  /*
   * Replay every testcase saved in the corpus.
   * Restore the original testcase ID and RNG state.
   */
  printf("\n=== CORPUS REPLAY ===\n");
  replay_ok = 1;

  for (j = 0; j < (int)corpus.count; j++) {
    if (corpus_get(&corpus, (uint)j, &saved) < 0) {
      printf("Replay %d: cannot read corpus entry\n", j + 1);
      replay_ok = 0;
      continue;
    }

    replay_state = saved.seed;

    printf("\nReplay %d: original TC %d, seed=%d\n",
           j + 1, (int)saved.id, (int)saved.seed);

    replay_type = run_testcase((int)saved.id, &replay_state);

    replay_point_count = covmock_collect(
        replay_type, saved.seed, points, 8);

    if (replay_type == saved.type && replay_point_count >= 0) {
      printf("  Type: %d (expected %d)\n",
             (int)replay_type, (int)saved.type);
      printf("  Simulated coverage points: %d\n",
             replay_point_count);
      printf("  Replay: PASS\n");
    } else {
      printf("  Replay: FAIL (type or coverage generation mismatch)\n");
      replay_ok = 0;
    }
  }

  printf("\nCorpus replay result: %s\n",
         replay_ok ? "PASS" : "FAIL");

  printf("\n=== WEEK 5 SUMMARY ===\n");
  printf("Total unique coverage points: %d\n",
         (int)tracker.total_seen);
  printf("Corpus entries: %d\n", (int)corpus.count);
  printf("=== FUZZING COMPLETED ===\n");

  exit(0);
}
