# Kernel Coverage Design

## 1. Mục tiêu và phạm vi

Thiết kế cơ chế ghi nhận coverage trong kernel xv6 để cung cấp
phản hồi cho fuzzer: testcase vừa chạy có đi qua điểm thực thi
mới hay không.

Tuần 2 thực hiện khảo sát và thiết kế. Cơ chế coverage chưa
được triển khai hoặc kiểm thử.

## 2. Mã nguồn khảo sát

- Repository: https://github.com/mit-pdos/xv6-riscv.git
- Commit: 06aad25c735fd3159bdfae5680be4eab7b1668b2
- Thư mục khảo sát: ~/xv6-week1-baseline
- Tài liệu được viết trên branch minh/week2 của repository nhóm.
- Cần thống nhất commit tích hợp với cả nhóm trước khi code.

## 3. Luồng xử lý syscall

Ví dụ syscall read:

1. Chương trình người dùng gọi read(fd, buffer, n).
2. Đoạn chuyển tiếp do user/usys.pl sinh ra đặt SYS_read vào
   thanh ghi a7, thực hiện ecall.
3. Sau khi đi qua mã vào trap, usertrap() nhận syscall tại nhánh
   r_scause() == 8 và gọi syscall().
4. syscall() đọc số hiệu từ trapframe->a7, kiểm tra số hiệu,
   tra bảng syscalls[] và gọi sys_read().
5. sys_read() lấy địa chỉ buffer, số byte và kiểm tra descriptor.
   Descriptor không hợp lệ dẫn tới trả về -1; nếu hợp lệ,
   hàm gọi fileread().
6. Nếu hàm xử lý trả về, syscall() lưu kết quả vào
   trapframe->a0 để trả lại chương trình người dùng.

Các file đã khảo sát:

- user/user.h
- user/usys.pl
- kernel/trap.c
- kernel/syscall.h
- kernel/syscall.c
- kernel/sysfile.c

Lưu ý phiên bản:

- SYS_read = 5.
- SYS_pause = 13; cần đồng bộ với tên sleep trong tài liệu fuzzer.
- SYS_sync = 22.
- Số hiệu syscall coverage chưa được cấp.

## 4. Đề xuất định nghĩa coverage

Prototype dự kiến dùng các mã điểm được gắn thủ công tại những
vị trí xử lý đã chọn trong kernel.

Đề xuất điểm ghi nhận ban đầu:

- Đường phân phối syscall hợp lệ.
- Đường xử lý số hiệu syscall không hợp lệ.
- Đường sys_read từ chối descriptor không hợp lệ.
- Đường sys_read chuyển tiếp tới fileread.

Đi tới fileread không đồng nghĩa với đọc thành công; cần khảo sát
fileread để xác định thêm các điểm xử lý bên trong.

Mỗi điểm cần có mã định danh riêng và bảng ánh xạ tới vị trí mã.
Cách đo này chỉ phản ánh các điểm đã gắn, chưa phải coverage
toàn bộ nhánh của kernel. Chỉ ghi số hiệu syscall là không đủ
để phân biệt các đường xử lý bên trong.

Đây là đề xuất, cần được nhóm thống nhất trước khi triển khai.

## 5. Nội dung thiết kế tiếp theo

- Phạm vi thu thập và vòng đời dữ liệu theo tiến trình.
- Cấu trúc buffer, dung lượng và cách xử lý khi đầy.
- API reset/read và cơ chế bật/tắt thu thập.
- Định dạng dữ liệu xuất về user space.
- Cách phối hợp với fork, exec và exit.
- Chi phí bộ nhớ, thời gian ghi nhận và kế hoạch đo overhead.
- Sơ đồ luồng dữ liệu và kế hoạch kiểm thử.

## 6. Đề xuất buffer coverage

### 6.1. Phạm vi thu thập

Prototype thu thập các điểm đã gắn trong kernel khi kernel
xử lý thay cho tiến trình được bật coverage.

Mỗi tiến trình được bật coverage có buffer riêng. Không ghi
hoạt động của các tiến trình không liên quan; chưa thu thập
đường xử lý ngắt và hoạt động nền.

Đây là coverage tại các điểm được gắn thủ công, không phải
coverage toàn bộ kernel.

### 6.2. Cấu trúc dữ liệu dự kiến

