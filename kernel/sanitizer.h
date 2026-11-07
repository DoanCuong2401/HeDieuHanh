#ifndef SANITIZER_H
#define SANITIZER_H

int san_check_poison(void *page, uint metadata_bytes);

void san_report_corruption(
    void *page,
    int offset,
    uint actual
) __attribute__((noreturn));

void sanitizer_selftest(void);
void sanitizer_integration_test(void);
#endif