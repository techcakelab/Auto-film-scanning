#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>ESP32 TMC2209 Stepper & Servo Web Bench</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700;800&family=JetBrains+Mono:wght@400;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #0b0f19;
      --card-bg: rgba(18, 26, 43, 0.9);
      --card-border: rgba(255, 255, 255, 0.08);
      --primary: #38bdf8;
      --primary-glow: rgba(56, 189, 248, 0.35);
      --accent: #f43f5e;
      --accent-glow: rgba(244, 63, 94, 0.35);
      --success: #10b981;
      --success-glow: rgba(16, 185, 129, 0.35);
      --warning: #f59e0b;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --radius: 16px;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
    }

    body {
      font-family: 'Outfit', sans-serif;
      background: radial-gradient(circle at 50% 0%, #1e293b 0%, var(--bg) 80%);
      color: var(--text);
      min-height: 100vh;
      padding: 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .container {
      width: 100%;
      max-width: 720px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    /* HEADER */
    header {
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 16px 20px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
    .brand-title {
      font-size: 1.25rem;
      font-weight: 800;
      background: linear-gradient(135deg, #38bdf8, #818cf8);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      letter-spacing: -0.5px;
    }
    .status-badge {
      font-family: 'JetBrains Mono', monospace;
      font-size: 0.75rem;
      font-weight: 600;
      padding: 4px 10px;
      border-radius: 20px;
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid var(--success);
      color: var(--success);
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .status-dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--success);
      box-shadow: 0 0 8px var(--success);
    }

    /* CARD */
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 20px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.4);
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    .card-title {
      font-size: 1.05rem;
      font-weight: 700;
      display: flex;
      align-items: center;
      justify-content: space-between;
      color: #e2e8f0;
      border-bottom: 1px solid rgba(255,255,255,0.06);
      padding-bottom: 10px;
    }

    /* BUTTONS */
    .btn {
      font-family: 'Outfit', sans-serif;
      font-weight: 600;
      font-size: 0.95rem;
      padding: 12px 14px;
      border-radius: 12px;
      border: 1px solid var(--card-border);
      background: #1e293b;
      color: var(--text);
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      transition: all 0.15s ease;
      box-shadow: 0 4px 10px rgba(0,0,0,0.3);
    }
    .btn:hover {
      background: #273549;
      transform: translateY(-2px);
    }
    .btn:active {
      transform: translateY(1px) scale(0.98);
    }

    .btn-primary {
      background: linear-gradient(135deg, #0284c7, #2563eb);
      border-color: rgba(56, 189, 248, 0.4);
      color: #ffffff;
    }
    .btn-primary:hover {
      background: linear-gradient(135deg, #0ea5e9, #3b82f6);
      box-shadow: 0 0 15px var(--primary-glow);
    }

    .btn-accent {
      background: linear-gradient(135deg, #e11d48, #be123c);
      border-color: rgba(244, 63, 94, 0.4);
      color: #ffffff;
    }
    .btn-accent:hover {
      background: linear-gradient(135deg, #f43f5e, #e11d48);
      box-shadow: 0 0 15px var(--accent-glow);
    }

    .btn-success {
      background: linear-gradient(135deg, #059669, #047857);
      border-color: rgba(16, 185, 129, 0.4);
      color: #ffffff;
    }
    .btn-success:hover {
      background: linear-gradient(135deg, #10b981, #059669);
      box-shadow: 0 0 15px var(--success-glow);
    }

    .btn-outline {
      background: rgba(255,255,255,0.03);
      border: 1px solid rgba(255,255,255,0.12);
    }
    .btn-outline:hover {
      background: rgba(255,255,255,0.08);
      border-color: var(--primary);
    }

    /* GRIDS */
    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    .grid-3 {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }
    .grid-6 {
      display: grid;
      grid-template-columns: repeat(6, 1fr);
      gap: 6px;
    }

    /* SLIDERS & INPUTS */
    .slider-box {
      width: 100%;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      height: 10px;
      border-radius: 5px;
      background: #1e293b;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: var(--primary);
      cursor: pointer;
      box-shadow: 0 0 10px var(--primary);
      border: 2px solid #ffffff;
    }

    .input-row {
      display: flex;
      gap: 8px;
    }
    input[type=number] {
      flex: 1;
      background: #0f172a;
      border: 1px solid rgba(255,255,255,0.15);
      border-radius: 10px;
      padding: 10px 14px;
      color: #fff;
      font-family: 'JetBrains Mono', monospace;
      font-size: 1rem;
      outline: none;
    }
    input[type=number]:focus {
      border-color: var(--primary);
    }

    .info-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.88rem;
      color: var(--text-muted);
    }

    /* CONSOLE */
    .console-box {
      font-family: 'JetBrains Mono', monospace;
      font-size: 0.78rem;
      background: #060911;
      border: 1px solid rgba(255,255,255,0.05);
      border-radius: 10px;
      padding: 10px 14px;
      color: #38bdf8;
      height: 110px;
      overflow-y: auto;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .console-item {
      opacity: 0.9;
    }
  </style>
</head>
<body>

<div class="container">
  <!-- HEADER -->
  <header>
    <div>
      <div class="brand-title">ESP32 + TMC2209 STEPPER BENCH</div>
      <div style="font-size: 0.8rem; color: var(--text-muted); margin-top:2px;">
        Chân kết nối: STEP=18 | DIR=19 | EN=23 | Servo=4
      </div>
    </div>
    <div class="status-badge">
      <div class="status-dot"></div>
      <span id="connStatus">ONLINE</span>
    </div>
  </header>

  <!-- 1. DRIVER POWER & CONTINUOUS RUN -->
  <div class="card">
    <div class="card-title">
      <span>1. Trạng Thái TMC2209 & Quay Liên Tục</span>
      <span id="driverStatusBadge" style="font-size: 0.8rem; font-weight:700; color:var(--accent);">DRIVER TẮT (FREE RUN)</span>
    </div>

    <div class="grid-2">
      <button id="btnEnable" class="btn btn-accent" onclick="toggleEnable()">
        ⚡ BẬT KHÓA TRỤC (EN = LOW)
      </button>
      <button id="btnDir" class="btn btn-outline" onclick="toggleDirection()">
        🔄 Chiều: TIẾN (FWD)
      </button>
    </div>

    <div class="grid-2">
      <button id="btnRun" class="btn btn-primary" style="font-size:1.05rem; padding:14px;" onclick="toggleRun()">
        ▶ BẮT ĐẦU QUAY LIÊN TỤC
      </button>
      <button class="btn btn-accent" style="font-size:1.05rem;" onclick="stopAll()">
        🛑 DỪNG KHẨN CẤP (STOP)
      </button>
    </div>
  </div>

  <!-- 2. JOGGING TỪNG BƯỚC -->
  <div class="card">
    <div class="card-title">
      <span>2. Điều Khiển Từng Bước (Micro-Jogging)</span>
    </div>

    <div class="info-row">
      <span>Bước vi dịch chuyển (Fine Steps):</span>
    </div>
    <div class="grid-6">
      <button class="btn btn-outline" onclick="jog(-10)">-10</button>
      <button class="btn btn-outline" onclick="jog(-50)">-50</button>
      <button class="btn btn-outline" onclick="jog(-200)">-200</button>
      <button class="btn btn-outline" onclick="jog(200)">+200</button>
      <button class="btn btn-outline" onclick="jog(50)">+50</button>
      <button class="btn btn-outline" onclick="jog(10)">+10</button>
    </div>

    <div class="info-row" style="margin-top: 4px;">
      <span>Theo góc / vòng (TMC2209 1/16 vi bước: 3200 bước = 1 vòng):</span>
    </div>
    <div class="grid-3">
      <button class="btn btn-outline" onclick="jog(-800)">◀ 1/4 Vòng (800b ~ 1 Frame)</button>
      <button class="btn btn-outline" onclick="jog(800)">1/4 Vòng (+800b) ▶</button>
      <button class="btn btn-outline" onclick="jog(3200)">1 Vòng Đầy (+3200b) ⏩</button>
    </div>

    <div class="info-row" style="margin-top: 4px;">
      <span>Nhập số bước tùy ý:</span>
    </div>
    <div class="input-row">
      <input type="number" id="customSteps" placeholder="Ví dụ: 800 bước" value="800">
      <button class="btn btn-outline" onclick="jogCustom(false)">◀ Lùi</button>
      <button class="btn btn-primary" onclick="jogCustom(true)">Tiến ▶</button>
    </div>
  </div>

  <!-- 3. TỐC ĐỘ (DELAY MICROSECONDS) -->
  <div class="card">
    <div class="card-title">
      <span>3. Tốc Độ Bước (StealthChop Speed)</span>
      <b id="speedLabel" style="color:var(--primary); font-family:'JetBrains Mono'; font-size:1.1rem;">350 µs</b>
    </div>

    <div class="slider-box">
      <input type="range" id="speedSlider" min="80" max="1500" step="20" value="350" oninput="onSpeedSlider(this.value)" onchange="sendSpeed(this.value)">
    </div>
    <div class="info-row" style="font-size:0.8rem;">
      <span>80 µs (Cực Nhanh)</span>
      <span>350 µs (Chuẩn Êm TMC2209)</span>
      <span>1500 µs (Chậm / Lực Kéo Cực Khỏe)</span>
    </div>
  </div>

  <!-- 4. RC SERVO (GPIO 4) -->
  <div class="card">
    <div class="card-title">
      <span>4. Test RC Servo (GPIO 4)</span>
      <b id="servoLabel" style="color:var(--primary); font-family:'JetBrains Mono'; font-size:1.1rem;">90°</b>
    </div>
    <div class="slider-box">
      <input type="range" id="servoSlider" min="0" max="180" value="90" oninput="document.getElementById('servoLabel').innerText = this.value + '°'" onchange="sendServo(this.value)">
    </div>
    <div class="grid-3">
      <button class="btn btn-outline" onclick="setServo(0)">0°</button>
      <button class="btn btn-outline" onclick="setServo(90)">90° (Giữa)</button>
      <button class="btn btn-outline" onclick="setServo(180)">180°</button>
    </div>
  </div>

  <!-- 5. U-SENSOR (GPIO 14) -->
  <div class="card">
    <div class="card-title">
      <span>5. Test Cảm Biến Chữ U (GPIO 14)</span>
      <button class="btn btn-outline" style="padding:4px 8px; font-size:0.75rem;" onclick="resetSensor()">Xóa Biến Đếm</button>
    </div>
    <div class="grid-2">
      <div style="background: rgba(0,0,0,0.3); border-radius: 8px; padding: 12px; text-align: center;">
        <div style="font-size: 0.8rem; color: var(--text-muted);">Tín Hiệu (PIN STATE)</div>
        <div id="sensorState" style="font-size: 1.5rem; font-weight: 700; color: var(--warning); font-family: 'JetBrains Mono';">--</div>
      </div>
      <div style="background: rgba(0,0,0,0.3); border-radius: 8px; padding: 12px; text-align: center;">
        <div style="font-size: 0.8rem; color: var(--text-muted);">Số Lần Bị Cản (Lỗ/Ngắt)</div>
        <div id="sensorCount" style="font-size: 1.5rem; font-weight: 700; color: var(--success); font-family: 'JetBrains Mono';">0</div>
      </div>
    </div>
  </div>

  <!-- 6. VL53L0X LASER SENSOR (I2C) -->
  <div class="card">
    <div class="card-title">
      <span>6. Cảm Biến Laser VL53L0X (I2C)</span>
    </div>
    <div style="background: rgba(0,0,0,0.3); border-radius: 8px; padding: 20px; text-align: center;">
      <div style="font-size: 0.9rem; color: var(--text-muted);">Khoảng cách đo được</div>
      <div id="laserDistance" style="font-size: 2.2rem; font-weight: 800; color: #818cf8; font-family: 'JetBrains Mono';">-- mm</div>
    </div>
  </div>

  <!-- 7. CONSOLE LOG -->
  <div class="card">
    <div class="card-title">
      <span>Nhật Ký Lệnh (Console Log)</span>
      <button class="btn btn-outline" style="padding:4px 8px; font-size:0.75rem;" onclick="document.getElementById('consoleBox').innerHTML=''">Xóa Log</button>
    </div>
    <div class="console-box" id="consoleBox">
      <div class="console-item">[READY] Kết nối thành công với ESP32 TMC2209 Stepper Bench!</div>
    </div>
  </div>
</div>

<script>
  let isEnabled = false;
  let isRunning = false;
  let isFwd = true;

  function log(msg) {
    const box = document.getElementById('consoleBox');
    const item = document.createElement('div');
    item.className = 'console-item';
    const t = new Date().toLocaleTimeString();
    item.innerText = `[${t}] ${msg}`;
    box.appendChild(item);
    box.scrollTop = box.scrollHeight;
  }

  function toggleEnable() {
    isEnabled = !isEnabled;
    const btn = document.getElementById('btnEnable');
    const badge = document.getElementById('driverStatusBadge');
    if (isEnabled) {
      btn.innerText = "🛑 TẮT KHÓA (EN = HIGH)";
      btn.className = "btn btn-outline";
      badge.innerText = "TMC2209 BẬT (HOLDING TORQUE)";
      badge.style.color = "var(--success)";
      log("BẬT TMC2209 (GPIO 23 = LOW -> Giữ cứng cốt motor)");
      fetch('/api/stepper/enable?state=1');
    } else {
      btn.innerText = "⚡ BẬT KHÓA TRỤC (EN = LOW)";
      btn.className = "btn btn-accent";
      badge.innerText = "TMC2209 TẮT (FREE RUN)";
      badge.style.color = "var(--accent)";
      log("TẮT TMC2209 (GPIO 23 = HIGH -> Thả lỏng cốt motor)");
      fetch('/api/stepper/enable?state=0');
    }
  }

  function toggleDirection() {
    isFwd = !isFwd;
    const btn = document.getElementById('btnDir');
    btn.innerText = isFwd ? "🔄 Chiều: TIẾN (FWD)" : "🔄 Chiều: LÙI (REV)";
    log(`Đổi chiều: ${isFwd ? 'TIẾN (GPIO 19 = HIGH)' : 'LÙI (GPIO 19 = LOW)'}`);
    fetch(`/api/stepper/dir?fwd=${isFwd ? 1 : 0}`);
  }

  function toggleRun() {
    isRunning = !isRunning;
    const btn = document.getElementById('btnRun');
    if (isRunning) {
      btn.innerText = "⏹ DỪNG QUAY LIÊN TỤC";
      btn.className = "btn btn-accent";
      log("TMC2209 bắt đầu quay liên tục...");
      fetch('/api/stepper/run?state=1');
    } else {
      btn.innerText = "▶ BẮT ĐẦU QUAY LIÊN TỤC";
      btn.className = "btn btn-primary";
      log("Đã dừng quay liên tục.");
      fetch('/api/stepper/run?state=0');
    }
  }

  function stopAll() {
    isRunning = false;
    document.getElementById('btnRun').innerText = "▶ BẮT ĐẦU QUAY LIÊN TỤC";
    document.getElementById('btnRun').className = "btn btn-primary";
    log("🛑 DỪNG TẤT CẢ HOẠT ĐỘNG!");
    fetch('/api/stepper/stop');
  }

  function jog(steps) {
    const fwd = steps > 0;
    const absSteps = Math.abs(steps);
    log(`Kéo TMC2209: ${steps > 0 ? '+' : ''}${steps} bước...`);
    fetch(`/api/stepper/jog?fwd=${fwd ? 1 : 0}&steps=${absSteps}`)
      .then(res => res.json())
      .then(d => log(`Jog OK: ${steps} bước`))
      .catch(e => log(`Lỗi: ${e}`));
  }

  function jogCustom(fwd) {
    const val = parseInt(document.getElementById('customSteps').value) || 0;
    if (val <= 0) return;
    jog(fwd ? val : -val);
  }

  function onSpeedSlider(v) {
    document.getElementById('speedLabel').innerText = v + " µs";
  }

  function sendSpeed(v) {
    log(`Đặt tốc độ: ${v} µs/step`);
    fetch(`/api/stepper/speed?delay=${v}`);
  }

  function sendServo(ang) {
    log(`Đặt Servo (GPIO 4): ${ang}°`);
    fetch(`/api/servo?angle=${ang}`);
  }

  function setServo(ang) {
    document.getElementById('servoSlider').value = ang;
    document.getElementById('servoLabel').innerText = ang + "°";
    sendServo(ang);
  }

  function resetSensor() {
    log("Đã xóa biến đếm cảm biến.");
    fetch('/api/sensor/reset');
  }

  // Ping kiểm tra kết nối và cập nhật trạng thái
  setInterval(() => {
    fetch('/api/status')
      .then(res => res.json())
      .then((data) => { 
        document.getElementById('connStatus').innerText = "ONLINE"; 
        
        // Update U-Sensor UI
        let stateStr = data.uSensorValue == 1 ? "HIGH (Mở)" : "LOW (Che)";
        let stateColor = data.uSensorValue == 1 ? "var(--success)" : "var(--accent)";
        let elState = document.getElementById('sensorState');
        elState.innerText = stateStr;
        elState.style.color = stateColor;
        document.getElementById('sensorCount').innerText = data.uSensorCount;

        // Update VL53L0X UI
        let elLaser = document.getElementById('laserDistance');
        if (data.vl53Distance == -1) {
          elLaser.innerText = "N/A (Out)";
          elLaser.style.color = "var(--text-muted)";
        } else {
          elLaser.innerText = data.vl53Distance + " mm";
          elLaser.style.color = "#818cf8";
        }
      })
      .catch(() => { document.getElementById('connStatus').innerText = "OFFLINE"; });
  }, 500);
</script>

</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
