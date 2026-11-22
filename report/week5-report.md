# BÁO CÁO TIẾN ĐỘ TUẦN 5
## MEMORY SANITIZER – FUZZER INTEGRATION

**Học phần:** Hệ điều hành  
**Đề tài:** System Call Fuzzer and Kernel Sanitizers  
**Sinh viên:** Trần Tiến Đạt – MSSV 20233313  
**Module phụ trách:** Memory Sanitizer & Reliability  
**Nền tảng thực hiện:** xv6-RISC-V, Ubuntu, QEMU  
**Nhánh phát triển:** `dat/week5-work`

---

## 1. Tóm tắt công việc

Trong tuần 5, em tiếp tục phát triển module Memory Sanitizer đã được xây dựng ở tuần 3 và tuần 4. Nội dung trọng tâm là chuẩn hóa định dạng thông báo lỗi, xây dựng công cụ phân tích log và thiết lập cơ chế liên kết sự kiện bộ nhớ với các testcase do System Call Fuzzer thực thi.

Ở các tuần trước, Memory Sanitizer đã có khả năng phát hiện việc thay đổi dữ liệu poison của một trang vật lý sau khi trang đó được giải phóng. Tuy nhiên, kết quả phát hiện lỗi chủ yếu được hiển thị qua thông báo trên màn hình QEMU. Điều này gây khó khăn khi cần tổng hợp kết quả từ nhiều testcase hoặc xác định testcase liên quan đến một cảnh báo.

Để giải quyết vấn đề trên, trong tuần 5 em đã thực hiện các công việc chính:

1. Chuẩn hóa thông báo Memory Sanitizer thành định dạng `[SAN-EVENT]`.
2. Xây dựng chương trình `san_log_parser.py` để đọc log QEMU và chuyển thành JSON.
3. Bổ sung marker `BEGIN` và `END` vào chương trình System Call Fuzzer.
4. Xây dựng cơ chế liên kết cảnh báo Sanitizer với testcase đang thực thi.
5. Bổ sung chức năng phân loại trạng thái `NO_EVENT`, `SANITIZER_EVENT` và `INCOMPLETE`.
6. Xây dựng chương trình tổng hợp kết quả từ nhiều file JSON.
7. Kiểm thử các tình huống bình thường, phát hiện lỗi, gián đoạn và ranh giới testcase.
8. Chạy kiểm thử hồi quy xv6 trên cấu hình một CPU và bốn CPU.

**Kết quả:** Các chức năng phân tích log đã hoạt động đúng trên những tình huống kiểm thử được xây dựng. Chương trình đọc thành công cảnh báo thật từ QEMU và xử lý đúng các marker testcase của Fuzzer. Bộ kiểm thử `usertests` cũng được xác nhận PASS trên cả hai cấu hình CPU.

## 2. Mục tiêu và yêu cầu tích hợp

### 2.1. Mục tiêu

Mục tiêu chính của tuần 5 là xây dựng nền tảng tích hợp Memory Sanitizer với System Call Fuzzer để hỗ trợ phát hiện, phân loại và lưu trữ kết quả kiểm thử tự động.

Các mục tiêu cụ thể bao gồm:

- Chuẩn hóa thông tin lỗi do Memory Sanitizer phát hiện.
- Thu thập log kernel thông qua QEMU.
- Chuyển đổi thông tin log thành dữ liệu JSON có cấu trúc.
- Theo dõi testcase đang thực thi trong Fuzzer.
- Liên kết cảnh báo bộ nhớ với testcase khi có đủ thông tin.
- Phân biệt các lần chạy hoàn thành bình thường và bị gián đoạn.
- Giữ nguyên tính ổn định của các chức năng xv6 đã kiểm thử ở những tuần trước.

### 2.2. Yêu cầu chức năng

Hệ thống cần đáp ứng các yêu cầu sau:

**Thứ nhất,** Memory Sanitizer phải xuất thông tin lỗi theo định dạng thống nhất, cho phép các chương trình bên ngoài nhận diện và phân tích.

**Thứ hai,** Fuzzer phải có cơ chế đánh dấu thời điểm bắt đầu và kết thúc của từng testcase để hỗ trợ xác định testcase đang chạy khi xảy ra sự cố.

