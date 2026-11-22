#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/coverage.h"
#include "user/user.h"

enum { SUITE_CASES = 3, MAX_READS = 2 };

static struct cov_result result;
static uint suite_points[SUITE_CASES * COV_CAPACITY];
static uint suite_captured, suite_events, suite_unique, suite_overflow;

static int
point_seen(const uint *points, uint count, uint id)
{
  for (uint i = 0; i < count; i++)
    if (points[i] == id)
      return 1;
  return 0;
}

static int
measure_reads(const char *name, int fd, uint reads, int expected_byte,
              uint read_point)
{
  const uint expected[] = {
    COV_DISPATCH_VALID,
    COV_SYSCALL_BASE + SYS_read,
    COV_READ_FORWARD,
    read_point,
  };
  const uint points_per_read = sizeof(expected) / sizeof(expected[0]);
  char data[MAX_READS] = {0};
  int actual[MAX_READS];

  if (cov_ctl(COV_RESET) != COV_OK || cov_ctl(COV_ENABLE) != COV_OK) {
    printf("FAIL %s: coverage setup\n", name);
    return 1;
  }

  for (uint i = 0; i < reads; i++)
    actual[i] = read(fd, &data[i], 1);

  if (cov_ctl(COV_DISABLE) != COV_OK) {
    printf("FAIL %s: coverage stop\n", name);
    return 1;
  }

  int bytes = cov_read(0, &result, sizeof(result));
  if (bytes < (int)sizeof(result.header) ||
      result.header.version != COV_VERSION ||
      result.header.capacity != COV_CAPACITY ||
      result.header.count > result.header.capacity ||
      result.header.overflow > 1 ||
      bytes != (int)(sizeof(result.header) +
                     result.header.count * sizeof(result.point_ids[0]))) {
    printf("FAIL %s: invalid coverage snapshot (bytes=%d)\n", name, bytes);
    return 1;
  }

  uint count = result.header.count;
  uint unique = 0;
  int failed = count != reads * points_per_read || result.header.overflow;
  printf("%s: read expected=1 each, actual:", name);
  for (uint i = 0; i < reads; i++) {
    printf(" %d", actual[i]);
    if (actual[i] != 1 || (expected_byte >= 0 && (uchar)data[i] != expected_byte))
      failed = 1;
  }
  printf(", points:");
  for (uint i = 0; i < count; i++) {
    uint id = result.point_ids[i];
    printf(" %u", id);
    if (id != expected[i % points_per_read])
      failed = 1;
    if (!point_seen(result.point_ids, i, id))
      unique++;
    if (!point_seen(suite_points, suite_unique, id))
      suite_points[suite_unique++] = id;
  }
  suite_captured++;
  suite_events += count;
  suite_overflow |= result.header.overflow;
  printf(", recorded=%u unique=%u overflow=%u [%s]\n",
         count, unique, result.header.overflow, failed ? "FAIL" : "PASS");
  return failed;
}

static int
file_cases(void)
{
  int fd = open("README", O_RDONLY);
  if (fd < 0) {
    printf("FAIL file: open README\n");
    return 1;
  }
  int failed = measure_reads("file", fd, 1, -1, COV_FILE_READ_INODE);
  failed |= measure_reads("file-repeat", fd, 2, -1, COV_FILE_READ_INODE);
  close(fd);
  return failed;
}

static int
pipe_case(void)
{
  int ends[2];
  if (pipe(ends) < 0) {
    printf("FAIL pipe: create pipe\n");
    return 1;
  }
  int failed;
  if (write(ends[1], "K", 1) == 1) {
    failed = measure_reads("pipe", ends[0], 1, 'K', COV_FILE_READ_PIPE);
  } else {
    printf("FAIL pipe: prepare data\n");
    failed = 1;
  }
  close(ends[0]);
  close(ends[1]);
  return failed;
}

int
main(void)
{
  int failed = file_cases();
  failed |= pipe_case();
  printf("suite: snapshots=%u/%u recorded=%u unique=%u overflow=%u [%s]\n",
         suite_captured, (uint)SUITE_CASES, suite_events, suite_unique,
         suite_overflow, failed ? "FAIL" : "PASS");
  exit(failed);
}
