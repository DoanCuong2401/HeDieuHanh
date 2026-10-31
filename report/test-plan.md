# WEEK 2 – MEMORY SANITIZER TEST PLAN

**Thành viên:** Trần Tiến Đạt  
**Nền tảng:** xv6-RISC-V / QEMU  
**Phiên bản:** Test Plan V0.1  
**Trạng thái:** Chưa thực thi

## 1. Mục tiêu

Xây dựng kế hoạch kiểm thử Memory Sanitizer nhằm đánh giá khả năng phát hiện thay đổi bất thường của trang đã giải phóng, kiểm tra False Positive và phát hiện dấu hiệu Memory Leak.

Các bài kiểm thử dự kiến thực hiện trong QEMU, ưu tiên cấu hình một CPU ở giai đoạn đầu.

## 2. Danh sách Test Case

| Test ID | Mục tiêu | Thao tác kiểm thử dự kiến | Kết quả mong đợi |
|---|---|---|---|
| SAN-T01 | Kiểm tra cấp phát bình thường | `kalloc()` → sử dụng hợp lệ → `kfree()` | Không cảnh báo |
| SAN-T02 | Phát hiện thay đổi poison | Giải phóng một trang, tạo thay đổi trong vùng poison bằng test kernel có kiểm soát, sau đó kích hoạt kiểm tra | Phát hiện byte bất thường |
| SAN-T03 | Kiểm tra False Positive | Giải phóng rồi cấp phát lại trang có metadata Free List hợp lệ | Không cảnh báo nhầm |
| SAN-T04 | Kiểm tra tái cấp phát | Lặp lại chu trình cấp phát/giải phóng hợp lệ | Không phát hiện bất thường |
| SAN-T05 | Kiểm tra số trang trống | Đếm số trang trước cấp phát, sau cấp phát và sau giải phóng | Số trang trở về mức ban đầu trong điều kiện test được kiểm soát |
| SAN-T06 | Kiểm tra dấu hiệu Memory Leak | Cấp phát trang trong bài test riêng nhưng cố ý chưa giải phóng trước khi đo kết quả | Phát hiện số trang trống giảm so với baseline |

## 3. Kiểm thử chức năng Poison/Check

### SAN-T01 – Hoạt động bình thường

**Mục đích:** Xác nhận sanitizer không phát hiện lỗi khi trang được sử dụng đúng cách.

**Các bước:**

1. Cấp phát một trang.
2. Sử dụng trang hợp lệ.
3. Giải phóng trang.
4. Cấp phát lại trang trong điều kiện kiểm soát.
5. Kiểm tra kết quả sanitizer.

**Expected Result:** Không có cảnh báo bất thường.

### SAN-T02 – Phát hiện thay đổi sau giải phóng

**Mục đích:** Kiểm chứng khả năng phát hiện một trường hợp thay đổi nội dung trang đã giải phóng.

**Các bước:**

1. Cấp phát và giải phóng một trang trong kernel test riêng.
2. Tạo thay đổi có kiểm soát tại một byte thuộc vùng poison.
3. Kích hoạt quá trình kiểm tra trước khi trang được cấp phát lại.
4. Ghi nhận cảnh báo.

**Expected Result:** Sanitizer trả về vị trí byte thay đổi và ghi nhận lỗi `FREED_PAGE_CORRUPTION`.

Bài test chỉ chạy trên phiên bản xv6 thử nghiệm trong QEMU.

## 4. Kiểm thử False Positive

### SAN-T03 – Metadata Free List

**Mục đích:** Xác nhận việc lưu con trỏ `next` của Free List không gây cảnh báo nhầm.

**Các bước:**

1. Giải phóng một trang hợp lệ.
2. Cho phép xv6 cập nhật metadata Free List.
3. Cấp phát lại trang.
4. Kiểm tra kết quả sanitizer.

**Expected Result:** Không có cảnh báo.

## 5. Kiểm thử Memory Leak

### SAN-T05 – Giải phóng đầy đủ

Giả sử ban đầu có N trang trống:

- Trước cấp phát: N.
- Sau cấp phát một trang: N − 1.
- Sau giải phóng: N.

**Expected Result:** Số trang trống quay về N trong môi trường kiểm thử được kiểm soát.

### SAN-T06 – Chưa giải phóng trang

- Trước cấp phát: N.
- Sau cấp phát: N − 1.
- Kết thúc bước đo mà chưa giải phóng: N − 1.

**Expected Result:** Phát hiện số trang chưa được trả lại.

Đây là dấu hiệu nghi ngờ Memory Leak trong bài test kiểm soát, không phải bằng chứng rằng mọi trường hợp giảm số trang đều là lỗi.

## 6. Điều kiện kiểm thử

- xv6-RISC-V khởi động được trên QEMU.
- Ưu tiên cấu hình `CPUS=1`.
- Test lỗi chạy trên branch thử nghiệm riêng.
- Có log kết quả và mã nguồn test.
- Không đưa lỗi cố ý vào phiên bản kernel ổn định.
- Có thể khởi động lại QEMU để khôi phục môi trường sau mỗi test.

## 7. Tiêu chí nghiệm thu

- Test hợp lệ không tạo cảnh báo sai.
- Test thay đổi poison thuộc phạm vi hỗ trợ được phát hiện.
- Thông tin lỗi có thể đọc và phân tích.
- Phép đếm trang hoạt động trong bài test kiểm soát.
- Kết quả kiểm thử có thể lặp lại.

## 8. Kết luận

Test Plan V0.1 xác định các trường hợp kiểm thử cần thực hiện để đánh giá Memory Sanitizer.

Các test sẽ được triển khai và chạy trong các tuần phát triển tiếp theo. Kết quả PASS/FAIL thực tế chỉ được cập nhật sau khi chạy kiểm thử.