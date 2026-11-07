#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "coverage.h"

/* Process table and parent-child relationship lock from proc.c. */
extern struct proc proc[NPROC];
extern struct spinlock wait_lock;

typedef char cov_buffer_size_check[
  COV_CAPACITY * sizeof(uint) <= PGSIZE ? 1 : -1
];

/* Called before allocproc() can fail and call freeproc(). */
void
covprocinit(struct proc *p)
{
  p->cov_buffer = 0;
  p->cov_count = 0;
  p->cov_capacity = 0;
  p->cov_overflow = 0;
  p->cov_enabled = 0;
}

/* Called by freeproc() with p->lock held. */
void
covprocfree(struct proc *p)
{
  p->cov_enabled = 0;

  if (p->cov_buffer != 0)
    kfree((void *)p->cov_buffer);

  covprocinit(p);
}

/* Control coverage of the calling process only. */
int
covcontrol(int op)
{
  struct proc *p = myproc();

  if (p == 0)
    return COV_ERR;

  switch (op) {
  case COV_RESET:
    if (p->cov_enabled)
      return COV_BUSY;

    p->cov_count = 0;
    p->cov_overflow = 0;
    return COV_OK;

  case COV_ENABLE:
    if (p->cov_buffer == 0) {
      uint *buffer = (uint *)kalloc();

      if (buffer == 0)
        return COV_ERR;

      memset(buffer, 0, PGSIZE);

      p->cov_buffer = buffer;
      p->cov_count = 0;
      p->cov_capacity = COV_CAPACITY;
      p->cov_overflow = 0;
    }

    p->cov_enabled = 1;
    return COV_OK;

  case COV_DISABLE:
    p->cov_enabled = 0;
    return COV_OK;

  default:
    return COV_ERR;
  }
}

/*
 * Called only from manually selected syscall execution paths.
 * Do not call from interrupt handlers or background kernel work.
 */
void
covhit(uint point_id)
{
  struct proc *p = myproc();

  if (p == 0 || !p->cov_enabled || p->cov_buffer == 0)
    return;

  if (p->cov_count >= p->cov_capacity) {
    p->cov_overflow = 1;
    return;
  }

  p->cov_buffer[p->cov_count] = point_id;
  p->cov_count++;
}

/*
 * Copy a stable buffer into a kernel snapshot.
 *
 * Caller holds target->lock. Target must be either:
 * - the calling process with collection disabled; or
 * - a ZOMBIE child.
 *
 * No allocation, user copy, or sleeping is allowed here.
 */
static int
covsnapshot(struct proc *target, struct cov_header *header,
            uint *points)
{
  if (target->cov_buffer == 0)
    return COV_NO_BUFFER;

  if (target->cov_capacity != COV_CAPACITY ||
      target->cov_count > target->cov_capacity)
    return COV_ERR;

  header->version = COV_VERSION;
  header->count = target->cov_count;
  header->capacity = target->cov_capacity;
  header->overflow = target->cov_overflow ? 1 : 0;

  memmove(points, target->cov_buffer,
          header->count * sizeof(uint));

  return COV_OK;
}

/*
 * pid == 0: read the calling process with collection disabled.
 * pid > 0: read a direct ZOMBIE child before wait() reaps it.
 *
 * Return bytes copied on success, or a COV_* error.
 * Reading never clears the source buffer or reaps the child.
 */
int
covread(int pid, uint64 dst, int buf_bytes)
{
  struct proc *caller = myproc();
  struct cov_header header;
  uint *points;
  int result;
  uint point_bytes;
  uint total_bytes;

  if (caller == 0 || pid < 0 || buf_bytes < 0)
    return COV_ERR;

  /*
   * Allocate only the point snapshot on the heap.
   * The small header remains on the stack.
   */
  points = (uint *)kalloc();
  if (points == 0)
    return COV_ERR;

  if (pid == 0) {
    acquire(&caller->lock);

    if (caller->cov_enabled)
      result = COV_BUSY;
    else
      result = covsnapshot(caller, &header, points);

    release(&caller->lock);
  } else {
    result = COV_ERR;

    /*
     * Same lock order as the process lifecycle:
     * wait_lock before an individual process lock.
     */
    acquire(&wait_lock);

    for (struct proc *target = proc;
         target < &proc[NPROC]; target++) {
      acquire(&target->lock);

      if (target->pid == pid && target->parent == caller) {
        if (target->state != ZOMBIE)
          result = COV_BUSY;
        else
          result = covsnapshot(target, &header, points);

        release(&target->lock);
        break;
      }

      release(&target->lock);
    }

    release(&wait_lock);
  }

  if (result != COV_OK) {
    kfree((void *)points);
    return result;
  }

  point_bytes = header.count * sizeof(uint);
  total_bytes = sizeof(header) + point_bytes;

  if ((uint)buf_bytes < total_bytes) {
    kfree((void *)points);
    return COV_TOO_SMALL;
  }

  /* Validate the complete destination range without overflow. */
  if (dst >= caller->sz ||
      (uint64)total_bytes > caller->sz - dst) {
    kfree((void *)points);
    return COV_ERR;
  }

  /*
   * All spinlocks have been released before copyout().
   * A copy failure may leave partial data in user space.
   * User code must accept data only after a successful return.
   */
  if (copyout(caller->pagetable, caller->sz, dst,
              (char *)&header, sizeof(header)) < 0) {
    kfree((void *)points);
    return COV_ERR;
  }

  if (point_bytes != 0 &&
      copyout(caller->pagetable, caller->sz,
              dst + sizeof(header),
              (char *)points, point_bytes) < 0) {
    kfree((void *)points);
    return COV_ERR;
  }

  kfree((void *)points);
  return (int)total_bytes;
}
