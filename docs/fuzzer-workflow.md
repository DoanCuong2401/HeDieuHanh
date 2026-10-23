# Syscall Fuzzer Workflow

## Luồng xử lý

1. **Syscall inventory:** lấy danh sách syscall từ source xv6.
2. **Sequence generator:** sinh syscall và tham số.
3. **Executor:** chạy chuỗi syscall trong xv6.
4. **Coverage collector:** ghi nhận đường thực thi đã đi qua.
5. **Crash monitor:** phát hiện panic hoặc kernel bị treo.
6. **Input minimization:** rút gọn chuỗi syscall gây lỗi.
7. **Reproducer:** lưu chuỗi tối thiểu để nhóm kiểm tra và sửa lỗi.

## Các module cần thống nhất

- Generator: định dạng đầu vào và chiến lược sinh chuỗi.
- Executor: cách chạy và thu thập kết quả.
- Coverage: cách ghi nhận và sử dụng coverage.
- Sanitizer: cách phát hiện use-after-free.
- Crash triage: cách lưu log và tái hiện lỗi.

## Tiêu chí bàn giao

- Có thể chạy lại một chuỗi syscall đã lưu.
- Có thể nhận biết kernel panic hoặc treo.
- Có thể lưu đầu vào gây lỗi.
- Có tài liệu hướng dẫn chạy thử và đánh giá kết quả.
