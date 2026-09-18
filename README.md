# ESP32 135 Autocarrier - Hệ thống quét Film 35mm tự động

Dự án mã nguồn mở điều khiển khay vận chuyển và định vị khung hình Film 35mm (135) tự động tốc độ cao cho việc quét ảnh bằng máy ảnh số (Camera Scanning / Digitization), tích hợp **Tay cầm điều khiển (Hand Control Panel)** với cảm giác cơ học cao cấp tương tự **Bobach 135 Autocarrier**.

---

## 1. Thiết kế Tay cầm điều khiển (Hand Control Panel) - Giao tiếp I2C

Tay cầm điều khiển được thiết kế với 1 công tắc, 2 núm vặn và 1 nút chụp lớn, giao tiếp với hệ thống chính qua module **PCF8574T (I2C)** để giảm thiểu số lượng cáp kết nối (chỉ cần 4 dây nguồn/tín hiệu và 1 dây ngắt INT).

```text
+-------------------------------------------------------------+
|               135 AUTOCARRIER CONTROL PANEL                 |
|                                                             |
|   +-------------------+              +------------------+   |
|   |   OLED DISPLAY    |              |   ( O ) SHUTTER  |   |
|   |  [ 128x64 I2C ]   |              |  Nút chụp lớn    |   |
|   |                   |              |                  |   |
|   +-------------------+              +------------------+   |
|                                                             |
|   [  AUTO - SEMI - MANUAL  ]                                |
|   Công tắc gạt 3 vị trí MTS-103                             |
|                                                             |
|           /---------\                         /---------\   |
|          |     O     |                       |     O     |  |
|           \---------/                         \---------/   |
|           NÚM JOG VỊ TRÍ                    NÚM CHỈNH TỐC ĐỘ|
|     Xoay để kéo tiến/lùi film         Xoay để thay đổi Speed|
+-------------------------------------------------------------+
```

### 3 Chế độ vận hành (Operation Modes):
1. **`AUTO` (Quét tự động cả cuộn):**
   * Bấm nút núm xoay hoặc nút Shutter $\rightarrow$ Máy tự động chụp, đếm đúng 8 lỗ sprocket để kéo sang frame tiếp theo, lặp lại cho đến hết 36 tấm hoặc hết film.
2. **`SEMI` (Chế độ Bán tự động - Quét từng tấm an toàn):**
   * Rất hữu ích cho film slide E-6 hoặc cuộn film có đoạn chụp đè/lệch khung: Bạn căn chỉnh khung hình đầu tiên $\rightarrow$ Bấm nút Shutter trên tay cầm $\rightarrow$ Máy kích chụp $\rightarrow$ Tự kéo đúng 1 frame (8 lỗ) $\rightarrow$ Dừng chờ bạn bấm chụp tấm tiếp theo.
3. **`MANUAL` (Chỉnh tay hoàn toàn bằng Núm xoay Hand Wheel):**
   * Vặn núm xoay theo chiều kim đồng hồ $\rightarrow$ Động cơ kéo film tiến vào từng bước nhỏ theo tay vặn.
   * Vặn ngược chiều kim đồng hồ $\rightarrow$ Động cơ trả film lùi lại (Rewind).
   * Bấm nút Shutter trên tay cầm $\rightarrow$ Máy ảnh chụp tấm đang căn.

---

## 2. Sơ đồ kết nối phần cứng chi tiết

### Bảng kết nối ESP32 với Khay film và Tay điều khiển (I2C PCF8574T):

**1. Kết nối ESP32 với Khay Film / OLED / PCF8574T:**
| Chân ESP32 | Thiết bị kết nối | Chân trên thiết bị | Chức năng |
| :--- | :--- | :--- | :--- |
| **GPIO 18** | Driver Stepper (A4988/TMC2209) | **STEP** | Xung bước động cơ NEMA 42 |
| **GPIO 19** | Driver Stepper | **DIR** | Chiều quay motor |
| **GPIO 23** | Driver Stepper | **EN** / **ENABLE** | Bật / Tắt dòng motor |
| **GPIO 25** | Optocoupler PC817 | **Chân 1 (Anode)** | Kích Shutter máy ảnh |
| **GPIO 32** | Cảm biến chữ U | **OUT** | Đếm lỗ sprocket |
| **GPIO 21** | OLED & PCF8574T | **SDA** | Dữ liệu I2C |
| **GPIO 22** | OLED & PCF8574T | **SCL** | Xung I2C |
| **GPIO 34** | Module PCF8574T | **INT** | Nhận tín hiệu ngắt từ tay cầm |
| **3.3V / 5V**| Các Module | **VCC** | Cấp nguồn mạch |
| **GND** | Các Module | **GND** | Mass toàn hệ thống |