**Thứ ba,** chương trình phân tích log cần trích xuất các trường dữ liệu quan trọng và chuyển chúng thành định dạng JSON.

**Thứ tư,** hệ thống phải phân biệt được một lần chạy không có cảnh báo với một lần chạy bị gián đoạn nhưng chưa ghi nhận cảnh báo.

**Thứ năm,** các thay đổi của Week 5 không được làm ảnh hưởng đến cơ chế poison checking, bộ cấp phát `kalloc()`/`kfree()` và các chức năng cơ bản của hệ điều hành xv6.

### 2.3. Phạm vi triển khai

Việc tích hợp được phát triển trên prototype đã hoàn thành ở Week 3 và Week 4.

Trong tuần 5, em tập trung vào quá trình thu thập và phân tích thông tin lỗi, chưa thay đổi thuật toán kiểm tra poison hoặc cách quản lý Free List.

Phần đã kiểm chứng bao gồm việc phát hiện lỗi bộ nhớ thật trên QEMU, thu thập log, chạy Fuzzer thực tế và phân tích kết quả.

Việc liên kết một cảnh báo với mã testcase cụ thể đã được xác minh bằng log giả lập có cấu trúc. Chưa có testcase của Fuzzer thực tế trực tiếp gây ra lỗi bộ nhớ để kiểm chứng toàn bộ luồng tích hợp trong cùng một lần chạy.

## 3. Chuẩn hóa thông báo Memory Sanitizer

### 3.1. Cơ chế báo lỗi trước khi tích hợp

Từ Week 3, Memory Sanitizer đã được tích hợp với bộ cấp phát trang vật lý của xv6.

Khi một trang được trả về Free List bằng `kfree()`, vùng dữ liệu của trang được ghi giá trị poison `0x01`.

Trong lần cấp phát tiếp theo, hàm `san_check_poison()` kiểm tra vùng dữ liệu tương ứng. Nếu phát hiện byte không còn mang giá trị poison mong đợi, hàm trả về vị trí byte bị thay đổi.

Hàm `san_report_corruption()` tiếp nhận thông tin, in cảnh báo và gọi `panic()` để dừng kernel.

Ở phiên bản trước, thông báo lỗi được in trên nhiều dòng. Dù thuận tiện khi quan sát trực tiếp, cấu trúc này chưa phù hợp cho việc tự động phân tích log.

### 3.2. Thiết kế định dạng SAN-EVENT

Trong Week 5, em sửa hàm `san_report_corruption()` tại file `kernel/sanitizer.c` để chuẩn hóa thông báo theo định dạng:

`[SAN-EVENT] type=... page=... offset=... expected=... actual=...`

Trong đó:

| Trường | Ý nghĩa |
|---|---|
| `SAN-EVENT` | Đánh dấu một sự kiện do Memory Sanitizer phát hiện |
| `type` | Loại lỗi bộ nhớ |
| `page` | Địa chỉ trang bộ nhớ liên quan |
| `offset` | Vị trí byte không hợp lệ |
| `expected` | Giá trị poison mong đợi |
| `actual` | Giá trị thực tế được đọc |

Thay đổi này chỉ ảnh hưởng đến định dạng thông báo. Cơ chế kiểm tra poison và hành vi gọi `panic()` khi phát hiện lỗi vẫn được giữ nguyên.

### 3.3. Kiểm thử phát hiện lỗi thật

Em sử dụng lại bài kiểm thử SAN-T03 đã xây dựng từ Week 3.

Quy trình kiểm thử:

1. Gọi `kalloc()` để cấp phát một trang.
2. Gọi `kfree()` để giải phóng trang.
3. Cố tình ghi giá trị `0x63` vào byte thứ 100 của trang đã giải phóng.
4. Gọi `kalloc()` để kích hoạt cơ chế kiểm tra poison.

Kết quả thực tế thu được từ QEMU:

`[SAN-T03] Starting integration test`

`[DEBUG] page=0x0000000087fff000 byte100=0x63`

`[SAN-EVENT] type=FREED_PAGE_CORRUPTION page=0x0000000087fff000 offset=100 expected=0x1 actual=0x63`

