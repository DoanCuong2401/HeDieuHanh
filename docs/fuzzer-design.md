# System Call Fuzzer Design

## 1. Mục tiêu

Thiết kế module System Call Fuzzer cho xv6-RISC-V chạy trên QEMU. Fuzzer sinh các chuỗi system call có trạng thái, hỗ trợ seed cố định, replay testcase và lưu log để phục vụ kiểm thử, phát hiện và tái hiện lỗi kernel.

Phạm vi tuần 2 là thiết kế và thống nhất giao diện. Việc triển khai prototype được thực hiện ở tuần 3.

## 2. Kiến trúc đề xuất

Luồng xử lý:

1. Đọc seed hoặc khởi tạo seed mới.
2. Sinh chuỗi system call theo grammar và trạng thái tài nguyên.
3. Kiểm tra và tạo tham số cho từng syscall.
4. Thực thi chuỗi trong môi trường xv6-RISC-V/QEMU.
5. Ghi lại kết quả syscall và trạng thái chạy.
6. Phát hiện dấu hiệu panic, crash hoặc timeout bằng cơ chế giám sát.
7. Lưu testcase và log để replay hoặc phân tích.

Coverage và sanitizer là các module phối hợp với fuzzer. API trao đổi dữ liệu cần được thống nhất với người phụ trách trước khi tích hợp.

## 3. Phân loại system call

Các nhóm ban đầu:

- Process: `fork`, `exec`, `exit`, `wait`, `kill`, `getpid`.
- File và descriptor: `open`, `read`, `write`, `close`, `fstat`, `stat`, `link`, `unlink`, `mkdir`, `mknod`, `chdir`, `dup`.
- Pipe và truyền dữ liệu: `pipe`, kết hợp với `read`, `write`, `close`.
- Bộ nhớ: `sbrk`.
- Thời gian và hệ thống: `uptime`, `sleep`.
- Syscall bổ sung: chỉ đưa vào nếu có trong source code của branch đang kiểm thử.

Danh sách cuối cùng và số hiệu syscall phải đối chiếu với `docs/syscall-inventory.md` và các file syscall trong source hiện tại. Không giả định mọi syscall trong danh sách ví dụ đều có mặt trong mọi phiên bản xv6.

## 4. Stateful grammar

Mỗi testcase gồm nhiều thao tác syscall. Việc sinh thao tác tiếp theo dựa trên trạng thái tài nguyên đã biết.

Ví dụ:

- File: `open -> read/write -> close`.
- Pipe: `pipe -> write/read -> close`.
- Process: `fork -> thực hiện thao tác trong tiến trình con -> wait`.
- Bộ nhớ: `sbrk` để thay đổi kích thước vùng nhớ, sau đó kiểm tra hành vi với các tham số phù hợp.

Fuzzer phải cập nhật trạng thái sau mỗi thao tác. Không sử dụng descriptor, PID hoặc địa chỉ bộ nhớ như thể chúng luôn hợp lệ sau khi tài nguyên đã bị đóng, tiến trình đã kết thúc hoặc vùng nhớ đã thay đổi.

Bên cạnh chuỗi hợp lệ, fuzzer có thể sinh trường hợp biên và thao tác sai có chủ đích. Kết quả trả về lỗi từ syscall phải được phân biệt với panic, crash hoặc timeout của kernel.

## 5. Chiến lược seed

Mỗi seed cần có định danh, giá trị seed ngẫu nhiên, chuỗi thao tác và phiên bản định dạng.

Seed ban đầu nên bao gồm:

- Chuỗi gọi syscall đơn giản.
- Chuỗi quản lý file descriptor.
- Chuỗi thao tác pipe.
- Chuỗi fork/wait.
- Chuỗi kết hợp nhiều nhóm syscall.
- Trường hợp tham số biên hoặc tài nguyên không hợp lệ.

Khi có testcase gây lỗi, giữ nguyên seed và chuỗi thao tác gốc để có thể tái hiện. Mọi biến thể sinh ra từ seed nên được ghi nhận liên kết với seed cha nếu triển khai hỗ trợ.

## 6. Định dạng input đề xuất

Ở phiên bản thiết kế, sử dụng định dạng văn bản có cấu trúc, mỗi dòng biểu diễn một thao tác:

`SYSCALL argument1 argument2 ...`

Ví dụ minh họa:

```text
open /README 0
read 3 0 10
close 3
```

Đây là ví dụ định dạng, không đảm bảo descriptor `3` luôn được cấp phát cho `open`. Khi triển khai, fuzzer cần ánh xạ kết quả thực tế của syscall vào trạng thái descriptor thay vì giả định giá trị cố định.

Định dạng chính thức, cách biểu diễn chuỗi và xử lý tham số phải được thống nhất trước khi triển khai parser.

## 7. Replay và ghi log

Replay phải sử dụng lại đúng chuỗi thao tác đã lưu, theo cùng thứ tự và cùng các tham số đã ghi nhận. Không sinh lại tham số ngẫu nhiên trong chế độ replay.

Mỗi lần chạy nên ghi:

- Mã testcase và seed.
- Chuỗi syscall đầu vào.
- Kết quả trả về của từng syscall nếu thu thập được.
- Trạng thái kết thúc của chương trình.
- Dấu hiệu panic hoặc timeout nếu có bằng chứng từ môi trường chạy.
- Log liên quan và thông tin phiên bản chương trình/kernel.

Phương thức reset QEMU, timeout và thu thập log cần được thống nhất trong quá trình thiết kế executor.

## 8. Pseudocode

```text
initialize(seed)
load_or_generate_sequence()

for each operation in sequence:
    validate_operation_format(operation)
    prepare_arguments_using_resource_state()
    result = execute_syscall(operation)
    update_resource_state(operation, result)
    record_operation_and_result()

    if panic_or_timeout_detected():
        save_input_seed_and_log()
        stop_current_testcase()

save_testcase_result()
```

Pseudocode trên mô tả luồng thiết kế, chưa phải mã triển khai.

## 9. Giao diện cần thống nhất

- Generator: cách biểu diễn chuỗi và tham số.
- Executor: cách chạy testcase, reset môi trường và timeout.
- Coverage: cách lấy dữ liệu coverage và định nghĩa loại coverage.
- Sanitizer: định dạng cảnh báo, mức độ nghiêm trọng và log.
- Reproducer: định dạng testcase lưu trữ và cách chạy lại.

Các API cụ thể cần được thống nhất với người phụ trách coverage và sanitizer trước khi tích hợp.

## 10. Tiêu chí nghiệm thu thiết kế

- Có danh sách syscall và phân loại tham số dựa trên source thực tế.
- Có grammar mô tả chuỗi syscall có trạng thái.
- Có định dạng seed và input đề xuất.
- Có pseudocode cho generator, executor và replay.
- Có testcase mẫu bao gồm trường hợp bình thường, biên và lỗi có chủ đích.
- Có định nghĩa dữ liệu log và các giao diện đang chờ nhóm thống nhất.
