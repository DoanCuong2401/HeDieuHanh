# Báo cáo Kernel Coverage tuần 5

**Đề tài 26134 — System Call Fuzzer and Kernel Sanitizers**

**Sinh viên:** Đinh Anh Minh — 20233522

**Nhánh:** `minh/week5` · **Mốc tuần 4:** `ad3eae2`

## 1. Mục tiêu

Dùng API coverage của tuần 4 trong một chương trình demo có kết quả dễ đọc. Hai ca thực hiện cùng syscall `read()` trên hai loại đối tượng. Kết quả cần cho thấy syscall trả về bao nhiêu byte và kernel đã đi qua nhánh nào.

## 2. Triển khai

- `user/covdemo.c` mở `README` và chuẩn bị một pipe chứa byte `K`. Mỗi ca gọi `cov_ctl(RESET)`, `cov_ctl(ENABLE)`, thực hiện đúng một lần `read()`, gọi `cov_ctl(DISABLE)` rồi đọc kết quả bằng `cov_read(0, ...)`.
- Việc mở file, ghi dữ liệu chuẩn bị cho pipe và in báo cáo diễn ra ngoài khoảng thu coverage. Mỗi ca kiểm tra kích thước dữ liệu trả về, phiên bản, dung lượng bộ đệm, số điểm và cờ tràn trước khi dùng các mã điểm.
- Chương trình so sánh giá trị trả về của `read()` với kỳ vọng, kiểm tra byte nhận ở ca pipe và đối chiếu thứ tự các điểm. Nó trả mã thoát khác 0 khi có ca lỗi. `Makefile` đưa `covdemo` vào ảnh đĩa xv6.

| Ca | Giá trị `read()` kỳ vọng | Điểm coverage kỳ vọng |
|---|---:|---|
| Đọc một byte từ file `README` | 1 | `1 261 4 8` |
| Đọc byte `K` từ pipe | 1 | `1 261 4 6` |

Cả hai đường chạy đều vào bộ phân phối syscall hợp lệ (`1`), gọi `read` (`261 = 256 + 5`) và chuyển yêu cầu xuống tầng xử lý file (`4`). Điểm cuối phân biệt đọc file lưu trên hệ thống tệp (`8`) và đọc pipe (`6`). Số điểm phản ánh các vị trí đã gắn đo trong kernel.

## 3. Cách chạy lại

Trong Terminal Ubuntu, mở repo ở nhánh `minh/week5` và chạy:

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

Thoát QEMU bằng `Ctrl+A` rồi `X`. Chạy lại với `make CPUS=4 qemu` để đối chiếu cấu hình nhiều CPU.

## 4. Kết quả

Môi trường kiểm thử dùng GCC RISC-V 14.2.0, QEMU 10.0.13 và 128 MiB RAM. Bản build tạo được `kernel/kernel`, `user/_covdemo` và `fs.img`. Kết quả runtime được ghi trong `logs/week5/cpu1.txt` và `logs/week5/cpu4.txt`.

| Kiểm tra | CPUS=1 | CPUS=4 |
|---|---|---|
| `covdemo` chạy hai lượt | Cả hai ca PASS ở mỗi lượt | Cả hai ca PASS ở mỗi lượt |
| Năm chương trình kiểm thử coverage của tuần 4 | PASS | PASS |
| `usertests -q` | ALL TESTS PASSED | ALL TESTS PASSED |

Bộ hồi quy xv6 trong tuần này chạy với tùy chọn `-q`. Các điểm đo vẫn theo phạm vi của tuần 4 và chương trình demo thu trên chính tiến trình đang chạy.

## 5. Mốc bàn giao phần coverage

| Tuần | Đầu việc | Tiêu chí xong |
|---|---|---|
| 5 | Demo API trên hai đường xử lý `read()` và chạy hồi quy | Log phân biệt được file và pipe, lưu minh chứng 1 và 4 CPU |
| 6 | Tổng hợp coverage theo từng ca và cả bộ ca kiểm thử cố định | Xuất số điểm ghi nhận, số điểm duy nhất và cờ tràn. Có ca kiểm tra các điểm trùng và so sánh kết quả giữa các lượt |
| 7 | Nghiệm thu phần coverage trên bản chung, viết hướng dẫn demo và báo cáo tổng kết | Chạy lại bộ kiểm thử cố định trên 1 và 4 CPU, đối chiếu kết quả theo từng ca và ghi rõ phạm vi các điểm đã gắn đo |

Tuần 6 và tuần 7 tiếp nối tuần 5 bằng commit và báo cáo riêng.