Bổ sung trạng thái quản lý coverage cho tiến trình:

- enabled: có đang thu thập hay không.
- buffer: con trỏ tới vùng lưu các mã điểm.
- count: số phần tử đã ghi.
- capacity: sức chứa tối đa.
- overflow: có sự kiện bị bỏ do buffer đầy hay không.

Đề xuất cấp phát riêng một trang 4096 byte khi bật coverage
lần đầu. Mỗi mã điểm dùng kiểu uint 32 bit, tương ứng buffer
chứa tối đa 1024 phần tử. Các trường quản lý nằm ngoài trang
dữ liệu này.

Không đặt mảng 4096 byte trên kernel stack.

Nếu cấp phát thất bại, trả về lỗi và giữ coverage ở trạng thái
tắt. Không làm tiến trình hoặc kernel bị panic vì lỗi này.

### 6.3. Quy tắc ghi

Khi đi qua một điểm đã gắn:

1. Kiểm tra có tiến trình hiện tại và coverage đang bật.
2. Nếu count nhỏ hơn capacity, ghi mã điểm vào buffer[count].
3. Tăng count sau khi ghi.
4. Nếu buffer đầy, đặt overflow và bỏ các sự kiện tiếp theo.

Không ghi vượt giới hạn và không ghi đè dữ liệu đầu buffer.
Giữ các mã lặp theo thứ tự xuất hiện; fuzzer loại trùng khi
tính tập điểm mới.

Hàm ghi không in log và không cấp phát bộ nhớ tại từng sự kiện.

### 6.4. Reset và đọc dữ liệu

Reset đặt count và overflow về 0; chưa cần xóa toàn bộ trang
vì dữ liệu hợp lệ được giới hạn bởi count.

Chỉ đọc khi đã tắt thu thập. Đọc không tự xóa dữ liệu.
Dữ liệu trả về gồm số phần tử, cờ overflow và danh sách mã điểm.

Nếu vùng nhận không đủ, trả lỗi và giữ nguyên buffer để
chương trình có thể thử lại với vùng nhận lớn hơn.

Chuyển dữ liệu từ kernel về user space bằng copyout.
Phiên bản mã nguồn khảo sát có chữ ký:

copyout(pagetable, psz, dstva, src, len)

Phải kiểm tra kích thước và kết quả copyout. Nếu sao chép lỗi,
không coi kết quả phía user là hợp lệ; buffer kernel được giữ.

Các syscall điều khiển coverage phải được loại khỏi thu thập
để tránh tự ghi nhận thao tác reset hoặc đọc dữ liệu.

### 6.5. Giới hạn cần thống nhất với fuzzer

Thiết kế buffer riêng cho tiến trình chưa giải quyết việc
tiến trình cha đọc kết quả sau khi tiến trình con kết thúc.

Cần chốt chính sách fork, exec, exit và quyền đọc dữ liệu trước
khi hoàn thiện API. Chưa mặc định rằng buffer được kế thừa hoặc
chia sẻ giữa cha và con.

Khi kernel panic, không thể dựa vào syscall đọc coverage để
lấy kết quả. Việc lưu coverage trong tình huống này cần được
thiết kế riêng với cơ chế giám sát.

## 7. Đề xuất vòng đời buffer và thu thập từ tiến trình con

Phần này bổ sung chính sách cho các vấn đề còn mở tại mục 6.5.
Đây là thiết kế đề xuất, chưa được triển khai hoặc thống nhất
chính thức với các thành viên.

### 7.1. Khởi tạo tiến trình

Trong allocproc, khởi tạo trạng thái coverage trước các thao tác
cấp phát có thể thất bại và gọi freeproc:

- buffer = 0.
- enabled = 0.
- count = 0.
- capacity = 0.
- overflow = 0.

Chỉ cấp phát trang buffer khi tiến trình yêu cầu bật coverage.
Mục đích là tránh cấp phát cho mọi tiến trình không được fuzz.

### 7.2. Chính sách fork

Tiến trình con không tự kế thừa buffer hoặc trạng thái bật
coverage của cha. Không sao chép con trỏ buffer từ cha sang con.

Executor tạo tiến trình con để chạy testcase. Tiến trình con
chủ động bật coverage trước khi thực hiện chuỗi syscall.

