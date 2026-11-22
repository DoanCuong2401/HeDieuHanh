# BÁO CÁO TIẾN ĐỘ TUẦN 3 – MEMORY SANITIZER & RELIABILITY

**Đề tài:** 26134 – System Call Fuzzer and Kernel Sanitizers  
**Thành viên:** Trần Tiến Đạt – 20233313  
**Module:** Memory Sanitizer & Reliability  
**Nền tảng:** xv6-RISC-V / QEMU

## 1. Mục tiêu tuần 3

Triển khai prototype Memory Sanitizer dựa trên thiết kế Week 2, tích hợp cơ chế kiểm tra poison vào bộ cấp phát trang vật lý của xv6 và xây dựng các bài kiểm thử chức năng.

## 2. Công việc đã thực hiện

### 2.1. Triển khai Memory Sanitizer

- Tạo `kernel/sanitizer.h` khai báo các hàm kiểm tra và báo lỗi.
- Tạo `kernel/sanitizer.c` hiện thực cơ chế kiểm tra poison.
- Tích hợp `san_check_poison()` vào `kalloc()` để kiểm tra trang trước khi cấp phát lại.
- Bỏ qua vùng metadata của Free List nhằm hạn chế False Positive.
- Bổ sung `sanitizer.o` vào Makefile.

### 2.2. Xây dựng chương trình kiểm thử

Tạo `kernel/sanitizer_test.c` với các bài kiểm thử đơn vị và tích hợp.

Khi phát hiện nội dung poison bất thường, sanitizer ghi địa chỉ trang, offset, giá trị mong đợi và giá trị thực tế, sau đó dừng kernel trong chế độ kiểm thử.

## 3. Kết quả kiểm thử

| Test ID | Nội dung | Kết quả |
|---|---|---|
| SAN-T01 | Kiểm tra mẫu poison hợp lệ và vùng metadata | PASS |
| SAN-T02 | Kiểm tra khả năng phát hiện byte bất thường | PASS |
| SAN-T03 | Kiểm tra phát hiện lỗi khi `kalloc()` cấp phát lại | PASS |

Ở SAN-T03, chương trình kiểm thử có kiểm soát làm thay đổi mẫu poison tại offset 100 của trang đã giải phóng.

Khi trang được cấp phát lại, Memory Sanitizer phát hiện giá trị thực tế `0x63` khác giá trị mong đợi `0x01`, xuất cảnh báo `FREED_PAGE_CORRUPTION` và gọi `panic()`.

Kết quả cho thấy cơ chế kiểm tra poison đã hoạt động trên đường cấp phát thực tế của `kalloc()`.

**Kiểm thử hồi quy:** Bổ sung kết quả `usertests` cuối cùng sau khi tắt bài kiểm thử gây panic.

## 4. Khó khăn và cách xử lý

- Lỗi khai báo hàm giữa các file `.c` và `.h`: bổ sung các prototype phù hợp.
- Lỗi liên kết `undefined reference`: kiểm tra phần định nghĩa hàm và Makefile.
- Lỗi QEMU khóa `fs.img`: đóng phiên QEMU trước khi chạy lại.
- Bài kiểm thử tích hợp ban đầu chưa phát hiện lỗi: bổ sung thông tin debug và kiểm tra lại luồng cấp phát.

## 5. Giới hạn

Prototype hiện chỉ phát hiện một số trường hợp nội dung trang bị thay đổi sau giải phóng và trước khi tái cấp phát.

Chưa triển khai đầy đủ Memory Leak Checking, Double Free Detection hoặc Shadow Memory.

## 6. Kế hoạch tuần 4

- Cải thiện tính ổn định và khả năng quản lý lỗi của Memory Sanitizer.
- Bổ sung cơ chế theo dõi số trang trống phục vụ Leak Checking.
- Mở rộng các bài kiểm thử và đánh giá False Positive.
- Chuẩn bị tích hợp với Fuzzer và Kernel Coverage của nhóm.

## 7. Kết luận

Trong Week 3, đã triển khai prototype Memory Sanitizer, tích hợp bước kiểm tra poison vào `kalloc()` và thực hiện thành công các bài kiểm thử SAN-T01, SAN-T02 và SAN-T03 trên QEMU.

Kết quả là cơ sở để tiếp tục hoàn thiện chức năng phát hiện lỗi bộ nhớ và tích hợp với các module khác ở các tuần tiếp theo.