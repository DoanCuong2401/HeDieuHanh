import json
import sys

# Nhận tên file log từ dòng lệnh
log_file = sys.argv[1]

testcase_id = sys.argv[2] if len(sys.argv) > 2 else "UNKNOWN"

events = []
current_case = None

begin_count = 0
end_count = 0
fuzzing_completed = False
# Đọc từng dòng trong file log
with open(log_file, "r") as f:
    for line in f:
        line = line.strip()
        
        # Kiem tra Fuzzer hoan thanh
        if line == "=== FUZZING COMPLETED ===":
            fuzzing_completed = True
            continue

        # Nhan dien testcase bat dau hoac ket thuc
        if line.startswith("[FUZZ-CASE]"):
            fields = {}

            for item in line.split()[1:]:
                if "=" in item:
                    key, value = item.split("=", 1)
                    fields[key] = value

            if fields.get("status") == "BEGIN":
                begin_count += 1
                current_case = fields.get("id")

            elif fields.get("status") == "END":
                end_count += 1
                current_case = None

            continue

        # Chỉ xử lý dòng chứa sự kiện Sanitizer
        if not line.startswith("[SAN-EVENT]"):
            continue

        event = {}

        # Tách các trường key=value
        fields = line.strip().split()[1:]

        for field in fields:
            key, value = field.split("=", 1)
            event[key] = value
        
        # Gan su kien loi voi testcase dang chay
        if current_case is not None:
            event["fuzzer_case_id"] = current_case

        events.append(event)


# Kiem tra lan chay Fuzzer da hoan thanh hay chua
run_completed = (
    begin_count > 0
    and begin_count == end_count
    and current_case is None
    and fuzzing_completed
)

# Phan loai ket qua
if len(events) > 0:
    status = "SANITIZER_EVENT"
elif run_completed:
    status = "NO_EVENT"
else:
    status = "INCOMPLETE"

# Chuyển danh sách sự kiện thành JSON
result = {
    "testcase_id": testcase_id,
    "begin_count": begin_count,
    "end_count": end_count,
    "fuzzing_completed": fuzzing_completed,
    "run_completed": run_completed,
    "status": status,
    "event_count": len(events),
    "events": events
}

print(json.dumps(result, indent=4))