**2. Kết nối mạch PCF8574T với các linh kiện trong Tay Cầm:**
*(Lưu ý: Tất cả các nút/công tắc đều nối 1 chân vào PCF8574T, chân còn lại nối **GND**)*
| Chân PCF8574T | Thiết bị trên Tay Cầm | Chức năng |
| :--- | :--- | :--- |
| **P0** | Núm JOG Vị trí (KY-040 #1) | Chân **CLK** |
| **P1** | Núm JOG Vị trí (KY-040 #1) | Chân **DT** |
| **P2** | Núm Chỉnh Tốc Độ (KY-040 #2) | Chân **CLK** |
| **P3** | Núm Chỉnh Tốc Độ (KY-040 #2) | Chân **DT** |
| **P4** | Công tắc MTS-103 (Chân bìa 1) | Chọn chế độ **AUTO** |
| **P5** | Công tắc MTS-103 (Chân bìa 2) | Chọn chế độ **MANUAL** |
| *(GND)*| Công tắc MTS-103 (Chân giữa) | *(Nếu không gạt P4 hay P5, mạch tự hiểu là **SEMI**)* |
| **P6** | Nút bấm Shutter lớn | Bấm chụp (trong chế độ MANUAL) |
| **P7** | Nút bấm nhỏ (Start/Stop) | Bắt đầu / Tạm dừng (trong AUTO/SEMI) |

---

## 3. Mạch Kích Shutter máy ảnh (Optocoupler PC817)

```
ESP32 GPIO 25 ---> [ Điện trở 220Ω ] ---> Chân 1 (Anode) PC817
ESP32 GND     -------------------------> Chân 2 (Cathode) PC817

Dây Jack máy ảnh (2.5mm / 3.5mm):
Tip (Shutter) + Ring (Focus)  ---------> Chân 4 (Collector) PC817
Sleeve (GND / Mass máy ảnh)   ---------> Chân 3 (Emitter) PC817
```

---

## 4. Cách nạp code vào ESP32

1. Mở file [**`esp32_135_autocarrier.ino`**](file:///C:/Users/LENOVO/.gemini/antigravity-ide/scratch/esp32_135_autocarrier/esp32_135_autocarrier.ino) trong **Arduino IDE**.
2. Cài 2 thư viện trong *Library Manager*: `Adafruit SH110X` & `Adafruit GFX Library`.
3. Chọn board **ESP32 Dev Module** và cổng COM $\rightarrow$ Nhấn **Upload**.

---

## 5. Điều khiển qua USB Serial Monitor (Baudrate: 115200)

Bạn có thể cắm cáp USB vào máy tính và gõ lệnh:
* `start` : Bắt đầu quét tự động (AUTO).
* `stop` : Dừng khẩn cấp.
* `mode auto` / `mode semi` / `mode manual` : Đổi chế độ vận hành.
* `full` / `half` : Đổi chuẩn Full-Frame (8 lỗ) hoặc Half-Frame (4 lỗ).
* `reset` : Reset số đếm về 0.

---

## 6. Công cụ Test Phần Cứng qua Web WiFi (Servo + Stepper + Shutter + Sensor)

Chương trình test độc lập toàn diện: [**`web_test_servo_step.ino`**](file:///c:/Users/LENOVO/.gemini/antigravity-ide/scratch/esp32_135_autocarrier/web_test_servo_step.ino)
* **Kết nối WiFi**: `ESP32_Autocarrier_Test` | Mật khẩu: `12345678`
* **Truy cập Trình duyệt Web**: [http://192.168.4.1](http://192.168.4.1)
* **Tính năng Web UI**:
  * Thanh trượt góc Servo 0° - 180° & chế độ Auto Sweep.
  * Điều khiển Stepper Motor (Jogging từng bước, quay liên tục, chỉnh tốc độ µs, Enable/Disable driver).
  * Kiểm tra cảm biến quang chữ U & Kích thử Shutter máy ảnh qua Opto PC817.

