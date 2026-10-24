# BÁO CÁO TIẾN ĐỘ TUẦN 1 – MEMORY SANITIZER & RELIABILITY
**Đề tài:** 26134 – System Call Fuzzer and Kernel Sanitizers

**Thành viên:** Trần Tiến Đạt

**Module phụ trách:** Memory Sanitizer & Reliability

**Nền tảng:** xv6-RISC-V, QEMU

## 1. Mục tiêu nghiên cứu

Trong tuần đầu tiên, mục tiêu là tìm hiểu cơ chế quản lý bộ nhớ vật lý của hệ điều hành xv6-RISC-V, từ đó xây dựng nền tảng kiến thức để phát triển Memory Sanitizer trong các tuần tiếp theo.

Các nội dung cần nghiên cứu:

- Tìm hiểu cấu trúc quản lý bộ nhớ của xv6.
- Hiểu cơ chế cấp phát và giải phóng trang bộ nhớ.
- Phân tích các hàm `kinit()`, `kalloc()` và `kfree()`.
- Tìm hiểu cấu trúc danh sách trang trống (Free List).
- Xác định vị trí có thể bổ sung cơ chế Memory Sanitizer.

## 2. Tổng quan về quản lý bộ nhớ trong xv6

xv6-RISC-V quản lý bộ nhớ vật lý theo từng trang (Page), mỗi trang có kích thước 4096 byte, tương đương 4 KiB.

Khi kernel cần sử dụng bộ nhớ, hệ điều hành sẽ cấp phát một trang còn trống. Sau khi sử dụng xong, trang được giải phóng để có thể cấp phát lại cho các hoạt động khác.

Các trang bộ nhớ còn trống được quản lý thông qua một danh sách liên kết gọi là Free List.

Hai hàm quan trọng nhất:

- `kalloc()`: Cấp phát một trang bộ nhớ vật lý.
- `kfree()`: Giải phóng một trang và đưa trang đó trở lại Free List.

Mã nguồn quản lý bộ nhớ vật lý được triển khai chủ yếu trong file `kernel/kalloc.c`.

## 3. Phân tích file kernel/kalloc.c

### 3.1. Cấu trúc struct run

```c
struct run {
    struct run *next;
};
```

Đây là cấu trúc dùng để liên kết các trang bộ nhớ còn trống.

Trong đó:

- `run`: Đại diện cho một phần tử trong danh sách trang trống.
- `next`: Con trỏ tới trang trống tiếp theo.

Ví dụ:

```text
Page A -> Page B -> Page C -> NULL
```

Các trang A, B, C đang còn trống và được liên kết với nhau.

### 3.2. Biến kmem

```c
struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;
```

Biến `kmem` chứa thông tin quản lý các trang bộ nhớ vật lý còn trống.

- `lock`: Khóa dùng để bảo vệ danh sách khi có nhiều CPU cùng truy cập.
- `freelist`: Con trỏ tới trang đầu tiên trong danh sách trang trống.

### 3.3. Hàm kinit()

Hàm `kinit()` được gọi trong quá trình khởi động kernel.

Nhiệm vụ:

- Khởi tạo khóa quản lý bộ nhớ.
- Gọi `freerange()` để đưa các trang bộ nhớ vật lý có thể sử dụng vào Free List.

Nhờ vậy, các trang trống sẵn sàng được cấp phát cho kernel.

### 3.4. Hàm kalloc()

Hàm `kalloc()` dùng để cấp phát một trang bộ nhớ vật lý.

Quy trình hoạt động:

1. Khóa danh sách trang trống bằng `acquire()`.
2. Lấy trang đầu tiên từ Free List.
3. Cập nhật con trỏ Free List sang trang tiếp theo.
4. Mở khóa bằng `release()`.
5. Nếu lấy được trang, ghi giá trị byte `5` vào trang để hỗ trợ phát hiện một số lỗi sử dụng bộ nhớ.
6. Trả về địa chỉ trang được cấp phát.

