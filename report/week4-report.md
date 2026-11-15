# BÁO CÁO TIẾN ĐỘ TUẦN 4
## MEMORY SANITIZER & RELIABILITY – XV6-RISC-V

**Học phần:** Hệ điều hành – Học kỳ 20261  
**Mã đề tài:** 26134  
**Tên đề tài:** System Call Fuzzer and Kernel Sanitizers  
**Sinh viên:** Trần Tiến Đạt – MSSV 20233313  
**Phần việc:** Memory Sanitizer & Reliability  
**Môi trường:** xv6-RISC-V, QEMU, Ubuntu; CPUS=1 và CPUS=4  
**Repository nhóm:** https://github.com/DoanCuong2401/HeDieuHanh  
**Phạm vi báo cáo:** Kết quả công việc cá nhân tuần 4

## 1. TÓM TẮT TIẾN ĐỘ

Tuần 4 tiếp nối prototype Memory Sanitizer đã được xây dựng ở tuần 3. Khi đó, nhóm đã có cơ chế kiểm tra mẫu poison của trang vật lý trong `kalloc()` và bài kiểm thử chứng minh việc thay đổi mẫu sau `kfree()` có thể được phát hiện khi trang được tái cấp phát. Trọng tâm của tuần 4 là bổ sung phép đếm trang trống, đánh giá việc tránh cảnh báo nhầm (False Positive), mô phỏng một trang chưa được giải phóng tại thời điểm đo và kiểm tra hồi quy sau khi tích hợp thay đổi.

Kết quả quan sát được: ba bài kiểm thử mới **SAN-T04, SAN-T05 và SAN-T06 đều PASS**. Ở SAN-T05, số trang trống chuyển từ **32735 → 32734 → 32735**. Bộ kiểm thử có sẵn `usertests` kết thúc bằng **ALL TESTS PASSED** trong cả hai phiên chạy với `CPUS=1` và `CPUS=4`. Các kết quả này xác nhận hành vi trong phạm vi đã thử, không có nghĩa toàn bộ lỗi rò rỉ bộ nhớ hoặc lỗi tranh chấp đồng thời đã được loại trừ.

## 2. MỤC TIÊU VÀ PHẠM VI CÔNG VIỆC

- Triển khai hàm `san_freepage_count()` để đo số trang vật lý đang nằm trong Free List của bộ cấp phát xv6.
- Bảo vệ quá trình duyệt Free List bằng `kmem.lock` nhằm tránh đọc danh sách khi nó bị thay đổi đồng thời.
- Xây dựng SAN-T04 để kiểm tra một đường cấp phát/giải phóng hợp lệ không làm sanitizer báo lỗi sai.
- Xây dựng SAN-T05 và SAN-T06 để xác nhận quy tắc đếm trang và nhận biết dấu hiệu outstanding allocation trong một phép đo được kiểm soát.
- Chạy lại `usertests` với một CPU và bốn CPU để phát hiện lỗi hồi quy của các chức năng xv6.

Phạm vi Week 4 chưa bao gồm bảng metadata theo dõi mỗi lần cấp phát, công cụ tự động phân loại Memory Leak hoặc phép đo chi phí hiệu năng định lượng. Những nội dung này là hướng phát triển tiếp theo chứ không phải kết quả đã hoàn thành.

## 3. HOẠT ĐỘNG KỸ THUẬT

### 3.1. Cơ chế Free List và ý tưởng đếm trang

Trong xv6, `kalloc()` lấy một trang vật lý khỏi danh sách các trang trống (`kmem.freelist`), trong khi `kfree()` trả trang về danh sách này. Mỗi nút Free List có cấu trúc `struct run` với trường `next` trỏ đến nút tiếp theo. Do đó, có thể đếm số trang trống bằng cách duyệt tuần tự danh sách, tăng bộ đếm tại mỗi nút và dừng khi gặp con trỏ `0`.

Bài đo sử dụng ba thời điểm: **N0** trước cấp phát, **N1** sau cấp phát một trang và **N2** sau giải phóng chính trang đó. Nếu không có thao tác cấp phát/giải phóng khác xen vào, kỳ vọng **N1 = N0 − 1** và **N2 = N0**. Mối quan hệ này kiểm tra tính nhất quán của bộ đếm trong tình huống đơn giản.

### 3.2. Thiết kế hàm `san_freepage_count()`

Hàm được bổ sung tại `kernel/kalloc.c` và duyệt Free List khi đang giữ khóa `kmem.lock`. Ý tưởng chính của mã nguồn như sau:

```c
int san_freepage_count(void)
{
    int count = 0;
    struct run *r;
    acquire(&kmem.lock);
    for (r = kmem.freelist; r != 0; r = r->next)
        count++;
    release(&kmem.lock);
    return count;
}
```

`acquire(&kmem.lock)` ngăn các thao tác dùng cùng khóa thay đổi Free List trong lúc đếm; `release(&kmem.lock)` giải phóng khóa sau khi hoàn tất. Giá trị trả về là số nút hiện đang có trong Free List, không phải dung lượng RAM còn trống của toàn hệ thống theo nghĩa tổng quát.

