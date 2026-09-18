/*
 * ==============================================================================
 * DỰ ÁN: ESP32 135 AUTOCARRIER - WEB TMC2209 & SERVO TEST BENCH
 * ==============================================================================
 * Chức năng: Điều khiển trực tiếp Driver TMC2209 (chế độ StealthChop siêu êm)
 *            và RC Servo qua giao diện Web WiFi.
 * 
 * SƠ ĐỒ ĐẤU NỐI (PINOUT):
 *  - TMC2209 STEP:     GPIO 18 (Xung bước tần số cao)
 *  - TMC2209 DIR:      GPIO 19 (Chiều quay: HIGH = Tiến / LOW = Lùi)
 *  - TMC2209 EN:       GPIO 23 (Active LOW: LOW = Bật giữ cốt / HIGH = Tắt thả lỏng)
 *  - RC SERVO (Opt):   GPIO 4  (Tín hiệu PWM 50Hz)
 *  - NGUỒN:
 *      + TMC2209 VIO:  3.3V của ESP32
 *      + TMC2209 GND:  GND chung
 *      + TMC2209 VMOT: Nguồn ngoài 12V / 24V cho động cơ NEMA
 * 
 * KẾT NỐI WIFI:
 *  - Mạng WiFi AP do ESP32 phát:
 *      + Tên WiFi (SSID):  ESP32_Autocarrier_Test 
 *      + Mật khẩu:         12345678
 *      + Địa chỉ Web:      http://192.168.4.1
 *  - Hoặc kết nối qua WiFi phòng/xưởng (STA_SSID đã cấu hình bên dưới).
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "web_page.h"

// ================================================================
// 1. CẤU HÌNH CHÂN GPIO CHO DRIVER TMC2209 & SERVO
// ================================================================
#define PIN_MOTOR_STEP        18   // Chân STEP driver TMC2209
#define PIN_MOTOR_DIR         19   // Chân DIR driver TMC2209
#define PIN_MOTOR_EN          23   // Chân EN driver TMC2209 (Active LOW)
#define PIN_SERVO             4    // Chân điều khiển RC Servo (PWM)
#define PIN_U_SENSOR          14   // Chân OUT cảm biến chữ U (Ngắt Interrupt)
#define PIN_I2C_SDA           21   // I2C SDA cho VL53L0X / OLED
#define PIN_I2C_SCL           22   // I2C SCL cho VL53L0X / OLED
// Cấu hình WiFi
const char* AP_SSID = "ESP32_Autocarrier_Test";
const char* AP_PASS = "12345678";
const char* STA_SSID = "271271";
const char* STA_PASS = "meomeomeo";

// ================================================================
// 2. BIẾN TOÀN CỤC & KHỞI TẠO ĐỐI TƯỢNG
// ================================================================
WebServer server(80);
Servo testServo;

// Trạng thái Driver TMC2209
bool motorEnabled = false;
bool motorRunningContinuous = false;
bool motorDirForward = true;
int stepSpeedDelayUs = 350; // Tốc độ delay xung bước (350us ~ cực êm trên TMC2209)
volatile long targetStepsRemaining = 0;
unsigned long lastStepMicros = 0;

volatile int uSensorCount = 0;
unsigned long lastSensorTime = 0;

// Trạng thái Servo
int currentServoAngle = 90;

// Trạng thái VL53L0X (Chạy ở Core 0)
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
bool loxConnected = false;
volatile int vl53Distance = -1;
TaskHandle_t SensorTask;

unsigned long lastIpPrintMillis = 0;

// ================================================================
// 3. CÁC HÀM XỬ LÝ REST API WEBSERVER
// ================================================================

void IRAM_ATTR uSensorISR() {
  unsigned long now = micros();
  if (now - lastSensorTime > 15000) { // 15ms debounce chống nhiễu
    uSensorCount++;
    lastSensorTime = now;
  }
}

// Hàm chạy Độc lập trên Core 0 để không làm lag nhịp Motor (Core 1)
void sensorTaskCode(void * parameter) {
  int errorCount = 0;
  Serial.println("[VL53L0X] Task cảm biến đã bắt đầu trên Core 1");
  
  for (;;) {
    if (loxConnected) {
      VL53L0X_RangingMeasurementData_t measure;
      memset(&measure, 0, sizeof(measure)); 
      measure.RangeStatus = 255; // Đặt giá trị rác để kiểm tra xem hàm có thực sự ghi đè không
      
      // Không phụ thuộc vào true/false của hàm này vì thư viện Adafruit hay trả về false nếu có warning (dù data vẫn đúng)
      lox.rangingTest(&measure, false);
      
      if (measure.RangeStatus != 255) {
        errorCount = 0; 
        if (measure.RangeStatus != 4) { // 4 = Out of range
          vl53Distance = measure.RangeMilliMeter;
          Serial.printf("[VL53L0X] Khoảng cách: %d mm\n", vl53Distance);
        } else {
          vl53Distance = -1;
          Serial.println("[VL53L0X] Out of range (Không có vật cản)");
        }
      } else {
        errorCount++;
        Serial.println("[VL53L0X] LỖI GIAO TIẾP I2C (Không có dữ liệu trả về)!");
      }
      
      if (errorCount > 10) { 
        Serial.println("[VL53L0X] Mất kết nối I2C quá nhiều, thử reset...");
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
        lox.begin(); 
        errorCount = 0;
        vTaskDelay(pdMS_TO_TICKS(500)); 
      }
    } else {
      Serial.println("[VL53L0X] Đang thử khởi tạo lại cảm biến...");
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
      if (lox.begin()) {
        Serial.println("[VL53L0X] KHỞI TẠO THÀNH CÔNG!");
        loxConnected = true;
      }
    }
    // Thời gian chờ 100ms
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  String json = "{";
  json += "\"motorEnabled\":" + String(motorEnabled ? "true" : "false") + ",";
  json += "\"motorRunning\":" + String(motorRunningContinuous ? "true" : "false") + ",";
  json += "\"dirForward\":" + String(motorDirForward ? "true" : "false") + ",";
  json += "\"speedDelayUs\":" + String(stepSpeedDelayUs) + ",";
  json += "\"servoAngle\":" + String(currentServoAngle) + ",";
  json += "\"uSensorValue\":" + String(digitalRead(PIN_U_SENSOR)) + ",";
  json += "\"uSensorCount\":" + String(uSensorCount) + ",";
  json += "\"vl53Distance\":" + String(vl53Distance);
  json += "}";
  server.send(200, "application/json", json);
}

void handleStepperEnable() {
  if (server.hasArg("state")) {
    motorEnabled = (server.arg("state").toInt() == 1);
    digitalWrite(PIN_MOTOR_EN, motorEnabled ? LOW : HIGH); // LOW = ENABLED (Khóa cứng cốt)
    if (!motorEnabled) {
      motorRunningContinuous = false;
      targetStepsRemaining = 0;
    }
    Serial.printf("[TMC2209] Enable: %s (GPIO 23 = %s)\n", 
      motorEnabled ? "ON (Lock)" : "OFF (Free)", motorEnabled ? "LOW" : "HIGH");
  }
  server.send(200, "application/json", "{\"enabled\":" + String(motorEnabled ? "true" : "false") + "}");
}

void handleStepperDir() {
  if (server.hasArg("fwd")) {
    motorDirForward = (server.arg("fwd").toInt() == 1);
    digitalWrite(PIN_MOTOR_DIR, motorDirForward ? HIGH : LOW);
    Serial.printf("[TMC2209] Direction: %s (GPIO 19 = %s)\n", 
      motorDirForward ? "TIEN" : "LUI", motorDirForward ? "HIGH" : "LOW");
  }
  server.send(200, "application/json", "{\"dirForward\":" + String(motorDirForward ? "true" : "false") + "}");
}

void handleStepperRun() {
  if (server.hasArg("state")) {
    motorRunningContinuous = (server.arg("state").toInt() == 1);
    if (motorRunningContinuous && !motorEnabled) {
      motorEnabled = true;
      digitalWrite(PIN_MOTOR_EN, LOW); // Tự động bật Enable
    }
    Serial.printf("[TMC2209] Continuous Run: %s\n", motorRunningContinuous ? "START" : "STOP");
  }
  server.send(200, "application/json", "{\"running\":" + String(motorRunningContinuous ? "true" : "false") + "}");
}

void handleStepperStop() {
  motorRunningContinuous = false;
  targetStepsRemaining = 0;
  Serial.println("[TMC2209] STOP ALL");
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

void handleStepperJog() {
  int fwd = server.hasArg("fwd") ? server.arg("fwd").toInt() : 1;
  long steps = server.hasArg("steps") ? server.arg("steps").toInt() : 200;

  motorDirForward = (fwd == 1);
  digitalWrite(PIN_MOTOR_DIR, motorDirForward ? HIGH : LOW);

  // Tự động bật Enable nếu đang tắt
  if (!motorEnabled) {
    motorEnabled = true;
    digitalWrite(PIN_MOTOR_EN, LOW);
  }

  targetStepsRemaining += steps;
  Serial.printf("[TMC2209] Jog: Dir=%s, Steps=%ld\n", motorDirForward ? "TIEN" : "LUI", steps);
  server.send(200, "application/json", "{\"status\":\"ok\",\"steps\":" + String(steps) + "}");
}

void handleStepperSpeed() {
  if (server.hasArg("delay")) {
    int d = server.arg("delay").toInt();
    if (d >= 50 && d <= 3000) {
      stepSpeedDelayUs = d;
      Serial.printf("[TMC2209] Speed Delay: %d us\n", stepSpeedDelayUs);
    }
  }
  server.send(200, "application/json", "{\"speedDelayUs\":" + String(stepSpeedDelayUs) + "}");
}

void handleServoAngle() {
  if (server.hasArg("angle")) {
    int angle = server.arg("angle").toInt();
    if (angle >= 0 && angle <= 180) {
      currentServoAngle = angle;
      testServo.write(currentServoAngle);
      Serial.printf("[SERVO] Set Angle: %d deg\n", currentServoAngle);
    }
  }
  server.send(200, "application/json", "{\"angle\":" + String(currentServoAngle) + "}");
}

void handleSensorReset() {
  uSensorCount = 0;
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ================================================================
// 4. SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println("\n=======================================================");
  Serial.println("     ESP32 + TMC2209 STEPPER & SERVO WEB BENCH         ");
  Serial.println("=======================================================");

  // 1. Cấu hình chân Động cơ bước TMC2209
  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_EN, OUTPUT);
  digitalWrite(PIN_MOTOR_STEP, LOW);
  digitalWrite(PIN_MOTOR_DIR, HIGH);
  digitalWrite(PIN_MOTOR_EN, HIGH); // Mặc định Disable để driver và motor mát

  // 2. Khởi tạo Servo ở GPIO 4
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  testServo.setPeriodHertz(50);
  testServo.attach(PIN_SERVO, 500, 2400);
  testServo.write(currentServoAngle);
  Serial.println("[OK] RC Servo sẵn sàng ở GPIO 4.");

  // 3. Khởi tạo Cảm biến chữ U ở GPIO 14
  pinMode(PIN_U_SENSOR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_U_SENSOR), uSensorISR, FALLING);
  Serial.println("[OK] Cảm biến chữ U sẵn sàng ở GPIO 14.");

  // 4. Khởi tạo VL53L0X qua I2C
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (!lox.begin()) {
    Serial.println("[LỖI] Không tìm thấy VL53L0X. Vui lòng kiểm tra dây SDA=21, SCL=22.");
    loxConnected = false;
  } else {
    Serial.println("[OK] Cảm biến Laser VL53L0X đã sẵn sàng.");
    loxConnected = true;
  }
  
  // LUÔN LUÔN tạo task, dù cảm biến có lỗi thì nó sẽ tự thử lại
  xTaskCreatePinnedToCore(
    sensorTaskCode,   // Hàm thực thi
    "SensorTask",     // Tên task
    4000,             // Kích thước stack
    NULL,             // Tham số
    1,                // Mức ưu tiên
    &SensorTask,      // Task Handle
    1                 // Đổi sang chạy ở Core 1 để tránh lỗi I2C đa luồng (cross-core)
  );

  // 5. Cấu hình WiFi (Đã tắt theo yêu cầu)
  /*
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("\n--- THÔNG TIN KẾT NỐI WIFI ---");
  Serial.printf("WiFi AP (ESP32 phát): %s | Pass: %s\n", AP_SSID, AP_PASS);
  Serial.print("=> ĐỊA CHỈ IP (WIFI AP): http://");
  Serial.println(WiFi.softAPIP());

  // Kết nối WiFi gia đình nếu có
  if (strlen(STA_SSID) > 0) {
    Serial.printf("Đang kết nối vào WiFi: %s ...\n", STA_SSID);
    WiFi.begin(STA_SSID, STA_PASS);
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
      delay(300);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\n[OK] Đã kết nối WiFi thành công!\n");
      Serial.print("=> ĐỊA CHỈ IP (WIFI GIA ĐÌNH): http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n[!] Không kết nối được WiFi gia đình, bạn hãy kết nối trực tiếp vào WiFi của ESP32.");
    }
  }

  // 4. Đăng ký các Endpoint Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/stepper/enable", HTTP_GET, handleStepperEnable);
  server.on("/api/stepper/dir", HTTP_GET, handleStepperDir);
  server.on("/api/stepper/run", HTTP_GET, handleStepperRun);
  server.on("/api/stepper/stop", HTTP_GET, handleStepperStop);
  server.on("/api/stepper/jog", HTTP_GET, handleStepperJog);
  server.on("/api/stepper/speed", HTTP_GET, handleStepperSpeed);
  server.on("/api/servo", HTTP_GET, handleServoAngle);
  server.on("/api/sensor/reset", HTTP_GET, handleSensorReset);

  server.begin();
  Serial.println("[OK] Web Server đã hoạt động!");
  */
  Serial.println("=======================================================\n");
}