`panic: memory sanitizer`

Kết quả xác nhận Memory Sanitizer phát hiện đúng loại lỗi, địa chỉ trang, vị trí byte bị thay đổi và giá trị dữ liệu thực tế.

Thông báo được xuất thành công theo định dạng mới, tạo điều kiện cho chương trình Python phân tích tự động.

## 4. Xây dựng chương trình phân tích log Python

### 4.1. Mục đích

Memory Sanitizer chạy bên trong kernel xv6, còn công cụ phân tích kết quả được triển khai trên Ubuntu.

Để kết nối hai môi trường, em xây dựng chương trình `tools/san_log_parser.py` bằng Python.

Quy trình xử lý được tổ chức như sau:

**xv6 Kernel → QEMU → File log → Python Parser → JSON**

Chương trình đọc các dòng log, tìm thông báo Sanitizer và chuyển thông tin thành cấu trúc dữ liệu có thể xử lý tự động.

### 4.2. Phân tích sự kiện SAN-EVENT

Chương trình đọc file log theo từng dòng.

Khi gặp một dòng bắt đầu bằng `[SAN-EVENT]`, chương trình tách các cặp thông tin có dạng `key=value`.

Ví dụ:

`offset=100`

được phân tách thành:

- Tên trường: `offset`
- Giá trị: `100`

Tương tự, các trường `type`, `page`, `expected` và `actual` cũng được trích xuất.

Mỗi sự kiện được lưu vào một dictionary Python, sau đó bổ sung vào danh sách `events`.

Danh sách này là cơ sở để tính toán số cảnh báo và xuất kết quả JSON.

### 4.3. Gắn mã testcase vào kết quả

Ban đầu, chương trình cho phép truyền mã testcase trực tiếp từ dòng lệnh.

Ví dụ:

`python3 tools/san_log_parser.py tools/sample-san.log TC001`

Kết quả JSON sẽ có trường:

`"testcase_id": "TC001"`

Chương trình cũng hỗ trợ giá trị mặc định `UNKNOWN` nếu người dùng không truyền mã testcase.

Sau đó, cơ chế được mở rộng để có thể nhận diện mã testcase tự động từ các marker do Fuzzer tạo ra.

### 4.4. Phân loại trạng thái thực thi

Để đánh giá đầy đủ hơn một lần chạy Fuzzer, chương trình theo dõi:

- Số testcase bắt đầu: `begin_count`
- Số testcase kết thúc: `end_count`
- Thông báo hoàn thành: `fuzzing_completed`
- Trạng thái hoàn thành: `run_completed`
- Số sự kiện Sanitizer: `event_count`

Từ các thông tin này, chương trình phân loại kết quả thành ba trạng thái:

| Trạng thái | Ý nghĩa |
|---|---|
| `SANITIZER_EVENT` | Có ít nhất một cảnh báo Sanitizer |
| `NO_EVENT` | Fuzzer hoàn thành và không ghi nhận cảnh báo |
| `INCOMPLETE` | Chưa có đủ bằng chứng Fuzzer hoàn thành, đồng thời không có cảnh báo Sanitizer |

Trường `run_completed` được lưu độc lập để phân biệt giữa việc phát hiện lỗi và việc hoàn thành toàn bộ lần chạy.

Điều này giúp tránh đánh giá nhầm một lần chạy bị gián đoạn thành một lần chạy thành công chỉ vì không tìm thấy cảnh báo bộ nhớ.

### 4.5. Xuất dữ liệu JSON

Chương trình xuất kết quả dưới định dạng JSON với các trường chính:

`testcase_id`, `begin_count`, `end_count`, `fuzzing_completed`, `run_completed`, `status`, `event_count`, `events`.

Định dạng JSON giúp kết quả dễ dàng được lưu trữ, kiểm tra cú pháp và sử dụng bởi các công cụ khác.

### 4.6. Chương trình tổng hợp kết quả

Ngoài bộ phân tích log, em xây dựng thêm `tools/san_summary.py`.

Chương trình đọc các file JSON trong một thư mục và tổng hợp:

- Tổng số testcase hoặc lần chạy được ghi nhận.
- Số kết quả có cảnh báo Sanitizer.
- Tổng số sự kiện Sanitizer.

Trong một thử nghiệm ban đầu với hai file JSON, chương trình cho kết quả:

`Total testcases: 2`

`Testcases with events: 1`

`Total sanitizer events: 1`

Hai file trong thử nghiệm gồm một kết quả lấy từ log QEMU thật và một kết quả tạo từ log mẫu. Vì vậy, đây là minh chứng cho chức năng tổng hợp, chưa phải thống kê một chiến dịch fuzzing thực tế.

## 5. Tích hợp với System Call Fuzzer

### 5.1. Đặc điểm Fuzzer hiện tại

Module System Call Fuzzer của nhóm nằm tại `user/fuzzer.c`.

Fuzzer sử dụng thuật toán sinh số giả ngẫu nhiên xorshift32 và hỗ trợ cấu hình seed để tái tạo thứ tự lựa chọn syscall.

Các syscall được phiên bản hiện tại sử dụng gồm:

- `getpid()`
- `uptime()`
- `pause()`

Fuzzer thực hiện lần lượt từng testcase và in kết quả ra console.

### 5.2. Hạn chế trước khi tích hợp

Ở phiên bản ban đầu, Fuzzer chỉ in thông tin testcase sau khi syscall hoàn thành.

Nếu kernel xảy ra lỗi và gọi `panic()` trong quá trình thực hiện syscall, dòng kết quả testcase có thể chưa được in.

Trong trường hợp này, chương trình phân tích log khó xác định testcase nào đang chạy tại thời điểm phát hiện lỗi.

### 5.3. Bổ sung marker BEGIN và END

Để giải quyết vấn đề trên, em bổ sung hai loại thông báo vào hàm `run_testcase()`:

`[FUZZ-CASE] id=1 status=BEGIN`

`[FUZZ-CASE] id=1 status=END`

Marker `BEGIN` được in trước khi syscall được thực thi.

Marker `END` được in sau khi syscall hoàn thành và chương trình đã in kết quả.

Cơ chế này cho phép nhận diện khoảng thời gian thực hiện mỗi testcase trong log.

### 5.4. Theo dõi testcase đang thực thi

Trong `san_log_parser.py`, em bổ sung biến `current_case` để theo dõi testcase.

Khi gặp marker `BEGIN`, chương trình lưu ID của testcase.

Khi gặp marker `END`, chương trình xóa thông tin testcase đang chạy.

Nếu một sự kiện `[SAN-EVENT]` xuất hiện trong khoảng thời gian đó, sự kiện được bổ sung trường `fuzzer_case_id`.

Ví dụ:

`[FUZZ-CASE] id=2 status=BEGIN`

`[SAN-EVENT] type=FREED_PAGE_CORRUPTION ...`

Kết quả JSON tương ứng có:

`"fuzzer_case_id": "2"`

Cơ chế này phù hợp với Fuzzer thực thi testcase tuần tự. Khi mở rộng sang thực thi đồng thời, cần bổ sung thông tin nhận diện tiến trình hoặc luồng để tránh gắn nhầm sự kiện.

### 5.5. Kiểm thử Fuzzer trên xv6

Em chạy Fuzzer thực tế bằng lệnh:

`fuzzer 1 5`

Trong đó seed bằng 1 và số testcase bằng 5.

Các syscall được thực thi lần lượt là:

| Testcase | Syscall | Kết quả thực tế |
|---|---|---|
| TC1 | `getpid()` | PID = 4 |
| TC2 | `uptime()` | 62 ticks |
| TC3 | `getpid()` | PID = 4 |
| TC4 | `pause(2)` | Trả về 0 |
| TC5 | `uptime()` | 64 ticks |

Sau khi bổ sung marker, log ghi nhận:

- 5 dòng `status=BEGIN`.
- 5 dòng `status=END`.
- Thông báo `=== FUZZING COMPLETED ===`.
- Không xuất hiện cảnh báo `[SAN-EVENT]`.

Điều này chứng minh Fuzzer chạy hoàn chỉnh các testcase đã cấu hình và cơ chế đánh dấu testcase hoạt động đúng trong lần kiểm thử thực tế.