Vì phép đếm có độ phức tạp O(N) theo số trang trống và giữ spinlock trong suốt vòng lặp, hiện tại hàm được sử dụng chủ yếu phục vụ kiểm thử. Không nên gọi liên tục trong luồng cấp phát hiệu năng cao nếu chưa đánh giá overhead.

### 3.3. Tổ chức mã nguồn và kiểm thử

Các thay đổi của Week 4 được tổ chức quanh các file sau:

| File | Vai trò trong Week 4 |
|---|---|
| `kernel/kalloc.c` | Bổ sung hàm đếm trang trống, giữ nguyên cơ chế kiểm tra poison từ tuần 3 |
| `kernel/sanitizer.h` | Khai báo hàm đếm và entry point của self-test Week 4 |
| `kernel/sanitizer_test.c` | Chứa SAN-T04, SAN-T05 và SAN-T06 |
| `kernel/main.c` | Bật lời gọi self-test tạm thời sau `kinit()`, rồi tắt khi trở về chế độ chạy bình thường |
| `report/week4-report.md` | Lưu báo cáo cá nhân của tuần 4 |

Việc gọi các bài test sau `kinit()` là cần thiết vì tại đó Free List và khóa `kmem.lock` đã được khởi tạo. Sau khi thu bằng chứng kiểm thử, lời gọi self-test được comment để tránh chạy test tự động mỗi lần boot bản kernel sử dụng bình thường.

## 4. THIẾT KẾ VÀ KẾT QUẢ TỪNG BÀI KIỂM THỬ

### 4.1. SAN-T04 – Kiểm tra False Positive

**Mục tiêu:** Xác nhận một chu trình cấp phát – giải phóng – tái cấp phát hợp lệ không kích hoạt cảnh báo `FREED_PAGE_CORRUPTION` ngoài ý muốn.

**Cách thực hiện:** Dùng `kalloc()` lấy một trang, trả trang bằng `kfree()`, sau đó gọi `kalloc()` lần nữa. Trong kịch bản không có thao tác xen vào, Free List hoạt động theo dạng LIFO nên trang vừa trả lại dự kiến được lấy ra tiếp theo. Mẫu poison phải còn hợp lệ trong vùng dữ liệu được kiểm tra; vùng metadata đầu trang được bỏ qua theo thiết kế từ Week 3.

**Kết quả ghi nhận:** `[SAN-T04] PASS: No false positive`. Đây là bằng chứng không có cảnh báo nhầm trong một chu trình hợp lệ đã thử, chưa phải đánh giá toàn diện tỷ lệ False Positive.

### 4.2. SAN-T05 – Kiểm tra tính đúng của Free Page Counter

**Mục tiêu:** Kiểm tra số trang trống biến động hợp lý khi cấp phát và giải phóng một trang.

**Các bước:** (1) Đo số trang ban đầu N0; (2) thực hiện `kalloc()` và đo N1; (3) thực hiện `kfree()` và đo N2; (4) so sánh N1 = N0 − 1 và N2 = N0.

**Kết quả thực tế:** `[SAN-T05] PASS: 32735 -> 32734 -> 32735`. Số trang giảm đúng một trang sau cấp phát và trở lại ban đầu sau giải phóng. Điều này xác nhận phép đếm phù hợp với hành vi thử nghiệm trên snapshot bộ nhớ tại thời điểm chạy.

### 4.3. SAN-T06 – Phát hiện dấu hiệu outstanding allocation

**Mục tiêu:** Xác nhận phép đếm có thể nhận biết một trang vẫn đang được cấp phát tại thời điểm đo và số lượng trở lại bình thường sau khi trả trang.

**Các bước:** (1) Đo số trang ban đầu; (2) cấp phát hai trang; (3) chỉ trả lại một trang rồi đếm số trang còn trống; (4) trả lại trang thứ hai và đo lần cuối. Đo giữa chừng sẽ thiếu một trang so với baseline; đo cuối phải trở về baseline.

**Kết quả ghi nhận:** `[SAN-T06] PASS: Outstanding page detected`.

**Diễn giải đúng:** Kết quả chứng minh bộ đếm phân biệt được tình huống còn một trang chưa được trả lại trong bài test. Trang đó được giải phóng ở cuối bài, nên không tạo rò rỉ cố ý kéo dài. Chức năng hiện chưa đủ dữ liệu để tự khẳng định một trang bị “leak” trong hệ thống thực tế.

### 4.4. Bảng tổng hợp kiểm thử chức năng

| Test ID | Tiêu chí đánh giá | Kết quả thực tế | Nhận xét |
|---|---|---|---|
| SAN-T04 | Không cảnh báo sai trong chu trình hợp lệ | PASS | Một tình huống False Positive được kiểm tra |
| SAN-T05 | Số trang diễn biến N → N−1 → N | PASS: 32735 → 32734 → 32735 | Phép đếm phù hợp kịch bản thử |
| SAN-T06 | Nhận biết trang còn được giữ tại thời điểm đo | PASS | Chỉ là outstanding allocation kiểm soát |

