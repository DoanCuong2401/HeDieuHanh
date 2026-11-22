# BÁO CÁO TIẾN ĐỘ TUẦN 6

## MEMORY SANITIZER – END-TO-END FUZZER INTEGRATION

**Học phần:** Hệ điều hành  
**Đề tài:** System Call Fuzzer and Kernel Sanitizers  
**Sinh viên:** Trần Tiến Đạt – MSSV 20233313  
**Module phụ trách:** Memory Sanitizer & Reliability  
**Nền tảng:** xv6-RISC-V, Ubuntu, QEMU  
**Nhánh phát triển:** `dat/week6-work`

---

## 1. Tóm tắt công việc

Trong tuần 6, em tiếp tục phát triển phần tích hợp giữa System Call Fuzzer và Memory Sanitizer trên hệ điều hành xv6-RISC-V.

Ở tuần 5, em đã xây dựng hệ thống chuẩn hóa cảnh báo `[SAN-EVENT]`, chương trình Python phân tích log và cơ chế đánh dấu testcase bằng `[FUZZ-CASE]`. Tuy nhiên, việc liên kết một cảnh báo Sanitizer với testcase Fuzzer mới được kiểm thử bằng log giả lập.

Mục tiêu của tuần 6 là kiểm chứng toàn bộ quy trình tích hợp bằng một lần chạy thực tế trên QEMU.

Để thực hiện, em đã bổ sung syscall kiểm thử `san_test()`, cho phép Fuzzer chủ động kích hoạt bài kiểm thử Memory Sanitizer SAN-T03.

Các công việc chính gồm:

1. Bổ sung chế độ kiểm thử riêng bằng cờ biên dịch `SAN_W6_TEST`.
2. Xây dựng syscall `san_test()` để gọi bài kiểm thử Sanitizer trong kernel.
3. Đăng ký syscall tại các thành phần kernel và user space.
4. Mở rộng chương trình Fuzzer với testcase tích hợp đặc biệt.
5. Cải thiện khả năng xử lý dữ liệu log của chương trình Python.
6. Chạy kiểm thử tích hợp thực tế trên QEMU.
7. Thu thập log và chuyển kết quả thành JSON.
8. Xác nhận cảnh báo được liên kết chính xác với testcase số 6.
9. Kiểm tra bản build thông thường khi tắt chế độ kiểm thử.
10. Kiểm thử hồi quy hệ điều hành trên CPUS=1 và CPUS=4.

**Kết quả:** Hệ thống đã kiểm chứng thành công toàn bộ chuỗi tích hợp từ testcase Fuzzer đến cảnh báo Sanitizer và kết quả JSON. Bản build thông thường không kích hoạt syscall gây lỗi, và bộ kiểm thử hồi quy xv6 đều PASS trên hai cấu hình CPU.

## 2. Mục tiêu và phạm vi thực hiện

### 2.1. Mục tiêu

Mục tiêu chính của Week 6 là hoàn thiện quy trình kiểm thử tích hợp:

**Fuzzer → System Call → Kernel Sanitizer → QEMU Log → Python Parser → JSON**

Hệ thống cần xác định được testcase đang thực thi khi Memory Sanitizer phát hiện lỗi bộ nhớ.

Các yêu cầu cụ thể:

- Fuzzer có khả năng gọi một syscall kiểm thử chuyên dụng.
- Syscall kích hoạt bài kiểm thử Memory Sanitizer có sẵn.
- Sanitizer phát hiện lỗi và xuất cảnh báo chuẩn hóa.
- Log QEMU lưu được marker testcase và sự kiện Sanitizer.
- Python phân tích và liên kết sự kiện với testcase chính xác.
- Chế độ kiểm thử có thể bật hoặc tắt thông qua cấu hình biên dịch.
- Bản build thông thường vẫn hoạt động ổn định.

### 2.2. Phạm vi

Week 6 tập trung vào kiểm thử tích hợp có kiểm soát.

Lỗi bộ nhớ được tạo ra chủ động bằng bài kiểm thử SAN-T03 đã xây dựng từ trước.

Việc kích hoạt này nhằm kiểm tra khả năng tích hợp giữa các module, không phải phát hiện một lỗi chưa biết bằng thuật toán fuzzing.

Các thuật toán poison checking và cơ chế phát hiện `FREED_PAGE_CORRUPTION` không cần viết lại.