## 6. Kiểm thử thực nghiệm và đánh giá kết quả

### 6.1. Kiểm thử chương trình phân tích log

Em xây dựng nhiều tình huống kiểm thử để xác minh độ chính xác của chương trình Python.

| Mã kiểm thử | Nội dung | Kết quả |
|---|---|---|
| W5-T01 | Phân tích một sự kiện Sanitizer | PASS – 1 sự kiện |
| W5-T02 | Log không có cảnh báo | PASS – 0 sự kiện |
| W5-T03 | Log giả lập có hai cảnh báo | PASS – 2 sự kiện |
| W5-T04 | Không cung cấp testcase ID | PASS – `UNKNOWN` |
| W5-T05 | Gắn lỗi với testcase đang chạy | PASS – ID 2 |
| W5-T06 | Kiểm tra ranh giới testcase | PASS – Không gắn nhầm |
| W5-T07 | Fuzzer hoàn thành, không có lỗi | PASS – `NO_EVENT` |
| W5-T08 | Fuzzer có sự kiện lỗi và chưa hoàn thành | PASS – `SANITIZER_EVENT` |
| W5-T09 | Fuzzer bị gián đoạn, không có cảnh báo | PASS – `INCOMPLETE` |

Các bài kiểm thử liên quan đến nhiều sự kiện, liên kết testcase và trạng thái gián đoạn sử dụng log giả lập để kiểm tra logic chương trình.

### 6.2. Kiểm thử đọc log QEMU thật

Để xác minh khả năng thu thập dữ liệu thực tế, em kích hoạt lại bài SAN-T03 trên QEMU.

Log được lưu vào file `/tmp/week5-qemu.log`, sau đó trích xuất thông báo `[SAN-EVENT]` và đưa vào bộ phân tích Python.

Kết quả:

- QEMU phát hiện và xuất cảnh báo corruption.
- Cảnh báo được ghi lại thành công trong file log.
- Python đọc được cảnh báo.
- Dữ liệu được chuyển sang JSON.
- `event_count` bằng 1.
- Các trường `page`, `offset`, `expected` và `actual` đúng với log gốc.

Kết quả xác nhận luồng **Kernel → QEMU → Log → Python → JSON** hoạt động thành công.

### 6.3. Kiểm thử liên kết sự kiện với testcase

Em sử dụng log giả lập để xác minh cơ chế liên kết testcase.

Trong tình huống thứ nhất, testcase số 2 bắt đầu nhưng gặp cảnh báo trước khi kết thúc.

Bộ phân tích trả về:

`"fuzzer_case_id": "2"`

Trong tình huống thứ hai, một sự kiện xuất hiện sau khi testcase 1 đã kết thúc nhưng trước khi testcase 2 bắt đầu.

Kết quả cho thấy sự kiện này không bị gắn nhầm cho testcase 1.

Khi sự kiện khác xuất hiện trong testcase 3, chương trình gắn đúng:

`"fuzzer_case_id": "3"`

Qua đó, cơ chế theo dõi ranh giới testcase hoạt động đúng trong các tình huống tuần tự đã kiểm tra.

### 6.4. Kiểm thử trạng thái INCOMPLETE

Em tạo một log giả lập trong đó testcase 1 hoàn thành, testcase 2 bắt đầu nhưng không có marker kết thúc.

Log cũng không chứa thông báo `FUZZING COMPLETED` hoặc cảnh báo Sanitizer.

Chương trình trả về:

- `begin_count = 2`
- `end_count = 1`
- `fuzzing_completed = false`
- `run_completed = false`
- `event_count = 0`
- `status = INCOMPLETE`

Kết quả này chứng minh chương trình không đồng nhất việc thiếu cảnh báo với việc hoàn thành thành công.

### 6.5. Kiểm thử hồi quy hệ điều hành

Sau khi hoàn thành các thay đổi Week 5, em chạy bộ kiểm thử `usertests` trên xv6.

| Cấu hình | Bộ kiểm thử | Kết quả |
|---|---|---|
| CPUS=1 | `usertests` | ALL TESTS PASSED |
| CPUS=4 | `usertests` | ALL TESTS PASSED |

