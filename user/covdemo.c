#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/coverage.h"
#include "user/user.h"

static struct cov_result result;

static int
measure_read(const char *name, int fd, int expected_byte, uint read_point)
{
  uint expected[] = {
    COV_DISPATCH_VALID,
    COV_SYSCALL_BASE + SYS_read,
    COV_READ_FORWARD,
    read_point,
  };
  uint count = sizeof(expected) / sizeof(expected[0]);
  char byte = 0;

  if (cov_ctl(COV_RESET) != COV_OK || cov_ctl(COV_ENABLE) != COV_OK) {
    printf("FAIL %s: coverage setup\n", name);
    return 1;
  }

  int actual = read(fd, &byte, 1);

  if (cov_ctl(COV_DISABLE) != COV_OK) {
    printf("FAIL %s: coverage stop\n", name);
    return 1;
  }

  int bytes = cov_read(0, &result, sizeof(result));
  if (bytes != (int)(sizeof(result.header) + sizeof(expected)) ||
      result.header.version != COV_VERSION ||
      result.header.capacity != COV_CAPACITY ||
      result.header.count != count ||
      result.header.overflow != 0) {
    printf("FAIL %s: invalid coverage snapshot (bytes=%d)\n", name, bytes);
    return 1;
  }

  printf("%s: read expected=1 actual=%d, points:", name, actual);
  int failed = actual != 1 ||
               (expected_byte >= 0 && (uchar)byte != expected_byte);
  for (uint i = 0; i < count; i++) {
    printf(" %u", result.point_ids[i]);
    if (result.point_ids[i] != expected[i])
      failed = 1;
  }
  printf(" [%s]\n", failed ? "FAIL" : "PASS");
  return failed;
}

int
main(void)
{
  int fd = open("README", O_RDONLY);
  if (fd < 0) {
    printf("FAIL file: open README\n");
    exit(1);
  }
  int failed = measure_read("file", fd, -1, COV_FILE_READ_INODE);
  close(fd);

  int ends[2];
  if (pipe(ends) < 0) {
    printf("FAIL pipe: create pipe\n");
    exit(1);
  }
  if (write(ends[1], "K", 1) == 1) {
    failed |= measure_read("pipe", ends[0], 'K', COV_FILE_READ_PIPE);
  } else {
    printf("FAIL pipe: prepare data\n");
    failed = 1;
  }
  close(ends[0]);
  close(ends[1]);

  exit(failed);
}