// ================================================================
// 5. VÒNG LẶP CHÍNH (HIGH-PRECISION TMC2209 STEPPING ENGINE)
// ================================================================
void loop() {
  // 1. Phục vụ Web Client Requests (Đã tắt)
  // server.handleClient();

  unsigned long currentMicros = micros();

  // 2. Tạo xung bước (STEP Pulses) trực tiếp siêu mượt cho TMC2209
  if (motorEnabled) {
    if (motorRunningContinuous || targetStepsRemaining > 0) {
      if (currentMicros - lastStepMicros >= (unsigned long)stepSpeedDelayUs) {
        lastStepMicros = currentMicros;

        // Xuất 1 xung STEP chuẩn cho TMC2209
        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(2); // Thời gian độ rộng xung tối thiểu của TMC2209
        digitalWrite(PIN_MOTOR_STEP, LOW);

        if (!motorRunningContinuous && targetStepsRemaining > 0) {
          targetStepsRemaining--;
        }
      }
    }
  } else {
    // Nếu motor không bật, thêm một khoảng nghỉ rất nhỏ để tránh lỗi Watchdog Timer (WDT) 
    // vì vòng lặp sẽ chạy quá nhanh
    vTaskDelay(1);
  }

  // 3. In trạng thái hoạt động mỗi 1 giây để biết ESP32 không bị treo
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 1000) {
    lastHeartbeat = millis();
    Serial.printf("[SYSTEM] Đang hoạt động... Khoảng cách hiện tại: %d mm\n", vl53Distance);
  }
  
  // (Đã tắt theo yêu cầu) In lại địa chỉ IP mỗi 5 giây
  /*
  if (millis() - lastIpPrintMillis > 5000) {
    lastIpPrintMillis = millis();
    Serial.println("\n-------------------------------------------------------");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[IP] => ĐỊA CHỈ WEB: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.print("[IP] => ĐỊA CHỈ WEB AP: http://");
      Serial.println(WiFi.softAPIP());
    }
    Serial.println("-------------------------------------------------------\n");
  }
  */
}