Kết quả cho thấy hệ điều hành vẫn vượt qua bộ kiểm thử hồi quy trên cả hai cấu hình đã thực hiện.

Tuy nhiên, kết quả này chưa chứng minh không tồn tại mọi lỗi race condition hoặc vấn đề hiệu năng trong sanitizer.

### 6.6. Đánh giá tổng thể

Từ các thử nghiệm đã thực hiện, có thể rút ra những nhận xét sau:

**Thứ nhất,** cơ chế phát hiện lỗi bộ nhớ của những tuần trước tiếp tục hoạt động với định dạng thông báo mới.

**Thứ hai,** dữ liệu kernel đã được chuyển thành JSON thành công, giúp việc lưu trữ và phân tích kết quả thuận tiện hơn.

**Thứ ba,** chương trình Python đã theo dõi được testcase đang chạy dựa trên marker của Fuzzer.

**Thứ tư,** việc phân loại trạng thái giúp phân biệt lần chạy hoàn thành với lần chạy bị gián đoạn.

**Thứ năm,** các kiểm thử hồi quy đã xác nhận chức năng cơ bản của xv6 tiếp tục hoạt động trên các cấu hình CPU đã chạy.

## 7. Khó khăn, giới hạn và hướng phát triển

### 7.1. Khó khăn trong việc thu thập log QEMU

Trong quá trình thử nghiệm, việc thu log từ QEMU cần bảo đảm thông báo kernel được ghi lại đầy đủ.

Em sử dụng công cụ `script` trên Ubuntu để lưu phiên Terminal vào file log.

Ngoài ra, khi phân tích dữ liệu, chương trình cần loại bỏ các ký tự điều khiển Terminal không liên quan để tránh ảnh hưởng đến việc nhận diện marker.

### 7.2. Vấn đề thực thi Fuzzer

Trong quá trình kiểm thử, có lần xv6 xuất hiện thông báo `exec fuzzer failed`.

Sau khi kiểm tra và tạo lại filesystem image, Fuzzer đã chạy trở lại bình thường.

Sự cố này cho thấy cần bảo đảm chương trình user được biên dịch và có mặt trong filesystem image trước khi chạy kiểm thử.

### 7.3. Giới hạn của cơ chế gắn testcase

Cơ chế hiện tại theo dõi testcase dựa trên các marker `BEGIN` và `END` được in tuần tự.

Nếu nhiều testcase chạy đồng thời hoặc thông báo từ các tiến trình khác xen vào, việc dựa hoàn toàn vào marker có thể chưa đủ để xác định chính xác nguồn phát sinh lỗi.

Trong các giai đoạn tiếp theo, có thể bổ sung thông tin PID, định danh lần chạy và metadata cho từng testcase.

### 7.4. Giới hạn của bộ phân loại kết quả

Chương trình hiện phân loại kết quả dựa trên số marker, thông báo hoàn thành và số sự kiện Sanitizer.

Cơ chế này chưa kiểm tra toàn diện các trường hợp như marker bị lặp, ID testcase không khớp, thứ tự sự kiện bất thường hoặc timeout thực tế.

Ngoài ra, trạng thái `SANITIZER_EVENT` chỉ xác nhận có cảnh báo và không tự cho biết toàn bộ lần chạy đã hoàn thành hay chưa.

### 7.5. Giới hạn của kiểm thử tích hợp

Mặc dù đã kiểm chứng việc phát hiện lỗi thật từ QEMU và việc chạy Fuzzer thật, Week 5 chưa có testcase Fuzzer thực tế trực tiếp tạo ra `FREED_PAGE_CORRUPTION`.

Vì vậy, việc tự động liên kết một lỗi thực tế với testcase gây lỗi trong cùng lần chạy vẫn cần được kiểm tra bổ sung.

Đây là hướng phát triển quan trọng trong tuần tiếp theo.

## 8. Kế hoạch tuần 6

Trong Week 6, em dự kiến tiếp tục hoàn thiện tính ổn định và khả năng tích hợp của Memory Sanitizer với các module còn lại.

