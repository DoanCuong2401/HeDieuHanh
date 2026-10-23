# xv6-RISC-V System Call Inventory

**Project:** 26134 — System Call Fuzzer and Kernel Sanitizers  
**Phase:** Week 1 — Source-code inspection  
**Owner:** Cường  
**Source:** Current local `xv6-riscv` checkout

## 1. Objective

Inventory the system calls registered in the current xv6-RISC-V source tree. Classify them by function and identify their argument types and resource requirements to prepare the grammar-based syscall fuzzer.

This document describes the current source code, including local customizations. It is not a claim that the fuzzer or kernel sanitizer has already been implemented.

## 2. System call number inventory

The current `kernel/syscall.h` defines 23 system call numbers, from 1 through 23.

| Number | System call | Category |
|---:|---|---|
| 1 | `fork` | Process |
| 2 | `exit` | Process |
| 3 | `wait` | Process |
| 4 | `pipe` | File descriptors / IPC |
| 5 | `read` | File I/O |
| 6 | `kill` | Process |
| 7 | `exec` | Process / program loading |
| 8 | `fstat` | File metadata |
| 9 | `chdir` | Filesystem |
| 10 | `dup` | File descriptors |
| 11 | `getpid` | Process |
| 12 | `sbrk` | Memory |
| 13 | `pause` | Process scheduling |
| 14 | `uptime` | System information |
| 15 | `open` | File I/O |
| 16 | `write` | File I/O |
| 17 | `mknod` | Filesystem |
| 18 | `unlink` | Filesystem |
| 19 | `link` | Filesystem |
| 20 | `mkdir` | Filesystem |
| 21 | `close` | File descriptors |
| 22 | `sync` | Custom or version-specific; implementation requires inspection |
| 23 | `hello` | Local custom syscall |

The dispatch table in `kernel/syscall.c` maps these syscall numbers to their corresponding `sys_*` handlers.

## 3. Argument and resource model

The following table is based on the inspected handler implementations and the standard user-level interfaces. The `sync` implementation and its user-level interface still need verification.

| System call | Arguments | Important resource or constraint |
|---|---|---|
| `fork` | None | Creates a child process |
| `exit` | Status: integer | Terminates the calling process; does not return normally |
| `wait` | Status pointer | Waits for a child and may write its exit status |
| `pipe` | Pointer to two integers | Returns read and write file descriptors |
| `read` | FD, buffer pointer, byte count | FD must be valid; buffer must be accessible |
| `kill` | PID: integer | Target PID should be selected carefully |
| `exec` | Path string, argument-vector pointer | Replaces the current process image on success |
| `fstat` | FD, `struct stat` pointer | Requires valid FD and writable status buffer |
| `chdir` | Path string | Path must identify a directory |
| `dup` | FD | Requires a valid open FD; allocates another descriptor |
| `getpid` | None | Returns the current process ID |
| `sbrk` | Byte count, allocation mode | Current implementation reads two integer arguments |
| `pause` | Tick count: integer | Negative values are clamped to zero |
| `uptime` | None | Returns elapsed clock ticks |
| `open` | Path string, mode: integer | May create a file depending on mode |
| `write` | FD, buffer pointer, byte count | FD must be valid and writable |
| `mknod` | Path string, major, minor | Creates a device node |
| `unlink` | Path string | Removes a filesystem link, subject to filesystem rules |
| `link` | Old path string, new path string | Both paths must be valid; directories are rejected |
| `mkdir` | Path string | Creates a directory |
| `close` | FD | FD must refer to an open descriptor |
| `sync` | To be verified | Inspect implementation and user wrapper before fuzzing |
| `hello` | No explicit user arguments in handler | Prints a kernel message and returns zero |

Pointer validity, string termination, argument limits, and return values should be checked against the current implementation before generating malformed inputs.

## 4. Initial seed sequences

The first seeds should emphasize valid resource lifecycles. Each sequence is a proposed test, not yet an implemented fuzzer test.

1. `open → read → close`
   - Open a known readable file.
   - Read a bounded number of bytes into a valid buffer.
   - Close the returned descriptor.

2. `open → write → close`
   - Open or create a test file with a suitable mode.
   - Write a bounded buffer.
   - Close the descriptor.

3. `open → dup → close`
   - Open a file.
   - Duplicate its descriptor.
   - Close both descriptors exactly once.

4. `pipe → close`
   - Create a pipe.
   - Close both returned descriptors.
   - Later extend this to read/write communication.

5. `fork → child exit → parent wait`
   - Create a child process.
   - Have the child terminate.
   - Have the parent wait for the child.

6. `sbrk → memory access → process exit`
   - Request a small, bounded memory increment using the correct mode.
   - Access only memory that is valid under the current allocation mode.
   - Exit normally.

The resource tracker must record which file descriptors and child processes are live. It must avoid closing the same descriptor twice in a supposedly valid seed, or waiting indefinitely for a process that cannot terminate.

## 5. Grammar-based generation plan

The generator will initially produce sequences from syscall categories and resource-aware rules.

- **Process rules:** `fork`, child termination, `wait`, and safe PID selection.
- **File rules:** open a known file, use the returned descriptor, then close it.
- **Memory rules:** bounded `sbrk` increments and accesses within allocated memory.
- **Pipe rules:** create a pipe, track both endpoints, and use them with appropriate read/write ordering.
- **Filesystem rules:** create and remove test paths while respecting dependencies.
- **Negative tests:** invalid descriptors, invalid paths, boundary lengths, and carefully controlled invalid pointers.

Generation should start with a small maximum sequence length and a bounded execution timeout. More complex sequences can be added after deterministic seeds are reliable.

## 6. Replay and reproducibility

Every test case should record:

- Random seed or test-case identifier.
- Ordered syscall operations and their generated arguments.
- Resource-tracker state needed to interpret the sequence.
- Exit status, timeout, or kernel failure information.
- Coverage information when the coverage component becomes available.

A failing test should be saved so it can be replayed deterministically. A later minimization stage should attempt to remove operations and simplify arguments while preserving the failure.

## 7. Candidate bug scenarios

These are hypotheses for future tests, not confirmed vulnerabilities.

1. **Invalid file descriptor handling:** call `read`, `write`, `dup`, `fstat`, or `close` with invalid or already-closed descriptors.
2. **Filesystem boundary handling:** test long paths, missing paths, repeated creation/removal, and invalid path combinations.
3. **Process lifecycle errors:** exercise repeated `fork`, child exit, and `wait` sequences with bounded process counts.
4. **Memory growth boundaries:** test zero, small, negative, and large `sbrk` increments under controlled conditions.
5. **Pipe lifecycle errors:** create pipes, close endpoints in different orders, and test reads/writes with correct timeout handling.

A scenario is not a discovered bug until a reproducible failure and supporting evidence are recorded.

## 8. Open questions and next steps

- Inspect the implementation and user wrapper for `sync`.
- Confirm the user-level syscall prototypes in `user/user.h`.
- Confirm the build and boot procedure used by the team.
- Agree on the interface between the fuzzer, coverage component, and sanitizer.
- Add branch or basic-block coverage only when actual instrumentation is implemented; syscall invocation counts alone are syscall coverage, not branch coverage.
- Preserve the existing local `hello` changes until their status and ownership are confirmed.

## 9. Evidence collected

The initial inventory was collected from:

- `kernel/syscall.h`
- `kernel/syscall.c`
- `kernel/sysproc.c`
- `kernel/sysfile.c`

The inspected source reports `SYS_hello = 23` and `SYS_sync = 22`. Further source inspection is required before treating all user-facing prototypes and the `sync` behavior as verified.