## 5. KIỂM THỬ HỒI QUY VÀ ĐỘ ỔN ĐỊNH

Sau phần thử nghiệm riêng của sanitizer, xv6 được chạy với `make CPUS=1 qemu` và `make CPUS=4 qemu`. Bộ `usertests` được thực thi tại shell của xv6; cả hai kết quả do sinh viên ghi nhận đều kết thúc với `ALL TESTS PASSED`.

| Cấu hình | Lệnh dùng để khởi động | Kết quả `usertests` |
|---|---|---|
| 1 CPU | `make CPUS=1 qemu` | ALL TESTS PASSED |
| 4 CPU | `make CPUS=4 qemu` | ALL TESTS PASSED |

**Nhận xét quan trọng:** Trong ảnh kiểm thử SAN-T04–SAN-T06 với `CPUS=4`, ba test này chạy sau `kinit()` nhưng **trước khi các hart 1, 2, 3 được khởi động**. Vì vậy, bản thân SAN-T04–SAN-T06 là bài test tuần tự. `usertests` PASS ở cấu hình 4 CPU giúp bổ sung bằng chứng hồi quy nhưng chưa chứng minh sanitizer đã được stress test đa luồng hoặc không có race condition.

## 6. KHÓ KHĂN, GIỚI HẠN VÀ ĐÁNH GIÁ

**6.1. Giới hạn của Leak Checking:** Số trang trống giảm có thể do chương trình đang giữ trang hợp lệ. Nếu không có metadata về người sở hữu, thời điểm cấp phát và thời điểm giải phóng, phép đếm không đủ cơ sở để kết luận Memory Leak. Phương án hiện tại phù hợp làm công cụ kiểm thử hoặc giám sát sơ bộ.

**6.2. Giới hạn của cơ chế khóa:** Duyệt Free List khi giữ `kmem.lock` bảo vệ tính nhất quán của danh sách nhưng cũng khiến các thao tác cấp phát/giải phóng khác phải chờ. Chưa có phép đo thời gian hoặc overhead ở Week 4, nên không kết luận về tác động hiệu năng.

**6.3. Giới hạn kiểm thử đa CPU:** Một lần `usertests` PASS trên 4 CPU không chứng minh không có race condition. Cần thử nhiều lượt, có workload cấp phát/giải phóng đồng thời và đối chiếu log để kiểm tra kỹ hơn.

**6.4. Giới hạn tích hợp:** Báo cáo lỗi hiện thiên về log kernel và panic trong chế độ debug. Giao diện kết nối các sự kiện sanitizer với Fuzzer, Coverage, testcase và crash triage cần được thống nhất với nhóm ở các tuần sau.

## 7. KẾ HOẠCH TUẦN 5

1. Thống nhất cấu trúc báo cáo lỗi giữa Memory Sanitizer, System Call Fuzzer và Kernel Coverage: loại lỗi, thời điểm, testcase ID, thông tin trang và log tương ứng.
2. Xây dựng một kịch bản tích hợp có thể tái hiện và xác định rõ cách phía Executor thu nhận kết quả sanitizer.
3. Đề xuất kiểm thử đồng thời hoặc chạy lặp trên nhiều CPU để mở rộng đánh giá Reliability.
4. Khảo sát phương án đo overhead của kiểm tra poison và Free Page Counting trên cùng điều kiện thử nghiệm.
5. Tổng hợp kết quả, chuẩn bị nội dung báo cáo tiến độ nhóm và cập nhật tài liệu thiết kế khi có thay đổi.

## 8. KẾT LUẬN

Trong Week 4, đã bổ sung cơ chế đếm trang trống vào bộ cấp phát xv6 và xây dựng ba bài kiểm thử mới. Kết quả SAN-T04, SAN-T05, SAN-T06 đều PASS, trong đó SAN-T05 ghi nhận `32735 → 32734 → 32735`. Bộ `usertests` trên cấu hình CPUS=1 và CPUS=4 đều kết thúc bằng `ALL TESTS PASSED`. Những kết quả này xác nhận bước tiến về kiểm tra tính nhất quán của Free List và khả năng nhận biết trang chưa được trả lại trong môi trường được kiểm soát. Việc xác định Memory Leak tự động, đánh giá lỗi đa CPU và tích hợp module vẫn là công việc tiếp theo.

## PHỤ LỤC – MINH CHỨNG KIỂM THỬ

**Hình 1.** Log SAN-T04, SAN-T05, SAN-T06 PASS và các hart phụ khởi động trên cấu hình QEMU CPUS=4.

**Hình 2.** `usertests` trên CPUS=1 kết thúc với `ALL TESTS PASSED`.

**Hình 3.** `usertests` trên CPUS=4 kết thúc với `ALL TESTS PASSED`.

*Ghi chú: Cấu hình CPUS=1/CPUS=4 của Hình 2 và Hình 3 theo thông tin sinh viên cung cấp trong quá trình kiểm thử.*
