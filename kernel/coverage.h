#ifndef XV6_COVERAGE_H
#define XV6_COVERAGE_H

/*
 * Include kernel/types.h before this header.
 *
 * Shared constants and result format for the coverage prototype.
 * These are NOT syscall numbers.
 */

#define COV_VERSION 1
#define COV_CAPACITY 1024

/* Operations for cov_ctl(). */
#define COV_RESET   1
#define COV_ENABLE  2
#define COV_DISABLE 3

/* API return values. */
#define COV_OK          0
#define COV_ERR        -1
#define COV_BUSY       -2
#define COV_TOO_SMALL  -3
#define COV_NO_BUFFER  -4

/* Manually assigned coverage points. */
#define COV_DISPATCH_VALID    1
#define COV_DISPATCH_INVALID  2
#define COV_READ_BAD_FD       3
#define COV_READ_FORWARD      4
#define COV_FILE_READ_REJECT  5
#define COV_FILE_READ_PIPE    6
#define COV_FILE_READ_DEVICE  7
#define COV_FILE_READ_INODE   8
#define COV_FILE_READ_BAD_DEV 9

/*
 * Additional dispatch point: COV_SYSCALL_BASE + syscall number.
 * This distinguishes getpid, uptime, pause, etc.
 * Coverage control/read syscalls must be excluded.
 */
#define COV_SYSCALL_BASE 256

/*
 * Header copied to user space before the point IDs.
 * uint must be 32 bits for this version of the interface.
 */
struct cov_header {
  uint version;
  uint count;
  uint capacity;
  uint overflow;
};

/*
 * Convenient maximum-sized receive structure.
 * cov_read() copies only the header and count valid point IDs.
 */
struct cov_result {
  struct cov_header header;
  uint point_ids[COV_CAPACITY];
};

/* Compile-time checks for the shared data format. */
typedef char cov_uint_size_check[
  sizeof(uint) == 4 ? 1 : -1
];

typedef char cov_header_size_check[
  sizeof(struct cov_header) == 16 ? 1 : -1
];

typedef char cov_result_size_check[
  sizeof(struct cov_result) == 4112 ? 1 : -1
];

#endif
