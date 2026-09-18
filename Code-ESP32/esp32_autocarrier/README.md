# DIY Auto 35mm Film Scanner (ESP32 Autocarrier)

An open-source ESP32-based automatic film transport system for digitizing 35mm film using a digital camera. Inspired by the **Bobach 135 Autocarrier**, this project provides precise, high-speed frame-by-frame advancement with a custom **Hand Control Panel**.

![Circuit Diagram](circuit_diagram.jpg)

---

## ✨ Features

- **3 Operation Modes**: AUTO / SEMI / MANUAL
- **Sprocket-hole counting** via U-shaped optical sensor for precise frame alignment
- **Camera shutter trigger** via PC817 optocoupler module (supports 2.5mm / 3.5mm jack)
- **Hand Control Panel** via PCF8574T I2C expander (only 4 wires needed)
- **OLED display** (128x64) for real-time status
- **Web-based hardware test tools** (no app required — just connect to WiFi)
- **Serial Monitor commands** for quick debugging

---

## 🗂️ Project Structure

```
esp32_135_autocarrier/
├── esp32_135_autocarrier.ino   # Main firmware
├── config.h                    # All GPIO pin definitions & parameters
├── README.md                   # This file
├── circuit_diagram.jpg         # Full wiring block diagram
├── platformio.ini              # PlatformIO config (for VS Code users)
├── web_test_motor/             # Web UI tool: test stepper motor
├── web_test_sensor/            # Web UI tool: test optical sensor & shutter
└── web_test_servo_step/        # Web UI tool: test servo + stepper
```

---

## 🔌 Hardware List

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32 Dev Module | Main controller |
| Stepper Motor Driver | TMC2209 | Drives film transport motor |
| Stepper Motor | NEMA 42 | Film transport mechanism |
| Optical Sensor | U-shaped photo interrupter | Counts sprocket holes |
| Shutter Trigger | PC817 Optocoupler module | Triggers camera shutter |
| I2C Expander | PCF8574T | Hand Control Panel interface |
| Display | 128x64 OLED (SSD1306/SH1106) | Status display |
| Power Supply | 12V DC Adapter | Main power input |
| Voltage Regulator | Mini560 DC-DC Buck (5V/5A) | 5V rail for ESP32 & logic |

---

## ⚡ Wiring (GPIO Pin Assignment)

### ESP32 → Peripherals

| ESP32 Pin | Device | Device Pin | Function |
|---|---|---|---|
| GPIO18 | TMC2209 | STEP | Step pulse |
| GPIO19 | TMC2209 | DIR | Motor direction |
| GPIO23 | TMC2209 | EN | Enable/disable motor (Active LOW) |
| GPIO25 | PC817 Module | IN+ | Camera shutter trigger |
| GPIO32 | U-shaped sensor | OUT | Sprocket hole counter (input) |
| GPIO21 | OLED + PCF8574T | SDA | I2C data |
| GPIO22 | OLED + PCF8574T | SCL | I2C clock |
| GPIO34 | PCF8574T | INT | Interrupt from Hand Control Panel |

### PCF8574T (I2C: 0x24) → Hand Control Panel

| PCF8574T Pin | Component | Function |
|---|---|---|
| P0 | KY-040 Rotary Encoder | CLK (JOG position) |
| P1 | KY-040 Rotary Encoder | DT (JOG position) |
| P2 | MTS-103 Toggle Switch | AUTO mode pin |
| P3 | MTS-103 Toggle Switch | MANUAL mode pin |
| P4 | Shutter Button | Trigger camera shutter |
| P5 | Start/Stop Button | Start / pause scanning |
| P6 | — | NC (unused) |
| P7 | — | NC (unused) |

> **Note:** MTS-103 center pin → GND. When neither P2 nor P3 is active = **SEMI** mode.

---

## 🎮 Operation Modes

### AUTO — Full Roll Scan
Press the **Start/Stop button** → System auto-triggers the camera, counts exactly **8 sprocket holes** to advance one full frame (36x24mm), then repeats until end of roll.

