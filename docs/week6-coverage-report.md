# Báo cáo Kernel Coverage tuần 6

**Đề tài 26134 — System Call Fuzzer and Kernel Sanitizers**

**Sinh viên** Đinh Anh Minh — 20233522

**Nhánh** `minh/week6` · **Commit nền** `52a1c85` của `minh/week5`

## 1. Mục tiêu

Bổ sung thống kê coverage theo từng ca và cả bộ ca cố định của `covdemo`. Kiểm tra phép đếm khi cùng một điểm kernel được ghi nhiều lần trong một ca hoặc xuất hiện ở nhiều ca.

## 2. Triển khai

- `user/covdemo.c` giữ ca đọc file README và ca đọc pipe. Ca `file-repeat` gọi `read()` hai lần trên cùng file trong một khoảng thu thập để tạo hai chuỗi mã điểm giống nhau.
- Mỗi ca đọc bản ghi coverage sau khi tắt thu thập. Chương trình xác minh phiên bản, dung lượng, số điểm, cờ tràn và số byte API trả về trước khi duyệt các mã điểm.
- `recorded` đếm số sự kiện đã ghi, kể cả các lần lặp. `unique` đếm mỗi mã điểm một lần trong phạm vi đang báo cáo. Tổng toàn bộ ca lấy hợp các mã điểm, còn cờ `overflow` bật nếu bất kỳ bản ghi hợp lệ nào báo tràn.
- Dòng `snapshots` cho biết số bản ghi hợp lệ trên tổng số ca dự kiến. Ca thiếu bản ghi, sai kết quả, sai chuỗi mã điểm hoặc báo tràn làm kết quả chung FAIL. Khi tràn, các số đếm chỉ áp dụng cho phần đã lưu.

## 3. Kết quả QEMU

| Ca | Sự kiện đã ghi | Mã điểm duy nhất | Cờ tràn |
|---|---:|---:|---:|
| `file` | 4 | 4 | 0 |
| `file-repeat` | 8 | 4 | 0 |
| `pipe` | 4 | 4 | 0 |
| Cả bộ ca · 3/3 bản ghi | 16 | 5 | 0 |

Hai lượt `covdemo` trên mỗi cấu hình CPUS=1 và CPUS=4 cho cùng kết quả. Năm chương trình kiểm thử coverage cho 44 kiểm tra PASS trên mỗi cấu hình. Trong đó, `coverage_test` kiểm tra cờ tràn khi bộ đệm đầy và kiểm tra RESET xóa cờ. `usertests -q` kết thúc với `ALL TESTS PASSED` trên cả hai cấu hình.

Môi trường kiểm thử dùng GCC RISC-V 14.2.0, QEMU 10.0.13 và 128 MiB RAM. Xem log `logs/week6/cpu1.txt` và `logs/week6/cpu4.txt`.

## 4. Cách chạy lại

Trong Terminal Ubuntu ở nhánh `minh/week6`:

```bash
make -j"$(nproc)" kernel/kernel fs.img
make CPUS=1 qemu
```

Khi shell xv6 hiện dấu `$`, nhập từng lệnh:

```text
covdemo
covdemo
coverage_test
covchildtest
covreadtest
covapitest
covexectest
usertests -q
```

Thoát QEMU bằng `Ctrl+A` rồi `X`. Chạy lại với `make CPUS=4 qemu` để đối chiếu.

## 5. Mục tiêu tuần 7

Chạy lại bộ ca cố định và bộ hồi quy coverage trên bản tích hợp của nhóm ở 1 và 4 CPU. Đối chiếu các số đếm theo từng ca, cập nhật cách chạy demo và chốt phạm vi các đường kernel đã gắn điểm đo. Năm mã điểm duy nhất ở tuần 6 là số quan sát được trong bộ ca, chưa có mẫu số để quy đổi thành phần trăm độ phủ kernel.