Nếu testcase tiếp tục fork, tiến trình cháu mặc định không được
thu thập. Prototype chưa tổng hợp coverage của cả cây tiến trình.
Cần ghi rõ giới hạn này khi đánh giá các testcase fork/wait.

### 7.3. Chính sách exec

Giữ buffer kernel, dữ liệu và trạng thái bật coverage khi exec
thành công vì tiến trình vẫn giữ nguyên danh tính.

Không lưu lâu dài con trỏ tới buffer trong user space.
Chương trình sau exec phải cấp vùng nhận mới nếu muốn đọc dữ liệu.

Nếu exec thất bại, giữ nguyên trạng thái coverage hiện có.

### 7.4. Chính sách exit và wait

Cho phép ghi các điểm đã chọn trong đường xử lý exit.
Trước khi công bố trạng thái ZOMBIE, ngừng ghi coverage.

Giữ buffer trong tiến trình ZOMBIE để cha có thể đọc dữ liệu.
Không giải phóng buffer ngay khi tiến trình gọi exit.

Trong freeproc, giải phóng buffer nếu tồn tại và đặt lại
các trường quản lý. Quy tắc này áp dụng cả khi thu hồi tiến trình
con bằng wait và khi dọn một tiến trình khởi tạo thất bại.

Sau khi wait thu hồi tiến trình, coverage của nó không còn
đọc được.

### 7.5. Quyền đọc

Đề xuất hai trường hợp được đọc:

- Tiến trình đọc buffer của chính nó khi đã tắt thu thập.
- Tiến trình cha đọc buffer của con trực tiếp khi con là ZOMBIE.

Không cho phép đọc tùy ý coverage của tiến trình khác.

Khi con còn chạy, API đọc từ cha trả trạng thái chưa sẵn sàng;
không lấy dữ liệu đang thay đổi.

### 7.6. Trình tự phối hợp với executor

1. Cha tạo tiến trình con và lưu PID.
2. Con bật coverage, thực hiện testcase rồi kết thúc.
3. Cha gọi API kiểm tra/đọc coverage theo PID của con.
4. Nếu con chưa kết thúc, cha chờ ngắn rồi thử lại, có timeout.
5. Khi con là ZOMBIE, cha đọc và lưu coverage.
6. Sau khi lưu kết quả, cha gọi wait để thu hồi con.

Không gọi wait để chờ con trước bước đọc coverage, vì wait
hiện tại sẽ giải phóng tiến trình và buffer.

Prototype ưu tiên một tiến trình con chạy testcase tại một thời
điểm cho mỗi executor. Timeout, kill và thu hồi con cần phối hợp
với thiết kế executor của Cường.

### 7.7. Đồng bộ và sao chép dữ liệu

Kiểm tra quan hệ cha-con và trạng thái tiến trình theo quy tắc
khóa của proc.c. Khi cần giữ cả hai khóa, tuân theo thứ tự
wait_lock trước proc lock.

Không gọi copyout khi đang giữ spinlock.

Đề xuất chụp dữ liệu vào vùng tạm của kernel khi đã xác nhận
buffer ổn định, sau đó thả khóa trước khi copyout về user space.
Kiểm tra lỗi cấp phát và giới hạn kích thước vùng tạm.

Không đặt toàn bộ bản sao buffer 4096 byte trên kernel stack.

### 7.8. Giới hạn

- Chưa thu thập tự động coverage của tiến trình cháu.
- Nếu cha kết thúc và con được chuyển cho init, không bảo đảm
  coverage của testcase được lưu.
- Kernel panic có thể ngăn việc đọc coverage bằng syscall.
- Các API và chính sách trên cần được nhóm duyệt trước khi code.

## 8. Đề xuất API coverage phiên bản 1

Các API dưới đây là đặc tả thiết kế, chưa được triển khai.
Tên, số hiệu syscall và định dạng cần được nhóm thống nhất.

### 8.1. API điều khiển

Chữ ký phía user dự kiến:

```c
int cov_ctl(int op);
```

Chỉ điều khiển coverage của tiến trình gọi.

| op | Tên dự kiến | Hành vi |
|---|---|---|
| 1 | COV_RESET | Đặt count và overflow về 0 |
| 2 | COV_ENABLE | Cấp buffer nếu chưa có và bật thu thập |
| 3 | COV_DISABLE | Tắt thu thập, giữ dữ liệu |