### SEMI — Frame-by-Frame (Safest)
Ideal for slide film (E-6) or irregularly spaced frames. Align the first frame manually → press **Shutter button** → camera fires → film advances exactly 1 frame (8 holes) → waits for next press.

### MANUAL — Hand Wheel Control
- **Turn JOG encoder clockwise** → motor advances film forward step by step.
- **Turn JOG encoder counter-clockwise** → motor rewinds film.
- **Press Shutter button** → triggers camera at the current frame.

---

## 🔋 Power Supply

```
12V DC Adapter
    |
    |--► VMOT (TMC2209) — Motor power rail (12V)
    |
    └--► Mini560 DC-DC Buck Converter
              |
              └--► 5V --► ESP32 VIN + all 5V logic
```

---

## 📷 Camera Shutter Connection (PC817 Module)

The PC817 optocoupler **module** (resistor built-in) provides galvanic isolation between ESP32 and camera:

```
ESP32 GPIO25 --► IN+  [PC817 Module]  OUT+ --► Camera Jack Tip + Ring (Shutter / Focus)
ESP32 GND    --► IN-                  OUT- --► Camera Jack Sleeve (GND)
```

Supports standard **2.5mm** or **3.5mm** camera remote shutter jacks.

---

## 🖥️ Serial Monitor Commands (Baud: 115200)

Connect via USB and send commands:

| Command | Action |
|---|---|
| `start` | Start auto scanning |
| `stop` | Emergency stop |
| `mode auto` | Switch to AUTO mode |
| `mode semi` | Switch to SEMI mode |
| `mode manual` | Switch to MANUAL mode |
| `full` | Set Full-Frame (8 holes per frame) |
| `half` | Set Half-Frame (4 holes per frame) |
| `reset` | Reset frame counter to 0 |

---

## 🌐 Web-Based Hardware Test Tools

Each sub-folder contains a standalone test sketch. Flash it separately to test hardware without the main firmware.

### `web_test_motor/`
- **WiFi AP:** `ESP32_Autocarrier_Test` | Password: `12345678`
- **URL:** http://192.168.4.1
- Test stepper motor: step jogging, continuous run, speed (us delay), enable/disable driver.

### `web_test_sensor/`
- Test U-shaped optical sensor (live sprocket count).
- Test camera shutter trigger via PC817 optocoupler.

### `web_test_servo_step/`
- Servo angle slider (0-180 degrees) with Auto Sweep mode.
- Combined stepper + servo test.

---

## 🛠️ Flashing the Firmware

### Arduino IDE
1. Open `esp32_135_autocarrier.ino` in Arduino IDE.
2. Install libraries via **Library Manager**:
   - `Adafruit SH110X`
   - `Adafruit GFX Library`
3. Select board: **ESP32 Dev Module**.
4. Select the correct COM port → click **Upload**.

### PlatformIO (VS Code)
```bash
pio run --target upload
```

---

## ⚙️ Key Parameters (`config.h`)

| Define | Default | Description |
|---|---|---|
| `HOLES_FULL_FRAME` | `8` | Sprocket holes per full frame (36x24mm) |
| `HOLES_HALF_FRAME` | `4` | Sprocket holes per half frame (18x24mm) |
| `DEFAULT_STEP_DELAY_US` | `570` | Motor speed (us between step pulses) |
| `JOG_STEPS_PER_CLICK` | `32` | Motor steps per JOG encoder click |
| `SENSOR_DEBOUNCE_US` | `15000` | Optical sensor debounce time (us) |
| `SHUTTER_PULSE_MS` | `300` | Shutter trigger pulse duration (ms) |
| `POST_SHUTTER_DELAY_MS` | `400` | Wait after shutter before advancing (ms) |
| `SETTLE_DELAY_MS` | `80` | Vibration settle time before shutter (ms) |
| `FILM_TIMEOUT_MS` | `4500` | Film advance timeout protection (ms) |

---

## 📄 License

This project is released as open-source. Feel free to use, modify, and share.

---

*Inspired by the Bobach 135 Autocarrier. Built for the film photography community.*
