#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "sanitizer.h"

#define SAN_POISON 0x01

int
san_check_poison(void *page, uint metadata_bytes)
{
    uchar *p = (uchar *)page;

    for (uint i = metadata_bytes; i < PGSIZE; i++) {
        if (p[i] != SAN_POISON) {
            return i;
        }
    }

    return -1;
}

void
san_report_corruption(void *page, int offset, uint actual)
{
    printk("[SANITIZER] FREED_PAGE_CORRUPTION\n");

    printk("page=%p offset=%d expected=0x%x actual=0x%x\n",
        (void *)page, offset, (uint)SAN_POISON, actual);

    panic("memory sanitizer");
}