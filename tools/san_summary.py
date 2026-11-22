import json
import sys
from pathlib import Path

# Nhận thư mục chứa kết quả JSON
results_dir = Path(sys.argv[1])

total_tests = 0
tests_with_events = 0
total_events = 0

# Đọc lần lượt các file JSON
for file_path in sorted(results_dir.glob("*.json")):

    with open(file_path, "r") as f:
        result = json.load(f)

    testcase_id = result["testcase_id"]
    event_count = result["event_count"]

    total_tests += 1
    total_events += event_count

    if event_count > 0:
        tests_with_events += 1
        status = "SANITIZER_EVENT"
    else:
        status = "NO_EVENT"

    print(f"{testcase_id}: {status} ({event_count} events)")

# In báo cáo tổng hợp
print("\n=== SANITIZER SUMMARY ===")
print(f"Total testcases: {total_tests}")
print(f"Testcases with events: {tests_with_events}")
print(f"Total sanitizer events: {total_events}")