## 3. Thiết kế syscall kiểm thử Memory Sanitizer

### 3.1. Vấn đề của phiên bản trước

Fuzzer hiện tại sử dụng các system call:

- `getpid()`
- `uptime()`
- `pause()`

Các syscall trên phục vụ kiểm thử chức năng và khả năng thực thi cơ bản, nhưng không được thiết kế để tạo ra tình huống write-after-free trong kernel.

Vì vậy, khi chạy Fuzzer bình thường, chương trình không tạo ra cảnh báo Sanitizer để kiểm chứng việc liên kết sự kiện với testcase.

Để giải quyết vấn đề này, em bổ sung một syscall chuyên dụng dành riêng cho thử nghiệm.

### 3.2. Xây dựng sys_san_test()

Trong file `kernel/sysproc.c`, em bổ sung hàm `sys_san_test()`.

Chức năng của hàm là gọi bài kiểm thử `sanitizer_integration_test()` đã có từ tuần 3.

Bài kiểm thử này thực hiện tình huống cố tình thay đổi dữ liệu trong một trang bộ nhớ đã được giải phóng.

Sau đó, khi trang được cấp phát lại, Memory Sanitizer phát hiện giá trị poison bị thay đổi và gọi `panic()`.

Syscall được đặt trong khối điều kiện:

`#ifdef SAN_W6_TEST`

Nhờ đó, chức năng cố tình kích hoạt lỗi chỉ được biên dịch vào kernel khi bật chế độ kiểm thử.

### 3.3. Đăng ký syscall

Để user space có thể gọi `san_test()`, em thực hiện các thay đổi tại:

| File | Chức năng |
|---|---|
| `kernel/syscall.h` | Định nghĩa mã syscall |
| `kernel/syscall.c` | Khai báo và đăng ký hàm xử lý syscall |
| `kernel/sysproc.c` | Cài đặt hàm `sys_san_test()` |
| `user/user.h` | Khai báo giao diện gọi syscall |
| `user/usys.pl` | Sinh mã stub để chuyển từ user space vào kernel |

Phần đăng ký hàm xử lý trong `kernel/syscall.c` cũng được bảo vệ bằng điều kiện `SAN_W6_TEST`.

Điều này bảo đảm bản build thông thường không tham chiếu đến hàm `sys_san_test()` khi hàm không được biên dịch.

### 3.4. Bật chế độ kiểm thử từ Makefile

Trong `Makefile`, em bổ sung cấu hình:

```makefile
ifeq ($(SAN_W6_TEST),1)
CFLAGS += -DSAN_W6_TEST
endif
```

Để bật chế độ kiểm thử, sử dụng:

```bash
make clean
make SAN_W6_TEST=1 CPUS=1 qemu
```

Để chạy bản build thông thường:

```bash
make clean
make CPUS=1 qemu
```

Việc thực hiện `make clean` khi chuyển chế độ giúp tránh sử dụng lại các file object được biên dịch với cấu hình khác.

## 4. Tích hợp syscall vào System Call Fuzzer

### 4.1. Mở rộng chương trình Fuzzer

Trong file `user/fuzzer.c`, em bổ sung một chế độ chạy kiểm thử Sanitizer thông qua tham số `san`.

Lệnh thực thi:

`fuzzer 1 5 san`

Trong đó:

- `1`: giá trị seed.
- `5`: số testcase thông thường.
- `san`: yêu cầu chạy thêm testcase tích hợp Sanitizer khi chế độ kiểm thử được bật.

Fuzzer thực hiện 5 testcase thông thường trước, sau đó gọi syscall `san_test()` trong testcase số 6.

### 4.2. Đánh dấu testcase

Trước khi gọi syscall, Fuzzer xuất marker:

`[FUZZ-CASE] id=6 status=BEGIN`

Nếu syscall thực thi thành công và quay trở lại, chương trình có thể xuất marker `END`.

Tuy nhiên, trong bài kiểm thử này, Memory Sanitizer được thiết kế để gọi `panic()` sau khi phát hiện lỗi.

Do đó, testcase 6 không có marker `END`.

Đây là hành vi phù hợp với tình huống kiểm thử gây lỗi có chủ đích.

### 4.3. Phân biệt hai chế độ chạy