Các công việc dự kiến:

1. Phối hợp xây dựng testcase tích hợp có thể kích hoạt cảnh báo Sanitizer trong quá trình chạy Fuzzer.
2. Kiểm tra khả năng ghi nhận testcase gây lỗi trong một lần chạy QEMU thực tế.
3. Mở rộng chương trình phân tích log để xử lý các marker bất thường và dữ liệu thiếu.
4. Nghiên cứu bổ sung thông tin PID hoặc định danh tiến trình vào sự kiện.
5. Phối hợp với module Coverage và Executor để thống nhất định dạng kết quả kiểm thử.
6. Xây dựng quy trình lưu log, tái hiện testcase và tổng hợp sự kiện phục vụ báo cáo cuối kỳ.
7. Tiếp tục kiểm thử hồi quy và đánh giá độ ổn định trên nhiều cấu hình CPU.

## 9. Kết luận

Trong tuần 5, em đã hoàn thành việc chuẩn hóa định dạng cảnh báo Memory Sanitizer và xây dựng bộ công cụ phân tích log bằng Python.

Các chức năng chính bao gồm đọc sự kiện `[SAN-EVENT]`, xuất dữ liệu JSON, nhận diện testcase qua marker `[FUZZ-CASE]`, theo dõi trạng thái hoàn thành và tổng hợp kết quả từ nhiều file kiểm thử.

Các bài kiểm thử xác nhận chương trình có thể đọc cảnh báo thật từ QEMU và xử lý đúng các tình huống giả lập liên quan đến testcase, cảnh báo bộ nhớ và lần chạy bị gián đoạn.

Fuzzer thực tế đã chạy thành công năm testcase với đầy đủ marker `BEGIN` và `END`. Bộ kiểm thử `usertests` trên CPUS=1 và CPUS=4 cũng đều PASS.

Nhìn chung, Week 5 đã tạo ra nền tảng thu thập và phân tích kết quả cần thiết cho quá trình tích hợp giữa Memory Sanitizer và System Call Fuzzer.

Trong những tuần tiếp theo, em sẽ tập trung kiểm chứng luồng tích hợp hoàn chỉnh bằng testcase Fuzzer thực tế có thể kích hoạt cảnh báo bộ nhớ, đồng thời nâng cao độ tin cậy của cơ chế xác định và tái hiện lỗi.

---

## PHỤ LỤC – MÃ NGUỒN VÀ MINH CHỨNG

### A. Các file liên quan

| File | Nội dung |
|---|---|
| `kernel/sanitizer.c` | Chuẩn hóa thông báo lỗi SAN-EVENT |
| `user/fuzzer.c` | Bổ sung marker BEGIN/END |
| `tools/san_log_parser.py` | Phân tích log và xuất JSON |
| `tools/san_summary.py` | Tổng hợp nhiều kết quả kiểm thử |
| `tools/sample-*.log` | Dữ liệu mẫu phục vụ kiểm thử parser |
| `results/week5/` | Các file JSON được tạo trong quá trình kiểm thử |

### B. Commit phát triển

**Nhánh:** `dat/week5-work`

**Commit:** `8c6e0a6`

**Thông điệp:** `SAN-05: integrate sanitizer events with fuzzer logs`

Commit này chứa các thay đổi mã nguồn chính của Week 5 trên nhánh phát triển.

### C. Các minh chứng cần lưu kèm khi tổng hợp báo cáo nhóm

- Log SAN-T03 phát hiện `FREED_PAGE_CORRUPTION` trên QEMU.
- JSON phân tích cảnh báo từ log QEMU thật.
- Log Fuzzer thực hiện 5 testcase với đủ BEGIN/END.
- JSON phân loại `NO_EVENT` trên log Fuzzer thật.
- JSON giả lập liên kết lỗi với testcase số 2.
- JSON giả lập nhận diện trạng thái `INCOMPLETE`.
- Kết quả `usertests` PASS trên CPUS=1 và CPUS=4.

**Ghi chú:** Trong báo cáo và khi trình bày kết quả, cần phân biệt minh chứng từ QEMU thực tế với những log giả lập được sử dụng để kiểm tra thuật toán phân tích.