# WEEK 2 – THIẾT KẾ MEMORY SANITIZER TRÊN XV6-RISC-V

**Đề tài:** 26134 – System Call Fuzzer and Kernel Sanitizers  
**Thành viên:** Trần Tiến Đạt  
**Module:** Memory Sanitizer & Reliability  

## 1. Mục tiêu thiết kế

Thiết kế cơ chế Memory Sanitizer cho xv6-RISC-V nhằm hỗ trợ phát hiện các thay đổi bất thường đối với trang bộ nhớ vật lý sau khi được giải phóng.

Các chức năng dự kiến:

- Page Poisoning.
- Kiểm tra tính toàn vẹn của poison.
- Phát hiện và ghi nhận dấu hiệu lỗi bộ nhớ.
- Hạn chế False Positive.
- Kiểm tra dấu hiệu Memory Leak.

## 2. Cơ sở thiết kế

Trong Week 1, đã nghiên cứu cơ chế quản lý trang bộ nhớ vật lý bằng `kalloc()`, `kfree()` và Free List.

Trong xv6-RISC-V chuẩn, mỗi trang có kích thước 4096 byte.

Hàm `kfree()` ghi mẫu byte `0x01` khi giải phóng trang. Phần đầu trang được sử dụng để lưu con trỏ `next` của Free List.

Hàm `kalloc()` lấy trang ra khỏi Free List và ghi mẫu byte `0x05` trước khi trả trang cho kernel.

Phương án V0.1 tận dụng cơ chế này, bổ sung bước kiểm tra poison trước khi trang được cấp phát lại.

## 3. Thiết kế Poison/Check/Unpoison

### 3.1. Poison

**Vị trí:** Hàm `kfree()` trong `kernel/kalloc.c`.

**Nguyên lý:**

1. Kiểm tra địa chỉ trang hợp lệ.
2. Ghi mẫu byte `0x01` vào toàn bộ trang.
3. Đưa trang vào Free List.
4. Sử dụng vùng đầu trang để lưu metadata của Free List.

**Mục đích:** Tạo một mẫu dữ liệu có thể kiểm tra sau khi trang được giải phóng.

Trong V0.1, có thể tận dụng thao tác poison đã có của xv6.

### 3.2. Check

**Vị trí:** Hàm `kalloc()` trong `kernel/kalloc.c`.

**Nguyên lý:**

1. Lấy một trang ra khỏi Free List.
2. Kiểm tra mẫu poison trước khi ghi đè nội dung trang.
3. Bỏ qua vùng metadata hợp lệ của `struct run`.
4. Nếu phát hiện byte khác mẫu poison, ghi nhận địa chỉ trang và vị trí bất thường.
5. Nếu không phát hiện bất thường, tiếp tục quá trình cấp phát.

**Vùng kiểm tra trong xv6-RISC-V chuẩn:**

- Metadata: byte 0–7, không so sánh với poison.
- Vùng kiểm tra: byte 8–4095.
- Giá trị poison mong đợi: `0x01`.

**Giá trị trả về của hàm kiểm tra dự kiến:**

- `-1`: Không phát hiện bất thường.
- Giá trị từ `8` đến `4095`: Vị trí byte đầu tiên không khớp poison.

### 3.3. Unpoison

**Vị trí:** Sau khi kiểm tra thành công trong `kalloc()`.

**Nguyên lý:**

Trang đã được lấy khỏi Free List được chuyển sang trạng thái cấp phát hợp lệ.

Trong V0.1, tiếp tục sử dụng thao tác ghi mẫu `0x05` của xv6 để khởi tạo nội dung trang.

Đây là bước Unpoison theo nghĩa logic, chưa sử dụng Shadow Memory hoặc cơ chế giám sát tất cả thao tác truy cập.

## 4. Thiết kế API và vị trí tích hợp

Các file dự kiến:

| File | Chức năng |
|---|---|
| `kernel/kalloc.c` | Gọi hàm kiểm tra khi cấp phát trang |
| `kernel/sanitizer.c` | Xử lý kiểm tra poison và ghi nhận lỗi |
| `kernel/sanitizer.h` | Khai báo API và hằng số |
| `kernel/sanitizer_test.c` | Các bài kiểm thử nội bộ kernel |

API dự kiến:

- `san_check_poison(page)`: Kiểm tra mẫu poison, trả về vị trí bất thường hoặc `-1`.
- `san_report_corruption(page, offset, expected, actual)`: Ghi nhận thông tin lỗi.
- `freepage_count()`: Đếm số trang trống để phục vụ leak checking.

Các API mới chỉ được đề xuất ở mức thiết kế, chưa được triển khai.

## 5. Thiết kế cơ chế cảnh báo