| Chế độ | Lệnh | Hành vi |
|---|---|---|
| Bình thường | `fuzzer 1 5` | Chạy 5 testcase |
| Kiểm thử tích hợp | `fuzzer 1 5 san` | Chạy 5 testcase và testcase Sanitizer số 6 khi bật cờ |
| Không bật `SAN_W6_TEST` | `fuzzer 1 5 san` | Chỉ chạy 5 testcase thông thường |

Nhờ vậy, việc kiểm thử gây lỗi không ảnh hưởng đến quy trình chạy Fuzzer trong bản build bình thường.

## 5. Nâng cấp chương trình phân tích log

### 5.1. Vấn đề xử lý dữ liệu sai định dạng

Trong quá trình phát triển, em tiếp tục cải thiện chương trình `tools/san_log_parser.py`.

Ở phiên bản trước, khi phân tích một sự kiện `[SAN-EVENT]`, chương trình giả định tất cả trường dữ liệu đều có dạng:

`key=value`

Nếu có một trường không chứa dấu `=`, thao tác tách dữ liệu có thể gây lỗi chương trình.

### 5.2. Cải thiện khả năng xử lý

Trong Week 6, em bổ sung điều kiện kiểm tra trường dữ liệu trước khi phân tách.

Những trường không chứa dấu `=` được bỏ qua.

Chương trình cũng kiểm tra tên trường trước khi thêm vào dictionary kết quả.

Thay đổi này giúp giảm khả năng chương trình bị dừng khi gặp dữ liệu log không đúng định dạng dự kiến.

### 5.3. Cơ chế liên kết testcase

Bộ phân tích tiếp tục sử dụng hai marker:

`[FUZZ-CASE] id=N status=BEGIN`

`[FUZZ-CASE] id=N status=END`

Khi xuất hiện cảnh báo `[SAN-EVENT]` trong thời gian một testcase đang thực thi, chương trình gắn mã testcase vào trường:

`fuzzer_case_id`

Trong Week 6, cơ chế này được kiểm thử bằng log QEMU thực tế có chứa cả marker và cảnh báo Sanitizer trong cùng một lần chạy.

## 6. Kiểm thử tích hợp thực tế trên QEMU

### 6.1. Môi trường kiểm thử

Môi trường thực nghiệm:

- Hệ điều hành: xv6-RISC-V.
- Trình giả lập: QEMU.
- Môi trường phát triển: Ubuntu.
- Cấu hình kiểm thử tích hợp: `CPUS=1`.
- Cờ biên dịch: `SAN_W6_TEST=1`.
- Chương trình thử nghiệm: `fuzzer 1 5 san`.

### 6.2. Quy trình kiểm thử

Các bước thực hiện:

1. Biên dịch kernel với cờ `SAN_W6_TEST=1`.
2. Khởi động xv6 trên QEMU.
3. Chạy Fuzzer với 5 testcase thông thường.
4. Kích hoạt testcase số 6 thông qua syscall `san_test()`.
5. Thực thi bài kiểm thử SAN-T03 trong kernel.
6. Memory Sanitizer phát hiện lỗi bộ nhớ.
7. Kernel xuất thông báo `[SAN-EVENT]` và gọi `panic()`.
8. Thu thập log QEMU.
9. Phân tích log bằng Python.
10. Kiểm tra testcase ID được liên kết với cảnh báo.

### 6.3. Kết quả từ QEMU

Khi chạy:

`fuzzer 1 5 san`

Fuzzer thực hiện thành công 5 testcase đầu tiên.

Sau đó log xuất hiện:

```text
[FUZZ-CASE] id=6 status=BEGIN
[SAN-T03] Starting integration test
[DEBUG] page=0x0000000087f23000 byte100=0x63
[SAN-EVENT] type=FREED_PAGE_CORRUPTION page=0x0000000087f23000 offset=100 expected=0x1 actual=0x63
panic: memory sanitizer
```

Kết quả cho thấy:

- Testcase 6 bắt đầu thực thi.
- Syscall đã kích hoạt bài kiểm thử trong kernel.
- Bộ kiểm tra poison phát hiện trang đã giải phóng bị thay đổi.
- Vị trí lỗi là byte thứ 100.
- Giá trị mong đợi là `0x01`.
- Giá trị thực tế là `0x63`.
- Kernel dừng bằng `panic()` theo thiết kế.