Ví dụ:

Trước khi cấp phát:

```text
Free List: A -> B -> C
```

Sau khi gọi `kalloc()`:

```text
Trang được cấp phát: A
Free List: B -> C
```

Trang A được lấy khỏi danh sách trống và có thể sử dụng.

Nếu không còn trang trống, hàm trả về `0`.

### 3.5. Hàm kfree()

Hàm `kfree()` dùng để giải phóng một trang bộ nhớ vật lý.

Quy trình hoạt động:

1. Kiểm tra tính hợp lệ của địa chỉ trang.
2. Ghi giá trị byte `1` vào toàn bộ trang.
3. Khóa danh sách trang trống.
4. Đưa trang vừa giải phóng vào đầu Free List.
5. Mở khóa.

Ví dụ:

Trước khi giải phóng:

```text
Trang A đang được sử dụng
Free List: B -> C
```

Sau khi gọi `kfree(A)`:

```text
Free List: A -> B -> C
```

Trang A đã được trả lại và có thể được cấp phát cho hoạt động khác.

## 4. Cơ chế Memory Poisoning

Trong xv6, khi giải phóng trang, hàm `kfree()` sử dụng câu lệnh:

```c
memset(pa, 1, PGSIZE);
```

Câu lệnh này ghi giá trị byte `1` vào toàn bộ trang bộ nhớ.

Sau đó, phần đầu trang được sử dụng để lưu con trỏ `next` của Free List.

Việc ghi một giá trị đặc biệt vào bộ nhớ giúp hỗ trợ quá trình debug.

Tuy nhiên, xv6 gốc chưa có cơ chế tự động kiểm tra toàn bộ nội dung poison trước khi trang được tái cấp phát.

Đây là cơ sở để phát triển Memory Sanitizer.

**Ý tưởng triển khai:**

- Khi trang được giải phóng, ghi mẫu dữ liệu poison.
- Khi chuẩn bị cấp phát lại, kiểm tra vùng dữ liệu poison còn nguyên vẹn hay không.
- Nếu phát hiện dữ liệu bị thay đổi bất thường, ghi nhận dấu hiệu lỗi bộ nhớ.
- Bỏ qua vùng metadata hợp lệ của Free List khi kiểm tra.

Phương pháp này có thể phát hiện một số trường hợp ghi vào trang sau khi giải phóng, nhưng không đảm bảo phát hiện tất cả lỗi Use-After-Free.

## 5. Kết quả thực hành Week 1

Đã khởi động thành công xv6-RISC-V trên QEMU bằng lệnh:

```bash
make CPUS=1 qemu
```

Kernel khởi động và xuất hiện giao diện shell:

```text
xv6 kernel is booting
init: starting sh
$
```

Đã kiểm tra các lệnh cơ bản:

```text
$ ls
$ echo hello
hello
```

**Kết quả quan sát:** xv6 đã khởi động thành công, các lệnh `ls` và `echo` hoạt động bình thường.

Các bài kiểm thử nâng cao như `usertests` và `grind` chưa có kết quả xác nhận trong tài liệu này.

## 6. Kết luận Week 1

Qua quá trình nghiên cứu, đã xác định được các thành phần chính trong cơ chế cấp phát bộ nhớ vật lý của xv6, bao gồm Free List, `struct run`, `kinit()`, `kalloc()` và `kfree()`.

Việc nghiên cứu file `kernel/kalloc.c` giúp xác định vị trí dự kiến tích hợp Memory Sanitizer trong các tuần tiếp theo.

## 7. Kế hoạch Week 2

- Nghiên cứu kỹ cơ chế Memory Poisoning.
- Thiết kế thuật toán kiểm tra trang đã giải phóng.
- Xác định cách phát hiện và báo cáo lỗi bộ nhớ.
- Thiết kế các trường hợp kiểm thử.
- Chuẩn bị tài liệu `docs/sanitizer-design.md`.