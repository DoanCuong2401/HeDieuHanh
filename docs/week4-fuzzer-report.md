# Week 4 Report - Stateful System Call Fuzzer v1

## 1. Objective

Develop a deterministic stateful syscall fuzzer for xv6-RISC-V.
The fuzzer generates syscall testcases using a user-provided seed
and supports replaying the generated testcase sequence.

## 2. Implemented features

- Deterministic pseudo-random testcase generation (xorshift32).
- Configurable seed and testcase count.
- File operation sequences: open, write, close, reopen, read, unlink.
- Pipe operation sequences: pipe, write, read, close.
- Process operation sequences: fork, getpid, exit, wait.
- Invalid file descriptor tests.
- Argument edge tests involving missing paths and read sizes 0 or 1.

## 3. Build and execution

Build:

    make clean
    make fs.img

Run in xv6:

    fuzzer 12345 20
    fuzzer 12345 20
    fuzzer 67890 20
    fuzzer 12345 100

## 4. Test results

| Test | Result |
|------|--------|
| Build fuzzer and filesystem image | PASS |
| Run 20 testcases with seed 12345 | PASS |
| Replay seed 12345 with 20 testcases | PASS |
| Run a different seed 67890 | PASS |
| Run 100 testcases with seed 12345 | PASS |

All observed runs reached FUZZING COMPLETED without a kernel panic.

The same seed reproduced the generated testcase sequence.
A different seed generated a different sequence.

## 5. Limitations

- Replay guarantees deterministic testcase generation, not identical runtime output.
- Process IDs and parent/child console output may differ between runs.
- The current file test checks read/write return lengths but does not compare
  the contents of the written and read buffers.
- This version does not yet implement coverage-guided testcase generation.

## 6. Conclusion

Stateful syscall Fuzzer v1 was built and executed successfully on xv6-RISC-V.
It supports deterministic testcase generation, replay, and basic testing of
file, pipe, process, invalid descriptor, and argument-edge operations.