Địa chỉ trang bộ nhớ có thể khác nhau giữa các lần chạy QEMU.

### 6.4. Thu thập log thực tế

Em sử dụng công cụ `script` trên Ubuntu để ghi lại phiên chạy QEMU.

Lệnh thực hiện:

```bash
script -q -f -c "make SAN_W6_TEST=1 CPUS=1 qemu" /tmp/week6-qemu.log
```

Từ log đầy đủ, em trích xuất các dòng chứa marker `[FUZZ-CASE]` và sự kiện `[SAN-EVENT]`.

Các dòng được giữ theo thứ tự xuất hiện để bảo đảm việc liên kết testcase không bị sai lệch.

### 6.5. Phân tích log thành JSON

Sau khi thu thập log, em sử dụng chương trình:

```bash
python3 tools/san_log_parser.py \
  /tmp/week6-fuzzer-events.log \
  W6-INTEGRATION-001
```

Kết quả được ghi nhận với các trường quan trọng:

```json
{
    "testcase_id": "W6-INTEGRATION-001",
    "begin_count": 6,
    "end_count": 5,
    "fuzzing_completed": false,
    "run_completed": false,
    "status": "SANITIZER_EVENT",
    "event_count": 1,
    "events": [
        {
            "type": "FREED_PAGE_CORRUPTION",
            "offset": "100",
            "expected": "0x1",
            "actual": "0x63",
            "fuzzer_case_id": "6"
        }
    ]
}
```

Đoạn JSON trên chỉ trình bày các trường chính; file kết quả thực tế còn chứa địa chỉ `page`.

### 6.6. Xác minh kết quả tự động

Kết quả được lưu tại:

`results/week6/real/W6-INTEGRATION-001.json`

Sau đó em sử dụng Python để kiểm tra các điều kiện:

- `begin_count == 6`
- `end_count == 5`
- `status == "SANITIZER_EVENT"`
- `event_count == 1`
- `fuzzer_case_id == "6"`
- `type == "FREED_PAGE_CORRUPTION"`

Chương trình xác nhận:

`W6-INTEGRATION-001: PASS`

Đây là minh chứng cho việc toàn bộ chuỗi tích hợp hoạt động trên dữ liệu thật từ QEMU.

### 6.7. Ý nghĩa kết quả

So với Week 5, Week 6 đạt được bước tiến quan trọng:

- Week 5 xác nhận khả năng phân tích log lỗi thật và liên kết testcase trên dữ liệu giả lập.
- Week 6 xác nhận việc liên kết trực tiếp một sự kiện Sanitizer thật với testcase Fuzzer đã kích hoạt nó.

Cần lưu ý rằng testcase số 6 là testcase chủ động gây lỗi nhằm xác minh cơ chế tích hợp, chưa phải kết quả Fuzzer tự động khám phá một lỗi bộ nhớ mới.

## 7. Kiểm tra chế độ build bình thường

### 7.1. Mục tiêu

Sau khi kiểm thử chế độ gây lỗi, cần bảo đảm chức năng kiểm thử chuyên dụng không ảnh hưởng đến hoạt động bình thường của hệ điều hành.

### 7.2. Thực hiện

Em biên dịch lại hệ thống mà không bật cờ `SAN_W6_TEST`:

```bash
make clean
make CPUS=1 qemu
```

Sau khi xv6 khởi động thành công, em thực hiện:

`fuzzer 1 5 san`

### 7.3. Kết quả

Hệ thống ghi nhận:

- 5 testcase được thực thi.
- 5 marker `BEGIN`.
- 5 marker `END`.
- Xuất hiện `=== FUZZING COMPLETED ===`.
- Không kích hoạt testcase số 6.
- Không xuất hiện cảnh báo Sanitizer.
- Kernel không bị panic.

Kết quả xác nhận cơ chế kiểm thử riêng không hoạt động khi tắt cờ biên dịch.

## 8. Kiểm thử hồi quy hệ điều hành

### 8.1. Mục tiêu

Các thay đổi liên quan đến syscall và Fuzzer không được làm ảnh hưởng đến hoạt động bình thường của xv6.

### 8.2. Kết quả

Sau khi hoàn thiện code, em chạy bộ kiểm thử `usertests` trên bản build thông thường.

