# BÁO CÁO CÁ NHÂN – WEEK 7

**Project 26134:** System Call Fuzzer and Kernel Sanitizers  
**Thành viên:** Trần Tiến Đạt – 20233313  
**Phụ trách:** Memory Sanitizer & Reliability

## 1. Mục tiêu

Hoàn thiện bộ kiểm thử Memory Sanitizer trên xv6-RISC-V, đánh giá khả năng phát hiện lỗi bộ nhớ, kiểm tra false positive, theo dõi trang chưa giải phóng và kiểm thử hồi quy.

## 2. Công việc thực hiện

- Bổ sung hàm `sanitizer_week7_planted_test()` trong `kernel/sanitizer_test.c`.
- Tạo các tình huống kiểm thử SAN-T07 đến SAN-T10.
- Thêm tùy chọn `SAN_W7_TEST=1` vào Makefile để tách bản kiểm thử khỏi bản build thông thường.
- Tích hợp bài kiểm thử vào `kernel/main.c`.
- Khôi phục giá trị poison sau mỗi bài kiểm thử để tránh ảnh hưởng đến Free List.

## 3. Kết quả kiểm thử

| Test | Nội dung | Kết quả |
|---|---|---|
| SAN-T03 | Phát hiện write-after-free khi cấp phát lại | PASS – kernel báo SAN-EVENT và panic |
| SAN-T07 | Phát hiện lỗi tại byte 200 | PASS |
| SAN-T08 | Phát hiện vị trí lỗi đầu tiên trong nhiều byte bị sửa | PASS |
| SAN-T09 | Không báo lỗi giả khi poison hợp lệ | PASS |
| SAN-T10 | Theo dõi trang đang cấp phát và kiểm tra khôi phục | PASS |

**Kiểm thử hồi quy:** Bản build bình thường với `CPUS=1` chạy `usertests` và đạt `ALL TESTS PASSED`.

## 4. Đánh giá

Memory Sanitizer phát hiện được các tình huống thay đổi poison trên trang đã giải phóng. Các bài kiểm thử T07 và T08 cho phép xác nhận vị trí lỗi mà không cần gây kernel panic.

SAN-T09 kiểm tra trường hợp poison hợp lệ không bị báo lỗi giả. SAN-T10 kiểm tra chênh lệch số trang trống trước, trong và sau khi cấp phát.

## 5. Giới hạn

- Các tình huống T03, T07 và T08 đều thuộc nhóm lỗi truy cập trang đã giải phóng, không phải ba loại lỗi bộ nhớ độc lập.
- Sanitizer hiện chưa bao phủ đầy đủ buffer overflow trên trang đang sử dụng, double-free và mọi trường hợp use-after-free.
- SAN-T10 chỉ theo dõi outstanding allocation bằng bộ đếm trang trống, chưa phải hệ thống tự động truy vết memory leak.
- Các bài planted bug được tạo có chủ đích để kiểm thử, không phải lỗi mới do Fuzzer tự tìm ra.

## 6. Kết luận

Hoàn thiện bộ kiểm thử Sanitizer Week 7 với các tình huống phát hiện corruption, false positive và theo dõi cấp phát bộ nhớ. Bản build thông thường vẫn hoạt động sau khi tách bài kiểm thử bằng cờ biên dịch.

**Trạng thái:** Hoàn thành phần kiểm thử cá nhân Week 7; kết quả kiểm thử và giới hạn kỹ thuật được ghi nhận để phục vụ báo cáo, demo cuối kỳ.