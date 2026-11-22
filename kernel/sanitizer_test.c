#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "sanitizer.h"

void
sanitizer_selftest(void)
{
    char *page = (char *)kalloc();

    if (page == 0) {
        printk("[SAN-TEST] FAIL: Cannot allocate page\n");
        return;
    }

    // T01: Kiểm tra trang có poison hợp lệ
    memset(page, 1, PGSIZE);

    // Giả lập vùng metadata được phép thay đổi
    page[0] = 0x55;

    int result = san_check_poison(page, sizeof(void *));

    if (result == -1)
        printk("[SAN-T01] PASS: Valid poison\n");
    else
        printk("[SAN-T01] FAIL: offset=%d\n", result);

    // T02: Giả lập dữ liệu poison bị thay đổi
    page[100] = 0x63;

    result = san_check_poison(page, sizeof(void *));

    if (result == 100)
        printk("[SAN-T02] PASS: Corruption detected\n");
    else
        printk("[SAN-T02] FAIL: offset=%d\n", result);

    // Khôi phục và giải phóng trang
    page[100] = 1;
    kfree(page);

    printk("[SAN-TEST] Finished\n");
}

void
sanitizer_integration_test(void)
{
    char *page = (char *)kalloc();

    if (page == 0) {
        printk("[SAN-T03] FAIL: Cannot allocate\n");
        return;
    }

    printk("[SAN-T03] Starting integration test\n");

    kfree(page);

    // Thay đổi poison sau khi giải phóng
    ((volatile uchar *)page)[100] = 0x63;

    printk("[DEBUG] page=%p byte100=0x%x\n",
           (void *)page,
           (uint)((volatile uchar *)page)[100]);

    // Cấp phát lại
    char *new_page = (char *)kalloc();

    printk("[DEBUG] new_page=%p\n", (void *)new_page);

    if (new_page == page)
        printk("[DEBUG] Reallocated same page\n");
    else
        printk("[DEBUG] Different page\n");

    printk("[SAN-T03] FAIL: Corruption not detected\n");
}


void
sanitizer_week4_selftest(void)
{
    int n0, n1, n2;
    char *p, *q, *a, *b;

    // SAN-T04: Test normal free and reallocation
    p = (char *)kalloc();

    if (p == 0) {
        printk("[SAN-T04] FAIL: allocation failed\n");
        return;
    }

    kfree(p);
    q = (char *)kalloc();

    if (q == 0) {
        printk("[SAN-T04] FAIL: reallocation failed\n");
        return;
    }

    if (q == p)
        printk("[SAN-T04] PASS: No false positive\n");
    else
        printk("[SAN-T04] FAIL: unexpected page\n");

    kfree(q);

    // SAN-T05: Test free page counter
    n0 = san_freepage_count();

    p = (char *)kalloc();

    if (p == 0) {
        printk("[SAN-T05] FAIL: allocation failed\n");
        return;
    }

    n1 = san_freepage_count();
    kfree(p);
    n2 = san_freepage_count();

    if (n1 == n0 - 1 && n2 == n0)
        printk("[SAN-T05] PASS: %d -> %d -> %d\n",
               n0, n1, n2);
    else
        printk("[SAN-T05] FAIL: %d -> %d -> %d\n",
               n0, n1, n2);

    // SAN-T06: Simulate an outstanding allocation
    n0 = san_freepage_count();

    a = (char *)kalloc();
    b = (char *)kalloc();

    if (a == 0 || b == 0) {
        if (a)
            kfree(a);
        if (b)
            kfree(b);
        printk("[SAN-T06] FAIL: allocation failed\n");
        return;
    }

    // Return only one of the two pages
    kfree(a);
    n1 = san_freepage_count();

    // Restore the remaining page
    kfree(b);
    n2 = san_freepage_count();

    if (n1 == n0 - 1 && n2 == n0)
        printk("[SAN-T06] PASS: Outstanding page detected\n");
    else
        printk("[SAN-T06] FAIL: unexpected free count\n");

    printk("[SAN-WEEK4] Finished\n");
}