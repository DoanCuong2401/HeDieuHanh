#include "kernel/types.h"
#include "kernel/coverage.h"
#include "kernel/syscall.h"
#include "user/user.h"

static struct cov_result result;
static struct cov_result saved;
static int failures;

static void
check(int ok, const char *name)
{
  if (ok) {
    printf("PASS: %s\n", name);
  } else {
    printf("FAIL: %s\n", name);
    failures++;
  }
}

static void
fatal(const char *name)
{
  cov_ctl(COV_DISABLE);
  printf("FAIL: %s\n", name);
  exit(1);
}

static void
collect_getpid(void)
{
  if (cov_ctl(COV_ENABLE) != COV_OK)
    fatal("enable coverage");

  getpid();

  if (cov_ctl(COV_DISABLE) != COV_OK)
    fatal("disable coverage");
}

int
main(void)
{
  int n, again, gate[2], pid, status;
  int parent_pid = getpid();

  collect_getpid();
  n = cov_read(0, &saved, sizeof(saved));
  if (n != (int)(sizeof(struct cov_header) + 2 * sizeof(uint)))
    fatal("prepare coverage snapshot");

  check(cov_read(0, &result, -1) == COV_ERR,
        "reject negative receive size");

  check(cov_read(0, &result, 0) == COV_TOO_SMALL,
        "reject zero receive size");

  /*
   * Address zero belongs to read-only user text in this xv6.
   * This exercises copyout permission checking.
   */
  check(cov_read(0, (void *)0, sizeof(result)) == COV_ERR,
        "reject copyout into read-only text");

  check(cov_read(0, sbrk(0), sizeof(result)) == COV_ERR,
        "reject destination at end of user memory");

  again = cov_read(0, &result, sizeof(result));
  check(again == n && memcmp(&saved, &result, (uint)n) == 0,
        "failed copies preserve source coverage");

  /* Re-enable appends unless RESET is explicitly requested. */
  collect_getpid();
  n = cov_read(0, &result, sizeof(result));
  check(n == (int)(sizeof(struct cov_header) + 4 * sizeof(uint)) &&
        result.header.count == 4 &&
        result.point_ids[0] == COV_DISPATCH_VALID &&
        result.point_ids[1] == COV_SYSCALL_BASE + SYS_getpid &&
        result.point_ids[2] == COV_DISPATCH_VALID &&
        result.point_ids[3] == COV_SYSCALL_BASE + SYS_getpid,
        "re-enable preserves previous points");

  int reset_ok = 1;
  for (int i = 0; i < 50; i++) {
    if (cov_ctl(COV_RESET) != COV_OK)
      fatal("reset coverage");

    collect_getpid();
    n = cov_read(0, &result, sizeof(result));

    if (n != (int)(sizeof(struct cov_header) + 2 * sizeof(uint)) ||
        result.header.version != COV_VERSION ||
        result.header.capacity != COV_CAPACITY ||
        result.header.count != 2 ||
        result.header.overflow != 0 ||
        result.point_ids[0] != COV_DISPATCH_VALID ||
        result.point_ids[1] != COV_SYSCALL_BASE + SYS_getpid)
      reset_ok = 0;
  }
  check(reset_ok, "50 reset and reuse cycles");

  if (pipe(gate) < 0)
    fatal("create synchronization pipe");

  pid = fork();
  if (pid < 0) {
    close(gate[0]);
    close(gate[1]);
    fatal("fork child");
  }

  if (pid == 0) {
    char token;

    close(gate[1]);

    /*
     * A child must not read its parent's coverage.
     * Parent has a valid buffer, so this tests access control.
     */
    int denied = cov_read(parent_pid, &result, sizeof(result));

    /*
     * The parent keeps the write end open without sending data
     * until it has checked COV_BUSY.
     */
    if (read(gate[0], &token, 1) != 1)
      exit(11);
    close(gate[0]);

    if (denied != COV_ERR)
      exit(12);

    if (cov_ctl(COV_ENABLE) != COV_OK)
      exit(13);

    getpid();
    exit(0);
  }

  close(gate[0]);

  check(cov_read(pid, &result, sizeof(result)) == COV_BUSY,
        "reject reading child before it exits");

  int released = write(gate[1], "G", 1);
  close(gate[1]);

  if (released != 1) {
    kill(pid);
    wait(&status);
    fatal("release child");
  }

  n = COV_BUSY;
  for (int i = 0; i < 300; i++) {
    n = cov_read(pid, &result, sizeof(result));
    if (n != COV_BUSY)
      break;
    pause(1);
  }

  if (n == COV_BUSY) {
    kill(pid);
    wait(&status);
    fatal("child polling timeout");
  }

  check(n == (int)(sizeof(struct cov_header) + 4 * sizeof(uint)) &&
        result.header.count == 4 &&
        result.header.overflow == 0 &&
        result.point_ids[0] == COV_DISPATCH_VALID &&
        result.point_ids[1] == COV_SYSCALL_BASE + SYS_getpid &&
        result.point_ids[2] == COV_DISPATCH_VALID &&
        result.point_ids[3] == COV_SYSCALL_BASE + SYS_exit,
        "read child after synchronized completion");

  status = -1;
  int reaped = wait(&status);
  check(reaped == pid && status == 0,
        "child cannot read parent coverage");

  check(cov_read(pid, &result, sizeof(result)) == COV_ERR,
        "reject child PID after reaping");

  if (failures) {
    printf("COVERAGE API TESTS FAILED: %d\n", failures);
    exit(1);
  }

  printf("ALL COVERAGE API TESTS PASSED\n");
  exit(0);
}
