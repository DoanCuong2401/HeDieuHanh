#include "kernel/types.h"
#include "kernel/coverage.h"
#include "kernel/syscall.h"
#include "user/user.h"

static struct cov_result result;
static struct cov_result repeated;
static int failures;

static void
check(int condition, const char *name)
{
  if (condition) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s\n", name);
    failures++;
  }
}

int
main(void)
{
  int pid, enabled, disabled, n, again, status, reaped;

  enabled = cov_ctl(COV_ENABLE);
  if (enabled != COV_OK) {
    printf("FAIL: enable parent coverage\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    cov_ctl(COV_DISABLE);
    printf("FAIL: fork\n");
    exit(1);
  }

  if (pid == 0) {
    /*
     * The child must start without the parent's buffer
     * and with coverage disabled.
     */
    if (cov_read(0, &result, sizeof(result)) != COV_NO_BUFFER)
      exit(10);

    if (cov_ctl(COV_ENABLE) != COV_OK)
      exit(11);

    getpid();

    /*
     * Do not disable explicitly: exit must freeze the buffer.
     * Expected points: getpid dispatch and exit dispatch.
     */
    exit(0);
  }

  disabled = cov_ctl(COV_DISABLE);
  check(disabled == COV_OK, "disable parent coverage");

  n = cov_read(0, &result, sizeof(result));
  check(n == (int)(sizeof(struct cov_header) + 2 * sizeof(uint)) &&
        result.header.count == 2 &&
        result.point_ids[0] == COV_DISPATCH_VALID &&
        result.point_ids[1] == COV_SYSCALL_BASE + SYS_fork,
        "parent buffer contains only its fork");

  /*
   * Read before wait. A running child returns COV_BUSY.
   * Bound polling so a broken implementation is reported.
   */
  n = COV_BUSY;
  for (int i = 0; i < 300; i++) {
    n = cov_read(pid, &result, sizeof(result));
    if (n != COV_BUSY)
      break;
    pause(1);
  }

  if (n == COV_BUSY) {
    printf("FAIL: child did not become readable within polling limit\n");
    kill(pid);
    wait(&status);
    exit(1);
  }

  check(n == (int)(sizeof(struct cov_header) + 4 * sizeof(uint)) &&
        result.header.version == COV_VERSION &&
        result.header.capacity == COV_CAPACITY &&
        result.header.count == 4 &&
        result.header.overflow == 0 &&
        result.point_ids[0] == COV_DISPATCH_VALID &&
        result.point_ids[1] == COV_SYSCALL_BASE + SYS_getpid &&
        result.point_ids[2] == COV_DISPATCH_VALID &&
        result.point_ids[3] == COV_SYSCALL_BASE + SYS_exit,
        "read zombie child coverage including exit");

  again = cov_read(pid, &repeated, sizeof(repeated));
  check(n > 0 && again == n &&
        memcmp(&result, &repeated, (uint)n) == 0,
        "child coverage remains stable before wait");

  status = -1;
  reaped = wait(&status);
  check(reaped == pid && status == 0,
        "child starts without inherited buffer and exits successfully");

  check(cov_read(pid, &result, sizeof(result)) == COV_ERR,
        "reject child read after wait");

  check(cov_read(-1, &result, sizeof(result)) == COV_ERR,
        "reject negative PID");

  if (failures) {
    printf("COVERAGE CHILD TESTS FAILED: %d\n", failures);
    exit(1);
  }

  printf("ALL COVERAGE CHILD TESTS PASSED\n");
  exit(0);
}
