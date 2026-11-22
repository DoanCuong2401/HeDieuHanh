#include "kernel/types.h"
#include "kernel/coverage.h"
#include "kernel/syscall.h"
#include "user/user.h"

/* Keep these large receive buffers off the stack. */
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
  int n, again;
  int enabled, busy_reset, busy_read, disabled;

  check(cov_ctl(999) == COV_ERR, "reject invalid operation");

  check(cov_read(0, &result, sizeof(result)) == COV_NO_BUFFER,
        "no buffer before enable");

  enabled = cov_ctl(COV_ENABLE);
  disabled = cov_ctl(COV_DISABLE);
  check(enabled == COV_OK && disabled == COV_OK,
        "enable and disable");

  n = cov_read(0, &result, sizeof(result));
  check(n == (int)sizeof(struct cov_header) &&
        result.header.version == COV_VERSION &&
        result.header.capacity == COV_CAPACITY &&
        result.header.count == 0 &&
        result.header.overflow == 0,
        "control syscalls do not record coverage");

  check(cov_ctl(COV_RESET) == COV_OK, "reset while disabled");

  enabled = cov_ctl(COV_ENABLE);
  busy_reset = cov_ctl(COV_RESET);
  busy_read = cov_read(0, &result, sizeof(result));
  getpid();
  uptime();
  disabled = cov_ctl(COV_DISABLE);

  check(enabled == COV_OK && disabled == COV_OK,
        "collect getpid and uptime");
  check(busy_reset == COV_BUSY, "reject reset while enabled");
  check(busy_read == COV_BUSY, "reject self read while enabled");

  n = cov_read(0, &result, sizeof(result));
  check(n == (int)(sizeof(struct cov_header) + 4 * sizeof(uint)) &&
        result.header.count == 4 &&
        result.header.overflow == 0 &&
        result.point_ids[0] == COV_DISPATCH_VALID &&
        result.point_ids[1] == COV_SYSCALL_BASE + SYS_getpid &&
        result.point_ids[2] == COV_DISPATCH_VALID &&
        result.point_ids[3] == COV_SYSCALL_BASE + SYS_uptime,
        "record correct point IDs in order");

  check(cov_read(0, &repeated, sizeof(struct cov_header)) ==
        COV_TOO_SMALL,
        "reject receive buffer too small");

  check(cov_read(0, (void *)~(uint64)0, sizeof(result)) == COV_ERR,
        "reject invalid destination");

  again = cov_read(0, &repeated, sizeof(repeated));
  check(n > 0 && again == n &&
        memcmp(&result, &repeated, (uint)n) == 0,
        "reads and failed reads preserve coverage");

  check(cov_ctl(COV_RESET) == COV_OK,
        "reset before overflow test");

  enabled = cov_ctl(COV_ENABLE);
  for (int i = 0; i < 600; i++)
    getpid();
  disabled = cov_ctl(COV_DISABLE);

  n = cov_read(0, &result, sizeof(result));
  int overflow_ok =
    enabled == COV_OK &&
    disabled == COV_OK &&
    n == (int)sizeof(result) &&
    result.header.count == COV_CAPACITY &&
    result.header.overflow == 1;

  if (overflow_ok) {
    for (uint i = 0; i < COV_CAPACITY; i++) {
      uint expected = (i % 2 == 0)
        ? COV_DISPATCH_VALID
        : COV_SYSCALL_BASE + SYS_getpid;
      if (result.point_ids[i] != expected)
        overflow_ok = 0;
    }
  }
  check(overflow_ok, "full buffer preserves prefix and sets overflow");

  check(cov_ctl(COV_RESET) == COV_OK, "reset full buffer");
  n = cov_read(0, &result, sizeof(result));
  check(n == (int)sizeof(struct cov_header) &&
        result.header.count == 0 &&
        result.header.overflow == 0,
        "reset clears count and overflow");

  if (failures) {
    printf("COVERAGE BASIC TESTS FAILED: %d\n", failures);
    exit(1);
  }

  printf("ALL COVERAGE BASIC TESTS PASSED\n");
  exit(0);
}
