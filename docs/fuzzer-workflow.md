
# System Call Fuzzer Workflow

## 1. Sơ đồ luồng hoạt động toàn hệ thống

```mermaid
flowchart TD
    A["Syscall Inventory"] --> B["Sequence Generator"]
    B --> C["Testcase Corpus / Seed"]
    C --> D["Executor / Test Runner"]
    D --> E["User Test Program"]
    E --> F["xv6-RISC-V Kernel trên QEMU"]

    F --> G["Coverage Collector"]
    F --> H["Sanitizer"]
    F --> I["Crash Monitor"]

    G --> J["Kết quả kiểm thử"]
    H --> J
    I --> J

    J --> K{"Có lỗi hoặc coverage mới?"}
    K -- "Có lỗi" --> L["Crash Triage"]
    L --> M["Input Minimization"]
    M --> N["Reproducer + Log"]
    N --> C

    K -- "Coverage mới" --> C
    K -- "Không có" --> B

    style A fill:#e8f1ff,stroke:#3972ac
    style B fill:#e8f1ff,stroke:#3972ac
    style F fill:#e5f5e9,stroke:#39834a
    style G fill:#fff2d8,stroke:#c58b24
    style H fill:#f4e8ff,stroke:#8751ad
    style I fill:#fce7e7,stroke:#b84b4b
```

## 2. Mô tả các module

1. **Syscall Inventory:** lấy danh sách system call từ source xv6.
2. **Sequence Generator:** sinh chuỗi system call và tham số.
3. **Testcase Corpus:** lưu seed và các testcase có giá trị.
4. **Executor:** chạy chương trình kiểm thử trên xv6-RISC-V bằng QEMU.
5. **Coverage Collector:** ghi nhận đường thực thi đã đi qua.
6. **Sanitizer:** hỗ trợ phát hiện các lỗi bộ nhớ, bao gồm use-after-free khi có cơ chế kiểm tra phù hợp.
7. **Crash Monitor:** phát hiện panic, crash hoặc kernel bị treo.
8. **Crash Triage và Input Minimization:** phân loại lỗi, rút gọn testcase gây lỗi.
9. **Reproducer:** lưu testcase tối thiểu và log để nhóm tái hiện, phân tích.

## 3. Tiêu chí bàn giao

- Chạy lại được một chuỗi system call đã lưu.
- Phát hiện được kernel panic hoặc timeout theo cơ chế giám sát đã triển khai.
- Lưu được testcase và log khi phát hiện lỗi.
- Có thể tái hiện lỗi và đánh giá coverage.
- Có tài liệu hướng dẫn chạy thử và đánh giá kết quả.

> Sơ đồ mô tả kiến trúc mục tiêu. Các chức năng cần được kiểm thử thực tế trước khi xác nhận đã triển khai hoàn chỉnh.

