# Báo cáo Kernel Coverage tuần 4

Người thực hiện: Đinh Anh Minh
Ngày: 15/11/2026
Branch: minh/week4

## 1. Mục tiêu

Bổ sung kiểm thử API và vòng đời tiến trình để hoàn thiện,
xác minh giao diện coverage v1 từ prototype tuần 3.

## 2. Công việc đã thực hiện

- Tạo minh/week4 từ branch coverage tuần 3.
- Merge origin/main, bổ sung thay đổi fuzzer tuần 4 của Cường.
- Thêm user/covapitest.c và user/covexectest.c.
- Đăng ký hai chương trình trong Makefile.
- Giữ nguyên mã triển khai kernel coverage tuần 3.

## 3. Giao diện coverage v1

- cov_ctl(op): RESET=1, ENABLE=2, DISABLE=3.
- cov_read(pid, buf, bytes): trả số byte sao chép khi thành công.
- pid=0: đọc bản thân khi đã tắt thu thập.
- pid>0: chỉ đọc con trực tiếp ở trạng thái ZOMBIE, trước wait.
- Mã lỗi: -1 lỗi chung, -2 đang bận, -3 vùng nhận nhỏ,
  -4 chưa có buffer.
- Buffer chứa tối đa 1024 mã uint 32 bit.
- Header gồm version, count, capacity, overflow; kích thước 16 byte.
- Vùng nhận tối đa 4112 byte.
- Bật lại không tự reset; đọc không tự xóa dữ liệu.
- Khi đầy, giữ phần đầu buffer, đặt overflow và bỏ sự kiện mới.
- Chỉ chấp nhận dữ liệu phía user khi cov_read trả thành công.

Ví dụ thu thập trong tiến trình hiện tại:

    cov_ctl(COV_RESET);
    cov_ctl(COV_ENABLE);
    /* Thực hiện syscall cần đo. */
    cov_ctl(COV_DISABLE);
    n = cov_read(0, &result, sizeof(result));

Chương trình phải kiểm tra kết quả từng lời gọi API.
Không in log trong khoảng thu thập nếu không muốn ghi syscall write.

## 4. Kiểm thử bổ sung

covapitest gồm 11 kiểm tra:

- Kích thước nhận âm và bằng 0.
- Copyout vào vùng text chỉ đọc.
- Địa chỉ nhận tại cuối bộ nhớ user.
- Dữ liệu nguồn được giữ sau lỗi sao chép.
- Bật lại giữ các điểm đã ghi.
- 50 vòng reset và tái sử dụng.
- Từ chối đọc con chưa kết thúc.
- Đọc con sau khi đồng bộ kết thúc.
- Con không được đọc coverage của cha.
- Từ chối PID con sau khi wait thu hồi.

covexectest gồm 3 kiểm tra:

- Giữ coverage và trạng thái bật qua exec thành công.
- 32 vòng fork/exec/wait với trạng thái coverage mới của con.
- Giữ coverage khi exec thất bại.

## 5. Kết quả

- CPUS=1: coverage_test, covchildtest, covreadtest,
  covapitest và covexectest đều PASS, tổng 44 kiểm tra.
- CPUS=4: covapitest và covexectest đều PASS,
  tổng 14 kiểm tra bổ sung.
- usertests trên CPUS=1: ALL TESTS PASSED.
- Môi trường: QEMU, RAM 128 MiB.

## 6. Minh chứng

- logs/week4/coverage-exec-cpu4.txt
- logs/week4/coverage-regression-cpu1.txt
- Log usertests CPUS=1 trong phiên regression hoặc phiên riêng
  logs/week4/usertests-cpu1.txt nếu đã chạy riêng.

## 7. Giới hạn và kế hoạch tiếp theo

- Chưa đo overhead hoặc coverage growth.
- Chưa tích hợp feedback coverage với fuzzer.
- Chưa thử lỗi cấp phát bằng fault injection.
- Các vòng tái sử dụng chưa chứng minh không rò rỉ bộ nhớ.
- Chưa tổng hợp coverage tiến trình cháu hoặc lấy dữ liệu khi panic.
- Chuẩn bị phối hợp tích hợp ba module theo kế hoạch tuần 5.