Quy tắc:

- RESET chỉ thực hiện khi coverage đang tắt.
- RESET khi chưa cấp buffer được coi là thành công, dữ liệu rỗng.
- ENABLE không tự xóa dữ liệu cũ; muốn bắt đầu ca mới phải RESET.
- ENABLE khi đã bật và DISABLE khi đã tắt đều thành công.
- Nếu cấp buffer thất bại, giữ trạng thái tắt và trả lỗi.
- Không cho phép điều khiển coverage của tiến trình khác.

Giá trị trả về:

- 0: thành công.
- -1: op không hợp lệ hoặc cấp phát thất bại.
- -2: yêu cầu RESET khi coverage còn bật.

### 8.2. API đọc

Chữ ký phía user dự kiến:

```c
int cov_read(int pid, void *buf, int buf_bytes);
```

Tham số:

- pid = 0: đọc coverage của chính tiến trình gọi.
- pid > 0: đọc coverage của con trực tiếp có PID tương ứng.
- pid < 0: không hợp lệ.
- buf: địa chỉ vùng nhận trong user space.
- buf_bytes: kích thước vùng nhận, tính bằng byte.

Điều kiện:

- Đọc bản thân chỉ khi đã tắt coverage.
- Đọc con chỉ khi con là ZOMBIE và chưa bị wait thu hồi.
- Chỉ đọc, không reset và không thu hồi tiến trình.
- Không chấp nhận một tiến trình không phải con trực tiếp,
  dù người gọi biết PID của tiến trình đó.

Giá trị trả về:

- Số byte đã sao chép: thành công.
- -1: tham số, quyền truy cập, cấp vùng tạm hoặc copyout bị lỗi.
- -2: chưa sẵn sàng; bản thân còn bật hoặc con chưa là ZOMBIE.
- -3: vùng nhận không đủ.
- -4: tiến trình được phép đọc nhưng chưa có buffer coverage.

Nếu đọc thất bại, không xóa dữ liệu kernel.
copyout có thể đã ghi một phần vào vùng nhận trước khi thất bại;
phía user chỉ sử dụng kết quả khi API trả về thành công.

### 8.3. Định dạng kết quả

Dữ liệu trả về gồm header và danh sách mã điểm, theo thứ tự:

```c
uint version;
uint count;
uint capacity;
uint overflow;
uint point_ids[count];
```

Đây là mô tả bố trí dữ liệu, chưa phải khai báo C hoàn chỉnh.

Quy ước:

- Mỗi uint có kích thước 4 byte trong ABI dự kiến.
- version = 1.
- count: số mã điểm hợp lệ.
- capacity: sức chứa buffer, dự kiến 1024.
- overflow = 1 nếu có sự kiện bị bỏ do đầy; ngược lại bằng 0.
- point_ids giữ cả mã lặp theo thứ tự ghi.
- Chỉ xuất count mã điểm, không xuất phần bộ nhớ chưa dùng.

Số byte trả về:

16 + 4 * count

Kích thước nhận tối đa cần chuẩn bị:

16 + 4 * 1024 = 4112 byte

4112 byte là vùng nhận kết quả phía user, khác với trang buffer
4096 byte trong kernel vì kết quả còn có header 16 byte.

Không xuất con trỏ hoặc địa chỉ buffer kernel.

Phía user phải kiểm tra version, count không vượt capacity
và số byte trả về khớp với header trước khi xử lý dữ liệu.

### 8.4. Trình tự sử dụng

Tiến trình con:

```text
cov_ctl(COV_RESET)
cov_ctl(COV_ENABLE)
thực hiện chuỗi syscall của testcase
exit
```

Con phải kiểm tra kết quả RESET và ENABLE; nếu thất bại,
không báo testcase đã được thu thập coverage thành công.

Khi exit, kernel ngừng ghi trước khi công bố ZOMBIE theo mục 7.
Không bắt buộc con tự gọi DISABLE trước exit.

Tiến trình cha:

```text
tạo con và lưu PID
chuẩn bị vùng nhận 4112 byte

lặp có giới hạn thời gian:
    gọi cov_read(PID_con, vùng_nhận, 4112)
    nếu thành công: lưu kết quả và thoát vòng lặp
    nếu -2: chờ ngắn rồi thử lại
    nếu lỗi khác: ghi nhận lỗi và thoát vòng lặp

xử lý timeout nếu có
wait để thu hồi con khi con đã kết thúc
```

