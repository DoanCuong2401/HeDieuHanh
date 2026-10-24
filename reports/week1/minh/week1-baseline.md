# Báo cáo baseline xv6 tuần 1

Người thực hiện: Đinh Anh Minh
Vai trò: Kernel Coverage
Ngày thực hiện: 22/10/2026

## 1. Mục tiêu

Chuẩn bị môi trường và kiểm thử xv6 nguyên bản để làm mốc
đối chiếu khi tích hợp coverage, fuzzer và sanitizer.

## 2. Môi trường

- Hệ điều hành: Ubuntu 22.04.5 LTS.
- Compiler: riscv64-unknown-elf-gcc 10.2.0.
- QEMU: 7.2.0.
- GNU Make: 4.3.
- Repository: https://github.com/mit-pdos/xv6-riscv.git
- Commit: 06aad25c735fd3159bdfae5680be4eab7b1668b2
- Cấu hình kiểm thử: CPUS=1, RAM 128 MiB.
- Sử dụng worktree riêng để giữ nguyên bài làm trong thư mục cũ.

## 3. Kết quả

- Biên dịch kernel và chương trình người dùng thành công.
- Khởi động xv6 và vào được shell.
- Chạy ls và echo hello thành công.
- usertests hoàn tất với kết luận ALL TESTS PASSED.
- grind có báo tiến độ; chưa ghi nhận kernel panic trong log.
- Phiên QEMU chạy grind kéo dài 12 phút 26 giây.
  Thời lượng này bao gồm cả khởi động và thao tác trong shell;
  chưa đo riêng thời gian thực thi grind.
- Kết thúc phiên bằng thao tác thoát QEMU.
- Chưa kiểm thử cấu hình CPUS=4.

## 4. Minh chứng

- logs/week1-environment.txt
- logs/week1-build.txt
- logs/week1-build-check.txt
- logs/week1-build-status.txt
- logs/week1-qemu-cpu1.txt
- logs/week1-grind-cpu1.txt

## 5. Kế hoạch tiếp theo

- Đối chiếu commit nền với phiên bản nhóm và giảng viên yêu cầu.
- Bổ sung ảnh minh chứng và bàn giao kết quả cho nhóm.
- Tuần 2: khảo sát luồng syscall, chọn mức coverage,
  thiết kế buffer và thống nhất giao diện với fuzzer.