| Cấu hình | Bộ kiểm thử | Kết quả |
|---|---|---|
| CPUS=1 | `usertests` | ALL TESTS PASSED |
| CPUS=4 | `usertests` | ALL TESTS PASSED |

Kết quả cho thấy các chức năng được kiểm tra bởi `usertests` tiếp tục hoạt động sau những thay đổi của Week 6.

Tuy nhiên, đây chưa phải kiểm thử đầy đủ về hiệu năng, race condition hoặc khả năng chịu tải của Memory Sanitizer.

## 9. Bảng tổng hợp kết quả Week 6

| Mã kiểm thử | Nội dung | Kết quả |
|---|---|---|
| W6-T01 | Build xv6 với `SAN_W6_TEST=1` | PASS |
| W6-T02 | Fuzzer chạy 5 testcase bình thường | PASS |
| W6-T03 | Testcase số 6 gọi syscall kiểm thử | PASS |
| W6-T04 | Memory Sanitizer phát hiện lỗi | PASS |
| W6-T05 | Kernel xuất `[SAN-EVENT]` | PASS |
| W6-T06 | Thu log QEMU thực tế | PASS |
| W6-T07 | Phân tích log thành JSON | PASS |
| W6-T08 | Liên kết lỗi với `fuzzer_case_id=6` | PASS |
| W6-T09 | Kiểm tra JSON tự động | PASS |
| W6-T10 | Build thông thường không gây panic | PASS |
| W6-T11 | `usertests` với CPUS=1 | PASS |
| W6-T12 | `usertests` với CPUS=4 | PASS |

Kết quả cho thấy các mục tiêu kiểm thử chính của Week 6 đã được thực hiện thành công.

## 10. Khó khăn và cách khắc phục

### 10.1. Lỗi liên kết khi tắt SAN_W6_TEST

Trong quá trình kiểm thử bản build thông thường, hệ thống xuất hiện lỗi:

`undefined reference to 'sys_san_test'`

Nguyên nhân là có vị trí trong bảng syscall vẫn tham chiếu đến hàm kiểm thử trong khi hàm này không được biên dịch.

Em khắc phục bằng cách bảo đảm phần khai báo và đăng ký `sys_san_test()` đều được đặt trong điều kiện `SAN_W6_TEST`.

### 10.2. Lỗi điều kiện biên dịch

Một lần biên dịch xuất hiện thông báo:

`#endif without #if`

Lỗi phát sinh do khối điều kiện biên dịch trong `kernel/sysproc.c` chưa cân đối.

Sau khi điều chỉnh lại cặp `#ifdef` và `#endif`, chương trình đã biên dịch thành công.

### 10.3. Phân biệt môi trường Ubuntu và xv6

Chương trình `fuzzer` được biên dịch để chạy trong môi trường user space của xv6.

Vì vậy, lệnh này phải được thực hiện trong shell xv6 sau khi QEMU khởi động, không chạy trực tiếp trong Terminal Ubuntu.

### 10.4. Kiểm thử gây kernel panic

Bài kiểm thử tích hợp chủ động gây corruption để Sanitizer phát hiện.

Khi phát hiện lỗi, kernel gọi `panic()`, khiến lần chạy không thể tiếp tục.

Điều này không phải lỗi của quá trình tích hợp, mà là hành vi dự kiến của bài kiểm thử.

Do đó, việc thu thập log và phân tích kết quả cần được thực hiện bên ngoài QEMU.

## 11. Giới hạn hiện tại

Mặc dù đã hoàn thành kiểm thử tích hợp thực tế, hệ thống vẫn có những giới hạn:

**Thứ nhất,** sự kiện corruption được tạo chủ động bởi testcase chuyên dụng. Chưa có bằng chứng Fuzzer phát hiện một lỗi bộ nhớ chưa biết.

**Thứ hai,** cơ chế liên kết testcase dựa trên các marker xuất hiện tuần tự trong log, chưa được mở rộng để phân biệt nhiều tiến trình chạy đồng thời.

**Thứ ba,** bộ phân tích log chưa xử lý toàn diện các trường hợp marker lặp, ID không hợp lệ hoặc thứ tự sự kiện bất thường.

**Thứ tư,** cơ chế kiểm thử hiện sử dụng `panic()` để kết thúc khi phát hiện lỗi, chưa hỗ trợ phục hồi kernel và tiếp tục chạy những testcase còn lại trong cùng phiên.