- Với -4, ghi nhận không có dữ liệu coverage; không lặp vô hạn.
- Với timeout, executor phải có quy trình kill và thu hồi con.
- Không bảo đảm đã có coverage nếu kernel panic.
- Không gọi wait trước khi hoàn tất đọc hoặc quyết định bỏ kết quả.

### 8.5. Giao diện cần Cường xác nhận

- Executor có thể đọc coverage trước khi gọi wait.
- Mỗi executor chạy một con testcase tại một thời điểm.
- Chấp nhận vùng nhận tối đa 4112 byte và các mã trả về dự kiến.
- Coverage của tiến trình cháu chưa được tổng hợp.
- Log gắn dữ liệu với testcase ID, seed và phiên bản kernel.
- Các kết quả chưa được đọc phải được phân biệt với coverage rỗng.

## 9. Overhead và kế hoạch đánh giá

### 9.1. Chi phí bộ nhớ dự kiến

Mỗi tiến trình đã cấp buffer sử dụng:

- Một trang dữ liệu 4096 byte.
- Các trường quản lý bổ sung trong struct proc.
- Vùng tạm khi đọc dữ liệu, được giải phóng sau khi đọc.

Nếu có N buffer tồn tại, riêng phần lưu mã điểm chiếm
N * 4096 byte. Buffer của tiến trình ZOMBIE vẫn chiếm bộ nhớ
cho đến khi tiến trình được thu hồi.

Kích thước chính xác của phần quản lý phải kiểm tra sau khi
triển khai do có thể có padding và dữ liệu đồng bộ.

### 9.2. Chi phí thời gian dự kiến

Mỗi điểm ghi thực hiện kiểm tra trạng thái, kiểm tra sức chứa
và ghi một mã điểm. Không cấp phát hoặc in log tại từng điểm.

Ngay cả khi tắt thu thập, các điểm được gắn vẫn có thể gây
chi phí kiểm tra trạng thái. Cần đo riêng ảnh hưởng này.

### 9.3. Kế hoạch đo

So sánh ba cấu hình:

1. Kernel chưa tích hợp coverage.
2. Kernel có coverage nhưng tắt thu thập.
3. Kernel có coverage và bật thu thập.

Giữ cùng toolchain, QEMU, CPU, RAM và workload.
Dùng workload cố định, đủ dài; reset trước mỗi lần đo,
ghi nhận overflow và lặp nhiều lần.

Ghi thời gian thực thi workload riêng với thời gian reset,
đọc dữ liệu và ghi log. Không dùng thời lượng cả phiên QEMU
làm thời gian thực thi workload.

Overhead phần trăm:

(T_moi - T_baseline) / T_baseline * 100%

Các giá trị thời gian và overhead chưa được đo trong tuần 2.
Coverage mới tăng không đồng nghĩa với đã tìm thấy bug.

## 10. Các file dự kiến bổ sung hoặc sửa

| File | Thay đổi dự kiến |
|---|---|
| kernel/coverage.h | Hằng số, cấu trúc và giao diện nội bộ |
| kernel/coverage.c | Quản lý buffer, ghi điểm và đọc dữ liệu |
| kernel/proc.h | Trạng thái coverage của tiến trình |
| kernel/proc.c | Khởi tạo, giữ dữ liệu khi exit và giải phóng |
| kernel/syscall.h | Cấp số hiệu syscall sau khi nhóm thống nhất |
| kernel/syscall.c | Đăng ký API và gắn điểm dispatch |
| kernel/sysproc.c | Hàm xử lý syscall điều khiển và đọc coverage |
| kernel/sysfile.c | Điểm coverage tại sys_read |
| kernel/file.c | Điểm coverage trong fileread |
| kernel/defs.h | Khai báo hàm nếu cần theo tổ chức mã nguồn |
| user/user.h | Khai báo API phía user |
| user/usys.pl | Sinh stub cho các syscall coverage |
| user/coverage_test.c | Chương trình kiểm thử API |
| Makefile | Thêm module và chương trình kiểm thử |

Chưa dự kiến sửa exec.c nếu trạng thái coverage được giữ
trong struct proc và không bị đường exec hiện tại thay đổi.

