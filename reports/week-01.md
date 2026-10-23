# PROJECT: 26134 – System Call Fuzzer and Kernel Sanitizers

**PLATFORM:** xv6-RISC-V / QEMU  
**WEEK / DATE:** Tuần 1 | [Điền ngày bắt đầu] – [Điền ngày kết thúc]

## 1. Mục tiêu tuần
- Xây dựng môi trường phát triển trên Ubuntu, khởi tạo Git repository (`main`, `dev`, `cuong/fuzzer`) và quy tắc phân nhánh[cite: 1].
- Liệt kê danh sách system call của xv6 và lên thiết kế sơ đồ luồng cho module fuzzer[cite: 1].
- Cài đặt công cụ QEMU/RISC-V toolchain, kiểm tra build/boot kernel và thiết lập Task Board quản lý công việc[cite: 1].

## 2. Tiến độ từng thành viên
| THÀNH VIÊN | TASK ĐÃ GIAO | KẾT QUẢ / % HOÀN THÀNH | BẰNG CHỨNG |
| :--- | :--- | :--- | :--- |
| **Đoàn Mạnh Cường** | Tạo repo, quy tắc branch, liệt kê syscall, sơ đồ fuzzer. | Đã hoàn thành (100%)[cite: 1] | Thư mục `docs/` trên nhánh `cuong/fuzzer` |
| **Đinh Anh Minh** | Cài đặt toolchain RISC-V, QEMU, kiểm tra build/boot xv6. | Đã hoàn thành / Đang tiến hành[cite: 1] | Log build / Ảnh QEMU |
| **Trần Tiến Đạt** | Tạo Task Board, khảo sát `kalloc.c`, tạo mẫu quản lý lỗi. | Đã hoàn thành / Đang tiến hành[cite: 1] | Link Task Board & tài liệu khảo sát |

## 3. Kiểm thử và demo
- **Build/Boot:** [Chờ cập nhật kết quả make qemu từ Minh][cite: 1]
- **Module Test:** Đã hoàn thành danh sách toàn bộ system call chuẩn bị cho việc thiết kế grammar ở Tuần 2.

## 4. Vấn đề, rủi ro, việc chưa hoàn thành
- Không có vấn đề nghiêm trọng. Các thành viên đang đồng bộ môi trường làm việc trên Ubuntu.

## 5. Kế hoạch tuần tiếp theo (Tuần 2 — Thiết kế hệ thống)
- **Cường:** Phân loại syscall và tham số; thiết kế grammar/sequence có trạng thái, seed, input format, replay (`docs/fuzzer-design.md`)[cite: 1].
- **Minh:** Khảo sát syscall dispatch; thiết kế coverage buffer, API reset/read, đo lường overhead (`docs/coverage-design.md`)[cite: 1].
- **Đạt:** Khảo sát lifecycle trang trong `kalloc/kfree`; thiết kế poison/check/unpoison (`docs/sanitizer-design.md`)[cite: 1].

## 6. Link minh chứng
- **Repository:** https://github.com/DoanCuong2401/HeDieuHanh.git[cite: 1]
- **Task Board:** [Điền link Task Board của nhóm][cite: 1]
