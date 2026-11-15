# Week 4 - Fuzzer Test Cases

## TC Group 1: File operations

Sequence:

    open(O_CREATE | O_RDWR)
    write()
    close()
    open(O_RDONLY)
    read()
    close()
    unlink()

Expected:
- File creation succeeds.
- Write and read return the expected byte count.
- File can be reopened.
- File can be removed.

## TC Group 2: Invalid file descriptors

Operations:

    close(invalid_fd)
    read(invalid_fd, buffer, 0)

Expected:
- Both operations return -1.

## TC Group 3: Pipe operations

Sequence:

    pipe()
    write(write_fd, data, 8)
    read(read_fd, buffer, 8)
    close(read_fd)
    close(write_fd)

Expected:
- Pipe creation succeeds.
- Write and read return 8.
- Both descriptors can be closed.

## TC Group 4: Process operations

Sequence:

    fork()
    child: getpid(), exit()
    parent: wait()

Expected:
- fork returns a positive child PID in the parent.
- Child exits.
- Parent successfully waits for the child.

## TC Group 5: Argument edge cases

Operations:

    open("/no-such-fuzzer-file", O_RDONLY)
    read(fd, buffer, 0)
    read(fd, buffer, 1)

Expected:
- Opening a missing file returns -1.
- Reading zero bytes returns 0.
- Reading one byte from a valid file returns the number of bytes read.

## Replay commands

    fuzzer 12345 20
    fuzzer 12345 20
    fuzzer 67890 20
    fuzzer 12345 100