Danh sách trên cần đối chiếu khi triển khai. Không chỉnh trực tiếp
file user/usys.S được sinh tự động.

Các file dùng chung như proc.c, syscall.c, syscall.h, defs.h
và Makefile cần được phân công trước khi tích hợp để giảm
xung đột với fuzzer và sanitizer.

## 11. Bảng mã điểm dự kiến

| Mã | Vị trí | Ý nghĩa |
|---|---|---|
| 1 | syscall | Phân phối syscall hợp lệ, trừ API coverage |
| 2 | syscall | Số hiệu syscall không hợp lệ |
| 3 | sys_read | Descriptor không hợp lệ |
| 4 | sys_read | Chuyển tiếp đến fileread |
| 5 | fileread | Từ chối vì không readable hoặc n âm |
| 6 | fileread | Chọn đường đọc pipe |
| 7 | fileread | Chọn đường đọc thiết bị |
| 8 | fileread | Chọn đường đọc inode |
| 9 | fileread | Từ chối thiết bị không có hàm đọc hợp lệ |

Đây là các điểm có thể gắn theo mã nguồn đã khảo sát.
Điểm chọn đường xử lý không đồng nghĩa với thao tác thành công.

Mã điểm cần duy nhất trong phạm vi phiên bản bảng ánh xạ.
Khi thêm điểm ở các module khác, phải cập nhật bảng này.
Khi so sánh coverage, phải ghi phiên bản kernel và bảng mã điểm.

## 12. Kế hoạch kiểm thử prototype

| ID | Tình huống | Kết quả mong đợi |
|---|---|---|
| CV01 | Chưa bật coverage | Không ghi điểm |
| CV02 | Bật và chạy syscall đã gắn điểm | Có mã điểm tương ứng |
| CV03 | Tắt rồi tiếp tục chạy syscall | Buffer không tăng |
| CV04 | Reset khi đã tắt | count và overflow về 0 |
| CV05 | Reset khi đang bật | Trả -2, giữ dữ liệu |
| CV06 | Sinh hơn 1024 sự kiện | count không quá 1024, overflow bằng 1 |
| CV07 | Đọc với vùng nhận thiếu | Trả -3, giữ dữ liệu |
| CV08 | Đọc vào địa chỉ không hợp lệ | Trả lỗi, không panic |
| CV09 | Cha đọc con còn chạy | Trả -2 |
| CV10 | Cha đọc con ZOMBIE có buffer | Đọc được trước wait |
| CV11 | Đọc con sau khi đã wait thu hồi | Trả lỗi |
| CV12 | Đọc tiến trình không có quyền | Trả lỗi |
| CV13 | Đọc tiến trình chưa có buffer | Trả -4 |
| CV14 | Fork từ tiến trình đang thu thập | Con không kế thừa buffer |
| CV15 | Exec thành công | Giữ dữ liệu và trạng thái coverage |
| CV16 | Lặp tạo, chạy và thu hồi con | Không rò buffer, không dùng con trỏ cũ |
| CV17 | Gọi các API coverage | Không tự ghi điểm coverage |
| CV18 | Chạy lại usertests | Vẫn ALL TESTS PASSED |
| CV19 | Chạy grind | Ghi thời gian quan sát, lỗi nếu có |
| CV20 | Cấp phát buffer thất bại | Trả lỗi, coverage giữ trạng thái tắt |

Cần bổ sung kiểm thử CPUS=1 và CPUS=4 để đánh giá đồng bộ.
Các tình huống cần gây lỗi cấp phát hoặc trạng thái đặc biệt
phải có cơ chế kiểm thử riêng.

Chưa đánh dấu PASS cho bất kỳ testcase coverage nào vì
prototype chưa được triển khai.

## 13. Trạng thái thiết kế

Đã soạn đề xuất định nghĩa coverage, buffer, vòng đời,
API và kế hoạch kiểm thử dựa trên source baseline.

Chưa hoàn tất các nội dung sau:

- Nhóm xác nhận API và trình tự executor đọc trước wait.
- Nhóm xác nhận commit xv6 dùng để tích hợp.
- Chốt quy tắc đồng bộ và cách sao chép snapshot khi triển khai.
- Đối chiếu phạm vi sửa file với module sanitizer.
- Triển khai và chạy kiểm thử thực tế.