**Thứ năm,** chức năng tự động tái hiện testcase gây lỗi và tích hợp với bộ thu thập coverage vẫn cần tiếp tục phát triển.

## 12. Kế hoạch Week 7

Trong tuần tiếp theo, em dự kiến tập trung vào nâng cao khả năng tự động hóa và độ tin cậy của hệ thống:

1. Cải thiện cơ chế xác thực thứ tự marker testcase.
2. Bổ sung định danh riêng cho từng lần chạy Fuzzer.
3. Xử lý các trường hợp log thiếu, sai định dạng hoặc bị gián đoạn.
4. Nghiên cứu cách lưu thông tin cần thiết để tái hiện testcase gây lỗi.
5. Phối hợp với module Coverage và Crash Monitor của nhóm.
6. Chuẩn hóa cách lưu trữ dữ liệu kết quả kiểm thử.
7. Bổ sung các bài kiểm thử để bảo đảm các trường hợp bất thường không làm chương trình phân tích log bị dừng.
8. Tiếp tục kiểm thử hồi quy xv6 trên nhiều cấu hình CPU.

## 13. Kết luận

Trong Week 6, em đã hoàn thiện bước kiểm thử tích hợp thực tế giữa System Call Fuzzer và Memory Sanitizer trên xv6-RISC-V.

Thông qua syscall `san_test()` và chế độ biên dịch `SAN_W6_TEST`, Fuzzer có thể chủ động kích hoạt bài kiểm thử gây corruption trong kernel.

Memory Sanitizer đã phát hiện lỗi, xuất cảnh báo `[SAN-EVENT]` và gọi `panic()` đúng thiết kế.

Log QEMU được thu thập và phân tích thành JSON. Bộ phân tích đã liên kết chính xác cảnh báo `FREED_PAGE_CORRUPTION` với testcase số 6.

Kết quả `W6-INTEGRATION-001: PASS` xác nhận toàn bộ quy trình tích hợp đã hoạt động trên dữ liệu thực tế.

Bản build thông thường cũng được kiểm chứng hoạt động ổn định. Bộ kiểm thử `usertests` đều PASS trên CPUS=1 và CPUS=4.

Nhìn chung, Week 6 đã hoàn thành mục tiêu kiểm chứng đầu-cuối giữa Fuzzer, Memory Sanitizer và chương trình phân tích log. Đây là nền tảng quan trọng để nhóm tiếp tục phát triển quy trình phát hiện, lưu trữ và tái hiện lỗi tự động trong những tuần tiếp theo.

---

## PHỤ LỤC – MÃ NGUỒN VÀ MINH CHỨNG

### A. Các file mã nguồn đã thay đổi

| File | Nội dung |
|---|---|
| `Makefile` | Cấu hình cờ `SAN_W6_TEST` |
| `kernel/sysproc.c` | Cài đặt syscall kiểm thử |
| `kernel/syscall.c` | Đăng ký syscall có điều kiện |
| `kernel/syscall.h` | Định nghĩa mã syscall |
| `user/user.h` | Khai báo syscall phía user |
| `user/usys.pl` | Tạo syscall stub |
| `user/fuzzer.c` | Bổ sung testcase số 6 |
| `tools/san_log_parser.py` | Cải thiện xử lý log |

### B. Minh chứng thực nghiệm

- Log QEMU ghi nhận testcase 6.
- Cảnh báo `[SAN-EVENT]` với `FREED_PAGE_CORRUPTION`.
- Thông báo `panic: memory sanitizer`.
- File `W6-INTEGRATION-001.json`.
- Kết quả kiểm tra `W6-INTEGRATION-001: PASS`.
- Bản build thông thường chạy Fuzzer không gây panic.
- Kết quả `usertests` PASS ở CPUS=1 và CPUS=4.

### C. Nhánh GitHub

**Nhánh phát triển:** `dat/week6-work`

**Repository:** `DoanCuong2401/HeDieuHanh`

**Nội dung:** Các thay đổi mã nguồn phục vụ tích hợp Memory Sanitizer và Fuzzer trong Week 6.

Báo cáo Week 6 được chuẩn bị để nộp riêng trên nhánh `dat/week6`, không trộn với mã nguồn phát triển.