# System Call Fuzzer — Testcase Design

## 1. Mục tiêu

Xây dựng tập testcase dùng để kiểm tra grammar, trạng thái tài nguyên, định dạng input, replay và khả năng ghi log của fuzzer.

## 2. Danh sách testcase ban đầu

| ID | Nhóm | Chuỗi thao tác minh họa | Mục tiêu |
|---|---|---|---|
| TC01 | Syscall đơn | `getpid` | Kiểm tra sinh và thực thi một syscall |
| TC02 | File | `open -> read -> close` | Kiểm tra vòng đời descriptor |
| TC03 | File | `open -> write -> close` | Kiểm tra thao tác ghi |
| TC04 | Descriptor | `close` với descriptor không hợp lệ | Kiểm tra xử lý tham số sai |
| TC05 | File | `open -> close -> close` | Kiểm tra thao tác trên descriptor đã đóng |
| TC06 | Pipe | `pipe -> write/read -> close` | Kiểm tra hai đầu pipe và vòng đời tài nguyên |
| TC07 | Process | `fork -> wait` | Kiểm tra trạng thái tiến trình |
| TC08 | Process | `fork -> exit -> wait` | Kiểm tra kết thúc và thu hồi tiến trình |
| TC09 | Memory | `sbrk` với giá trị biên phù hợp | Kiểm tra hành vi thay đổi kích thước vùng nhớ |
| TC10 | Kết hợp | File + process + pipe | Kiểm tra chuỗi nhiều nhóm syscall |
| TC11 | Tham số biên | Độ dài đọc/ghi bằng 0 hoặc giá trị biên phù hợp | Kiểm tra xử lý biên |
| TC12 | Replay | Chạy lại một seed cố định | Kiểm tra khả năng tái hiện đầu vào |

## 3. Thông tin cần ghi nhận

Mỗi lần chạy cần ghi testcase ID, seed, chuỗi đầu vào, kết quả syscall, trạng thái kết thúc, timeout/panic nếu phát hiện được và đường dẫn log.

Các testcase phải được đối chiếu với syscall thực tế có trong source. Các chuỗi chỉ mang tính minh họa cho đến khi được chạy và kiểm thử.

## 4. Tiêu chí hoàn thành

- Các testcase có mục tiêu kiểm tra rõ ràng.
- Input có thể lưu thành seed và đọc lại.
- Có testcase replay.
- Có cách phân biệt lỗi syscall thông thường với sự cố kernel.
- Kết quả chạy thực tế được ghi riêng; không đánh dấu PASS khi chưa chạy.
