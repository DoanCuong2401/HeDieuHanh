# Kernel Coverage Data Flow

## 1. Phạm vi

Sơ đồ mô tả thiết kế dự kiến cho prototype coverage.
Chưa phải chức năng đã triển khai.

Mỗi tiến trình được bật coverage có buffer riêng.
Executor đọc coverage của con trực tiếp khi con là ZOMBIE,
trước khi gọi wait để thu hồi con.

## 2. Luồng thu thập và trả dữ liệu

```mermaid
flowchart TD
    A["Executor cha"] -->|"fork và lưu PID"| B["Tiến trình con"]
    B -->|"cov_ctl ENABLE"| C["Cấp buffer và bật thu thập"]
    C --> D["Thực hiện chuỗi syscall"]
    D --> E["Các điểm được gắn trong kernel"]
    E -->|"Ghi mã điểm"| F["Buffer riêng của con"]
    D -->|"exit"| G["Ngừng ghi và chuyển ZOMBIE"]
    F --> H["Snapshot coverage trong kernel"]
    G --> H
    A -->|"cov_read theo PID"| H
    H -->|"Thả khóa rồi copyout"| I["Vùng nhận của cha"]
    I --> J["Lưu log và tìm điểm mới"]
    J --> K["wait thu hồi con"]
    K --> L["freeproc giải phóng buffer"]
```

## 3. Luồng điều khiển của executor

```mermaid
flowchart TD
    A["Tạo con và đặt giới hạn thời gian"] --> B["Gọi cov_read"]
    B --> C{"Kết quả"}
    C -->|"Thành công"| D["Kiểm tra header và lưu coverage"]
    C -->|"-2 Chưa sẵn sàng"| E{"Hết thời gian"}
    E -->|"Chưa"| F["Chờ ngắn"]
    F --> B
    E -->|"Rồi"| G["Xử lý timeout và yêu cầu dừng con"]
    C -->|"Lỗi khác"| H["Ghi lỗi thu thập"]
    D --> I["Thu hồi con bằng wait"]
    G --> I
    H --> I
```

## 4. Dữ liệu giữa các thành phần

| Nguồn | Đích | Dữ liệu |
|---|---|---|
| Executor | Tiến trình con | Testcase, seed và cấu hình chạy |
| Tiến trình con | API điều khiển | RESET, ENABLE hoặc DISABLE |
| Điểm trong kernel | Buffer coverage | Mã điểm thực thi |
| Executor | API đọc | PID con, địa chỉ và kích thước vùng nhận |
| API đọc | User space của executor | version, count, capacity, overflow và mã điểm |
| Executor | Log hoặc corpus | Testcase ID, seed, coverage và trạng thái chạy |

## 5. Quy tắc xử lý

- Executor không gọi wait trước khi đọc xong hoặc quyết định
  bỏ kết quả coverage.
- Không gọi copyout khi giữ spinlock.
- Nếu buffer đầy, dừng ghi thêm và đặt overflow.
- Nếu API đọc trả lỗi, không sử dụng dữ liệu nhận được.
- Nếu con chưa có buffer, ghi nhận thiếu dữ liệu; không coi
  đây là testcase có coverage rỗng hợp lệ.
- Với lỗi thu thập khi con còn chạy, executor phải bảo đảm
  con kết thúc hoặc yêu cầu dừng trước khi thu hồi.
- Khi timeout, phải có giới hạn chờ và cơ chế giám sát;
  kill không bảo đảm xử lý được một kernel đã treo.
- Khi kernel panic hoặc treo, bộ giám sát ngoài QEMU lưu
  testcase và log; không bảo đảm lấy được coverage.
- Coverage của tiến trình cháu chưa được tổng hợp.

## 6. Nội dung cần nhóm xác nhận

- Executor hỗ trợ trình tự đọc coverage trước wait.
- API và định dạng dữ liệu khớp coverage-design.md.
- Một con testcase được thực thi tại một thời điểm cho mỗi executor.
- Thống nhất timeout, kill, thu hồi tiến trình và reset QEMU.
- Thống nhất commit kernel và phiên bản bảng mã điểm.
