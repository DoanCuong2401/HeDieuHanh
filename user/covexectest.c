#include "kernel/types.h"
#include "kernel/coverage.h"
#include "kernel/syscall.h"
#include "user/user.h"

static struct cov_result result;

static int
check_points(uint *expected, uint count)
{
  int n = cov_read(0, &result, sizeof(result));

  if (n != (int)(sizeof(struct cov_header) + count * sizeof(uint)) ||
      result.header.version != COV_VERSION ||
      result.header.capacity != COV_CAPACITY ||
      result.header.count != count ||
      result.header.overflow != 0)
    return 0;

  for (uint i = 0; i < count; i++)
    if (result.point_ids[i] != expected[i])
      return 0;

  return 1;
}

int
main(int argc, char *argv[])
{
  /*
   * Executed in the new user image.
   * Coverage must still contain getpid and exec from before exec.
   */
  if (argc == 2 && strcmp(argv[1], "afterexec") == 0) {
    uint expected[] = {
      COV_DISPATCH_VALID, COV_SYSCALL_BASE + SYS_getpid,
      COV_DISPATCH_VALID, COV_SYSCALL_BASE + SYS_exec,
      COV_DISPATCH_VALID, COV_SYSCALL_BASE + SYS_getpid
    };

    getpid();

    if (cov_ctl(COV_DISABLE) != COV_OK)
      exit(20);

    exit(check_points(expected, 6) ? 0 : 21);
  }

  /*
   * Repeated fork/exec/wait exercises initialization and reuse.
   * Each new child must start without a coverage buffer.
   */
  for (int i = 0; i < 32; i++) {
    int pid = fork();
    if (pid < 0) {
      printf("FAIL: fork at cycle %d\n", i + 1);
      exit(1);
    }

    if (pid == 0) {
      char *args[] = {"covexectest", "afterexec", 0};

      if (cov_read(0, &result, sizeof(result)) != COV_NO_BUFFER)
        exit(10);

      if (cov_ctl(COV_ENABLE) != COV_OK)
        exit(11);

      getpid();
      exec("covexectest", args);

      /* Successful exec never returns here. */
      cov_ctl(COV_DISABLE);
      exit(12);
    }

    int status = -1;
    int reaped = wait(&status);
    if (reaped != pid || status != 0) {
      printf("FAIL: cycle %d, child status=%d\n", i + 1, status);
      exit(1);
    }
  }

  printf("PASS: coverage survives successful exec\n");
  printf("PASS: 32 fork exec wait cycles with fresh child state\n");

  /*
   * Executing a directory must fail.
   * That failure must preserve the existing coverage buffer.
   */
  char *badargs[] = {"/", 0};
  uint expected[] = {
    COV_DISPATCH_VALID, COV_SYSCALL_BASE + SYS_getpid,
    COV_DISPATCH_VALID, COV_SYSCALL_BASE + SYS_exec
  };

  if (cov_ctl(COV_RESET) != COV_OK ||
      cov_ctl(COV_ENABLE) != COV_OK) {
    printf("FAIL: prepare failed-exec test\n");
    exit(1);
  }

  getpid();
  int ret = exec("/", badargs);
  int stopped = cov_ctl(COV_DISABLE);

  if (ret != -1 || stopped != COV_OK ||
      !check_points(expected, 4)) {
    printf("FAIL: failed exec preserves coverage\n");
    exit(1);
  }

  printf("PASS: failed exec preserves coverage\n");
  printf("ALL COVERAGE EXEC TESTS PASSED\n");
  exit(0);
}
