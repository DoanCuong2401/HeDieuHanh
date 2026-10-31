# BÁO CÁO TIẾN ĐỘ TUẦN 2 – MEMORY SANITIZER & RELIABILITY

**Đề tài:** 26134 – System Call Fuzzer and Kernel Sanitizers

**Thành viên:** Trần Tiến Đạt

**Module phụ trách:** Memory Sanitizer & Reliability

**Nền tảng:** xv6-RISC-V, QEMU

**Trạng thái:** Bản nháp – Cập nhật theo tiến độ thực tế cuối tuần

## 1. Mục tiêu tuần 2

Tiếp nối kết quả nghiên cứu cơ chế quản lý bộ nhớ vật lý trong Week 1, tuần 2 tập trung vào thiết kế Memory Sanitizer cho hệ điều hành xv6-RISC-V.

Các mục tiêu chính:

- Thiết kế cơ chế Poison/Check/Unpoison.
- Xác định vị trí tích hợp trong kernel.
- Thiết kế cơ chế phát hiện và báo cáo lỗi.
- Xây dựng phương án hạn chế False Positive.
- Đề xuất phương pháp Memory Leak Checking.
- Chuẩn bị Test Plan cho giai đoạn triển khai.

## 2. Nội dung công việc

### 2.1. Thiết kế Memory Sanitizer

Phương án V0.1 được xây dựng dựa trên cơ chế Memory Poisoning đã có trong xv6.

Khi một trang bộ nhớ được giải phóng bằng `kfree()`, mẫu byte `0x01` được ghi vào trang và trang được đưa vào Free List.

Khi `kalloc()` lấy trang để tái cấp phát, sanitizer dự kiến kiểm tra nội dung poison trước khi ghi đè bằng mẫu `0x05`.

Nếu phát hiện byte bị thay đổi bất thường, sanitizer ghi nhận địa chỉ trang và vị trí lỗi.

### 2.2. Thiết kế phòng tránh False Positive

Trong xv6-RISC-V chuẩn, phần đầu trang trống được dùng để lưu metadata của Free List.

Do đó, thuật toán kiểm tra dự kiến bỏ qua vùng metadata hợp lệ và chỉ so sánh dữ liệu trong vùng poison còn lại.

Thiết kế này nhằm tránh trường hợp hệ thống báo lỗi nhầm khi con trỏ `next` được thay đổi hợp lệ.

### 2.3. Thiết kế cơ chế báo lỗi

Khi phát hiện bất thường, hệ thống dự kiến ghi nhận:

- Loại lỗi.
- Địa chỉ trang.
- Vị trí byte bất thường.
- Giá trị mong đợi.
- Giá trị thực tế.

Định dạng log được thiết kế để dễ theo dõi và hỗ trợ việc tích hợp với các module khác trong nhóm.

### 2.4. Thiết kế Memory Leak Checking

Đề xuất xây dựng cơ chế đếm số trang trống trong Free List.

Số trang trống sẽ được ghi nhận trước và sau các bài kiểm thử có kiểm soát nhằm phát hiện dấu hiệu bộ nhớ chưa được giải phóng.

### 2.5. Xây dựng Test Plan

Đã đề xuất các nhóm test:

- Cấp phát và giải phóng hợp lệ.
- Phát hiện thay đổi poison.
- Kiểm tra False Positive.
- Kiểm tra tái cấp phát.
- Kiểm tra số trang trống.
- Kiểm tra dấu hiệu Memory Leak.

Các trường hợp kiểm thử được trình bày chi tiết trong `test-plan.md`.

## 3. Sản phẩm thiết kế dự kiến

| Tài liệu | Nội dung | Trạng thái |
|---|---|---|
| `sanitizer-design.md` | Thiết kế Memory Sanitizer V0.1 | Bản nháp, chờ rà soát |
| `test-plan.md` | Kế hoạch kiểm thử | Bản nháp, chờ rà soát |
| `week2-report.md` | Báo cáo tiến độ tuần 2 | Đang cập nhật |

**Branch làm việc:** `dat/week2`

**Commit/PR:** [Bổ sung sau khi push]

## 4. Kết quả kiểm thử

Trong Week 2, công việc tập trung vào thiết kế.

Chưa có kết quả kiểm thử sanitizer thực tế. Các Test Case và Expected Result hiện mới là kế hoạch dự kiến.

## 5. Khó khăn và vấn đề cần giải quyết

- Cơ chế page poisoning đơn giản không phát hiện được mọi trường hợp Use-After-Free.
- Cần bỏ qua metadata Free List để hạn chế False Positive.
- Cần bảo đảm tính đồng bộ khi bổ sung kiểm tra vào allocator.
- Cần thống nhất định dạng báo lỗi và giao diện tích hợp với Fuzzer và Coverage.
- Cần kiểm soát điều kiện thử nghiệm khi đánh giá dấu hiệu Memory Leak.

## 6. Kế hoạch Week 3

- Triển khai các hàm kiểm tra poison.
- Tạo `kernel/sanitizer.c` và `kernel/sanitizer.h`.
- Tích hợp bước kiểm tra vào `kalloc()`.
- Xây dựng bài kiểm thử kernel đầu tiên.
- Kiểm tra chức năng và ghi nhận kết quả thực nghiệm.
- Cập nhật tài liệu theo kết quả triển khai.

## 7. Kết luận

Week 2 tập trung chuyển từ nghiên cứu cơ chế quản lý bộ nhớ sang thiết kế kỹ thuật Memory Sanitizer.

Phương án V0.1 sử dụng Memory Poisoning và kiểm tra dữ liệu trước khi tái cấp phát, kết hợp với cơ chế cảnh báo và kế hoạch kiểm thử.

Sau khi hoàn thành rà soát và thống nhất giao diện với các thành viên, thiết kế sẽ được sử dụng làm cơ sở triển khai trong Week 3.