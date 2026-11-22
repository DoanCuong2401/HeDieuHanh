#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/coverage.h"
#include "user/user.h"

static struct cov_result result;
static int failures;

/*
 * Collect exactly one read syscall.
 * Setup, cleanup and printing happen with coverage disabled.
 */
static void
test_read(const char *name, int fd, int length,
          int expected_return, uint branch, int expected_byte)
{
  char byte = 0;
  int actual, bytes;
  uint expected[4];
  uint count = branch == COV_READ_BAD_FD ? 3 : 4;

  if (cov_ctl(COV_RESET) != COV_OK ||
      cov_ctl(COV_ENABLE) != COV_OK) {
    cov_ctl(COV_DISABLE);
    printf("FAIL: coverage setup for %s\n", name);
    failures++;
    return;
  }

  actual = read(fd, &byte, length);

  int stopped = cov_ctl(COV_DISABLE);
  bytes = cov_read(0, &result, sizeof(result));

  expected[0] = COV_DISPATCH_VALID;
  expected[1] = COV_SYSCALL_BASE + SYS_read;
  expected[2] = branch == COV_READ_BAD_FD
    ? COV_READ_BAD_FD : COV_READ_FORWARD;
  expected[3] = branch;

  int ok =
    stopped == COV_OK &&
    actual == expected_return &&
    bytes == (int)(sizeof(struct cov_header) + count * sizeof(uint)) &&
    result.header.version == COV_VERSION &&
    result.header.capacity == COV_CAPACITY &&
    result.header.count == count &&
    result.header.overflow == 0;

  if (expected_byte >= 0 && (unsigned char)byte != expected_byte)
    ok = 0;

  if (ok) {
    for (uint i = 0; i < count; i++) {
      if (result.point_ids[i] != expected[i])
        ok = 0;
    }
  }

  if (ok) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s (read=%d, coverage_bytes=%d)\n",
           name, actual, bytes);
    failures++;
  }
}

int
main(void)
{
  int fd, device;
  int pipefd[2];

  if (cov_ctl(COV_ENABLE) != COV_OK ||
      cov_ctl(COV_DISABLE) != COV_OK) {
    printf("FAIL: initial coverage setup\n");
    exit(1);
  }

  test_read("negative descriptor", -1, 1, -1,
            COV_READ_BAD_FD, -1);

  fd = open("README", O_RDONLY);
  if (fd < 0) {
    printf("FAIL: open README\n");
    exit(1);
  }

  test_read("negative length rejected", fd, -1, -1,
            COV_FILE_READ_REJECT, -1);

  test_read("inode read returns one byte", fd, 1, 1,
            COV_FILE_READ_INODE, -1);

  close(fd);

  test_read("closed descriptor rejected", fd, 1, -1,
            COV_READ_BAD_FD, -1);

  if (pipe(pipefd) < 0) {
    printf("FAIL: create pipe\n");
    exit(1);
  }

  test_read("write-only pipe endpoint rejected", pipefd[1], 1, -1,
            COV_FILE_READ_REJECT, -1);

  if (write(pipefd[1], "K", 1) != 1) {
    close(pipefd[0]);
    close(pipefd[1]);
    printf("FAIL: prepare pipe data\n");
    exit(1);
  }

  test_read("pipe read returns expected byte", pipefd[0], 1, 1,
            COV_FILE_READ_PIPE, 'K');

  close(pipefd[0]);
  close(pipefd[1]);

  device = open("console", O_RDONLY);
  if (device < 0) {
    printf("FAIL: open console\n");
    exit(1);
  }

  /*
   * Zero-length console read exercises device dispatch
   * without waiting for keyboard input.
   */
  test_read("zero-length console read", device, 0, 0,
            COV_FILE_READ_DEVICE, -1);

  close(device);

  if (failures) {
    printf("COVERAGE READ TESTS FAILED: %d\n", failures);
    exit(1);
  }

  printf("ALL COVERAGE READ TESTS PASSED\n");
  exit(0);
}
