#include <WebServer.h>
#include <WiFi.h>

// Pin definitions for TMC2209 and optical sensor (see config.h)
#define PIN_MOTOR_STEP 18
#define PIN_MOTOR_DIR 19
#define PIN_MOTOR_EN 23
#define PIN_U_SENSOR 32

// Sensor debounce parameters
#define SENSOR_DEBOUNCE_US 15000 // 15ms
#define WAIT_DELAY_MS 2000       // Pause duration between auto cycles (2 seconds)

// WiFi credentials
const char *ssid = "271271";
const char *password = "meomeomeo";

WebServer server(80);

// Motor state and hole counter
bool isRunning = false;
int speedDelay = 400; // Step delay in microseconds
int holeCount = 0;
int targetHoles = 0;

// Auto loop mode state
bool isAutoLoop = false;
bool isWaiting = false;
unsigned long waitStartTime = 0;

// Static HTML web page (using raw string literal)
const char *htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>TMC2209 Web Test & Sensor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f4f4f9; color: #333; }
    h1 { color: #2c3e50; font-size: 24px; }
    .control-panel { background: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; max-width: 90%; }
    .btn { background-color: #3498db; color: white; padding: 12px 20px; font-size: 16px; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; margin: 8px; transition: 0.3s; width: 140px; }
    .btn-stop { background-color: #e74c3c; }
    .btn-auto { background-color: #9b59b6; width: 100%; margin: 15px 0; padding: 15px; font-size: 18px; }
    .btn:hover { opacity: 0.8; transform: scale(1.05); }
    .btn:active { transform: scale(0.95); }
    #status { margin-top: 20px; font-weight: bold; color: #7f8c8d; }
  </style>
</head>
<body>
  <h1>Stepper Motor & Sprocket Sensor Test</h1>
  <div class="control-panel">
    <button class="btn btn-auto" onclick="sendCommand('/run2holes')">🎞 Auto Run (2 Holes - Pause 2s)</button>
    <br>
    <button class="btn" onclick="sendCommand('/forward')">⏩ Forward</button>
    <button class="btn btn-stop" onclick="sendCommand('/stop')">⏹ Stop</button>
    <button class="btn" onclick="sendCommand('/backward')">⏪ Backward</button>
    <div id="status">Status: Stopped</div>
  </div>

  <script>
    function sendCommand(cmd) {
      fetch(cmd)
        .then(response => response.text())
        .then(state => {
          document.getElementById('status').innerText = 'Status: ' + state;
        })
        .catch(err => {
          alert("Connection error to ESP32!");
        });
    }
    
    // Auto-refresh status every 2 seconds
    setInterval(function() {
      fetch('/status')
        .then(response => response.text())
        .then(state => {
          document.getElementById('status').innerText = 'Status: ' + state;
        });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", htmlPage); }

void handleForward() {
  targetHoles = 0; 
  isAutoLoop = false;
  isWaiting = false;
  isRunning = true;
  digitalWrite(PIN_MOTOR_DIR, LOW); // Forward direction
  digitalWrite(PIN_MOTOR_EN, LOW);
  server.send(200, "text/plain", "Running forward freely...");
}

void handleBackward() {
  targetHoles = 0;
  isAutoLoop = false;
  isWaiting = false;
  isRunning = true;
  digitalWrite(PIN_MOTOR_DIR, HIGH);
  digitalWrite(PIN_MOTOR_EN, LOW);
  server.send(200, "text/plain", "Running backward freely...");
}

void handleStop() {
  isRunning = false;
  isAutoLoop = false;
  isWaiting = false;
  targetHoles = 0;
  digitalWrite(PIN_MOTOR_EN, HIGH);
  server.send(200, "text/plain", "Stopped");
}

void handleRun2Holes() {
  holeCount = 0;
  targetHoles = 2; // Target: 2 sprocket holes
  isAutoLoop = true;
  isWaiting = false;
  isRunning = true;
  digitalWrite(PIN_MOTOR_DIR, LOW); // Chạy tới
  digitalWrite(PIN_MOTOR_EN, LOW);
  server.send(200, "text/plain", "Auto running: 2 holes...");
}

void handleStatus() {
  if (isWaiting) {
    server.send(200, "text/plain", "Pausing for 2 seconds...");
  } else if (isRunning && isAutoLoop) {
    server.send(200, "text/plain", "Counting holes (" + String(holeCount) + "/" + String(targetHoles) + ")");
  } else if (isRunning) {
    server.send(200, "text/plain", "Running freely");
  } else {
    server.send(200, "text/plain", "Stopped");
  }
}

void setup() {
  Serial.begin(115200);

  // Configure motor GPIO pins
  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_EN, OUTPUT);

  // Configure sensor pin
  pinMode(PIN_U_SENSOR, INPUT_PULLUP);

  // Disable motor on startup
  digitalWrite(PIN_MOTOR_EN, HIGH);
  digitalWrite(PIN_MOTOR_STEP, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Open this IP in your browser: http://");
  Serial.println(WiFi.localIP());

  // Register URL routes for web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/forward", HTTP_GET, handleForward);
  server.on("/backward", HTTP_GET, handleBackward);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/run2holes", HTTP_GET, handleRun2Holes);
  server.on("/status", HTTP_GET, handleStatus);

  // Start web server
  server.begin();
  Serial.println("Web server ready.");
}

void loop() {
  // 1. Handle incoming web requests
  server.handleClient();

  // 2. Handle 2-second wait timer (only in waiting state)
  if (isWaiting) {
    if (millis() - waitStartTime >= WAIT_DELAY_MS) {
      isWaiting = false;
      holeCount = 0; // Reset hole counter
      isRunning = true;
      digitalWrite(PIN_MOTOR_EN, LOW); // Re-enable motor
      Serial.println(">>> Resuming next 2-hole cycle... <<<");
    }
  }

  // 3. Sprocket hole counting with debounce
  static int sensorState = HIGH;
  static int lastReading = HIGH;
  static unsigned long lastDebounceTime = 0;
  
  int reading = digitalRead(PIN_U_SENSOR);
  if (reading != lastReading) {
    lastDebounceTime = micros();
  }

  if ((micros() - lastDebounceTime) > SENSOR_DEBOUNCE_US) {
    if (reading != sensorState) {
      sensorState = reading;
      
      // Trigger on HIGH-to-LOW transition (hole detected)
      if (sensorState == LOW) {
        if (isRunning && targetHoles > 0 && !isWaiting) {
          holeCount++;
          Serial.print("Phát hiện lỗ phim thứ: ");
          Serial.println(holeCount);
          
          if (holeCount >= targetHoles) {
            // Đã đủ số lỗ yêu cầu -> Dừng động cơ
            isRunning = false;
            digitalWrite(PIN_MOTOR_EN, HIGH);
            
            if (isAutoLoop) {
              isWaiting = true;
              waitStartTime = millis();
              Serial.println(">>> Đã đếm đủ 2 lỗ. Tạm dừng 2 giây... <<<");
            } else {
              targetHoles = 0;
              Serial.println(">>> Đã đếm đủ 2 lỗ. Dừng động cơ hoàn toàn! <<<");
            }
          }
        }
      }
    }
  }
  lastReading = reading;

  // 4. Non-blocking motor step generation
  static unsigned long lastStepTime = 0;
  if (isRunning && !isWaiting) {
    if (micros() - lastStepTime >= speedDelay) {
      lastStepTime = micros();

      // Generate a short STEP pulse
      digitalWrite(PIN_MOTOR_STEP, HIGH);
      delayMicroseconds(2); // TMC2209 needs pulse >100ns; 2us is safe
      digitalWrite(PIN_MOTOR_STEP, LOW);
    }
  }
}