Khi phát hiện dấu hiệu trang đã giải phóng bị thay đổi, hệ thống dự kiến ghi:

- Loại cảnh báo: `FREED_PAGE_CORRUPTION`.
- Địa chỉ trang bộ nhớ.
- Vị trí byte bất thường.
- Giá trị mong đợi.
- Giá trị thực tế.
- Tên hoặc mã bài kiểm thử nếu có.

Ví dụ định dạng log dự kiến:

`[SANITIZER] FREED_PAGE_CORRUPTION`

`page=0x... offset=100 expected=0x01 actual=0x63`

Ở phiên bản debug ban đầu, có thể dừng bài kiểm thử sau khi ghi nhận lỗi để tránh tiếp tục sử dụng trang có trạng thái không đáng tin cậy.

Cần thực hiện thao tác báo lỗi ở vị trí phù hợp, không giữ spinlock khi thực hiện xử lý có nguy cơ gây lỗi đồng bộ.

Việc xuất thông tin qua console chưa đồng nghĩa với việc Fuzzer ở User Space tự động đọc được báo cáo. Giao diện truyền lỗi giữa kernel và Fuzzer sẽ cần được cả nhóm thống nhất.

## 6. Thiết kế hạn chế False Positive

False Positive là tình huống sanitizer báo lỗi mặc dù không có hành vi sai trong phạm vi kiểm tra.

Trong xv6, phần đầu trang trống được sử dụng để lưu con trỏ `next` của Free List. Vì vậy, các byte metadata này có thể khác giá trị poison một cách hợp lệ.

Phương án:

- Bỏ qua vùng metadata khi kiểm tra.
- Kiểm tra trước khi `kalloc()` ghi mẫu `0x05`.
- Chỉ kiểm tra trang vừa được lấy ra khỏi Free List.
- Xác định rõ trạng thái và giới hạn của từng bài kiểm thử.
- Thử nghiệm cả trường hợp cấp phát/giải phóng hợp lệ.

## 7. Thiết kế Memory Leak Checking

Đề xuất xây dựng hàm `freepage_count()` để đếm số trang hiện có trong Free List.

Quy trình kiểm tra:

1. Đếm số trang trống trước bài test.
2. Thực hiện một thao tác cấp phát có kiểm soát.
3. Kết thúc bài test và thực hiện giải phóng tài nguyên cần thiết.
4. Đếm lại số trang trống.
5. So sánh hai kết quả.

Nếu số trang trống giảm bất thường, có thể tồn tại dấu hiệu rò rỉ bộ nhớ.

Phép kiểm tra cần thực hiện trong điều kiện kiểm soát các hoạt động cấp phát khác. Chênh lệch số trang không tự động chứng minh mọi trường hợp Memory Leak.

Hàm đếm phải bảo vệ quá trình duyệt Free List bằng cơ chế đồng bộ phù hợp.

## 8. Đồng bộ và an toàn khi tích hợp

Các thao tác đọc hoặc cập nhật Free List cần tuân thủ cơ chế bảo vệ bằng `kmem.lock`.

Trong thiết kế V0.1:

- Thao tác lấy trang phải bảo đảm đúng đồng bộ.
- Kiểm tra poison phải thực hiện trước khi ghi mẫu cấp phát mới.
- Không báo lỗi bằng cách tùy tiện thay đổi Free List.
- Ưu tiên kiểm thử trên QEMU với `CPUS=1` trước khi kiểm tra nhiều CPU.

## 9. Giới hạn của thiết kế

Phương án kiểm tra poison tại thời điểm tái cấp phát chỉ phát hiện một số trường hợp thay đổi dữ liệu của trang đã giải phóng.

Các giới hạn:

- Không phát hiện được mọi thao tác đọc Use-After-Free.
- Không phát hiện được thay đổi nếu dữ liệu cuối cùng vẫn trùng mẫu poison.
- Không bảo đảm phát hiện lỗi nếu trang bị sử dụng sai sau khi đã được cấp phát lại.
- Chưa tự động phát hiện Double Free.
- Chưa sử dụng Shadow Memory để kiểm tra từng thao tác truy cập bộ nhớ.

## 10. Kết luận

Đã đề xuất kiến trúc Memory Sanitizer V0.1 dựa trên cơ chế Free List và Memory Poisoning của xv6-RISC-V.

Phương án tập trung vào kiểm tra tính toàn vẹn của poison trước khi tái cấp phát, ghi nhận bất thường và hạn chế cảnh báo sai do metadata của Free List.

Trong Week 3, nhóm dự kiến triển khai các hàm kiểm tra, tích hợp vào `kalloc()` và chạy các bài kiểm thử để xác minh tính khả thi của thiết kế.