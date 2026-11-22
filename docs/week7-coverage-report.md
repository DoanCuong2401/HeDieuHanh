# Báo cáo nghiệm thu Kernel Coverage tuần 7

**Đề tài 26134 — System Call Fuzzer and Kernel Sanitizers**

**Sinh viên** Đinh Anh Minh — 20233522

**Nhánh** `minh/week7` · **Nền tuần 6** `6494b2e` · **Mốc GitHub main** `bd319c5`

## 1. Phạm vi nghiệm thu

Nhánh tuần 7 ghép công việc tuần 6 với `main` tại `bd319c5`. Mốc `main` này chứa fuzzer tuần 4 và coverage tuần 5. Bộ nghiệm thu chạy các ca coverage cố định và hồi quy xv6 trên nhánh tuần 7. Kết quả áp dụng cho cây mã đã kiểm thử tại hai cấu hình QEMU. Các thay đổi trên nhánh khác của nhóm cần được kiểm tra lại khi vào bản chung.

## 2. Chạy lại trên Ubuntu

Chuẩn bị GCC RISC-V, GNU Make, Python 3, `bc` và QEMU RISC-V theo README của repo. Trong thư mục gốc của repo ở nhánh `minh/week7`, chạy:

```bash
CPUS=1 ./test-xv6.py coverage &&
cp test-xv6.out coverage-cpu1.txt
CPUS=4 ./test-xv6.py coverage &&
cp test-xv6.out coverage-cpu4.txt
```

Chạy từng cấu hình theo thứ tự và chỉ sao chép log khi lệnh báo `ALL WEEK 7 COVERAGE CHECKS PASSED`. Mỗi lượt lưu console vào `test-xv6.out`. Trình chạy dùng ảnh đĩa tạm trong QEMU để phiên nghiệm thu không ghi các thay đổi của guest vào ảnh `fs.img` gốc.

Trình chạy đợi shell xv6, chạy `covdemo` hai lượt và so sánh từng dòng kết quả với số liệu kỳ vọng. Sau đó nó chạy lần lượt `coverage_test`, `covchildtest`, `covreadtest`, `covapitest`, `covexectest` và `usertests -q`. Nó kiểm tra kết quả mới của từng lệnh, lưu log khi có sai lệch hoặc quá thời gian, và trả mã thoát khác 0 khi thất bại.

Để demo bằng tay, chạy `make CPUS=1 qemu` trong Ubuntu rồi gõ `covdemo` trong shell xv6. Thoát QEMU bằng `Ctrl+A` rồi `X`.

## 3. Kết quả và đối chiếu

Cùng một bộ kết quả xuất hiện trong cả hai lượt demo trên mỗi cấu hình:

| Ca | Chuỗi mã điểm | Ghi nhận | Duy nhất | Cờ tràn |
|---|---|---:|---:|---:|
| `file` | `1 261 4 8` | 4 | 4 | 0 |
| `file-repeat` | `1 261 4 8 1 261 4 8` | 8 | 4 | 0 |
| `pipe` | `1 261 4 6` | 4 | 4 | 0 |
| Tổng hợp · 3/3 bản ghi | Hợp mã điểm `1, 4, 6, 8, 261` | 16 | 5 | 0 |

| Kiểm tra | 1 CPU | 4 CPU |
|---|---|---|
| Hai lượt demo đối chiếu đúng chuỗi và số liệu | PASS | PASS |
| Năm chương trình hồi quy coverage | 44 kiểm tra PASS | 44 kiểm tra PASS |
| Hồi quy nhanh xv6 | `ALL TESTS PASSED` | `ALL TESTS PASSED` |

Kiểm tra nhánh thất bại bằng một bản sao trình chạy đặt kỳ vọng sai cho tổng số mã điểm. Trình chạy báo sai lệch ngay ở lượt đầu và trả mã thoát 1.

Môi trường kiểm thử dùng GCC RISC-V 14.2.0, QEMU 10.0.13 và RAM 128 MiB. Minh chứng lưu tại `logs/week7/cpu1.txt` và `logs/week7/cpu4.txt`.

## 4. Phạm vi các điểm đã đo

Mã 1 là tiếp nhận syscall hợp lệ, 261 là `read`, 4 là chuyển tiếp yêu cầu đọc, 8 là đọc inode và 6 là đọc pipe. Các bài hồi quy kiểm tra thêm hành vi API, quyền đọc buffer, vòng đời tiến trình và tràn bộ đệm. Đơn vị báo cáo là số mã điểm đã quan sát từ các vị trí gắn đo thủ công.

## 5. Bàn giao

Tuần 7 bàn giao một lệnh nghiệm thu, hai log console và tài liệu hướng dẫn chạy lại. Phần Kernel Coverage đạt mốc nghiệm thu trên nền GitHub `main` đã nêu. Khi nhóm đổi commit nền hoặc đưa thêm module vào bản chung, chạy lại hai cấu hình và lưu log theo commit mới.
