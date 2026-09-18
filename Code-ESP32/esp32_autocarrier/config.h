#ifndef CONFIG_H
#define CONFIG_H

// ================================================================
// ESP32 GPIO PIN ASSIGNMENT
// ================================================================

// 1. NEMA 42 Stepper Motor Driver (TMC2209)
#define PIN_MOTOR_STEP 18 // Step pulse pin
#define PIN_MOTOR_DIR 19  // Direction pin
#define PIN_MOTOR_EN 23   // Enable pin (Active LOW)

// 2. Camera shutter trigger via PC817 Optocoupler module
#define PIN_SHUTTER_OPTO 25 // Shutter trigger output

// 3. U-shaped optical sensor (counts film sprocket holes)
#define PIN_U_SENSOR 32

// 4. HAND CONTROL PANEL (detached) - PCF8574T I2C Expander
#define I2C_ADDRESS_PCF 0x24 // I2C address of PCF8574T
#define PIN_PCF_INT 34       // Interrupt pin from PCF8574T to ESP32

// PCF8574T bit mapping (P0 - P5)
#define PCF_PIN_ENC_JOG_CLK 0  // KY-040 Jog encoder - CLK
#define PCF_PIN_ENC_JOG_DT  1  // KY-040 Jog encoder - DT
#define PCF_PIN_MODE_AUTO   2  // MTS-103 toggle switch - AUTO pin
#define PCF_PIN_MODE_MANUAL 3  // MTS-103 toggle switch - MANUAL pin
#define PCF_PIN_BTN_SHUTTER 4  // Shutter button
#define PCF_PIN_BTN_START   5  // Start/Stop button

// 5. OLED Display SSD1306/SH1106 I2C
#define PIN_OLED_SDA 21

#define PIN_OLED_SCL 22
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ================================================================
// OPERATING PARAMETERS & 35MM FILM STANDARDS
// ================================================================

// Number of sprocket holes per frame
#define HOLES_FULL_FRAME 8 // Full Frame 35mm: 36x24mm + 2mm gap = 8 holes (38mm)
#define HOLES_HALF_FRAME 4 // Half Frame 35mm: 18x24mm = 4 holes (19mm)

// Motor transport speed (microseconds delay between step pulses)
#define DEFAULT_STEP_DELAY_US 570

// Motor steps per JOG encoder click (Jog sensitivity)
// Higher value = faster film movement per click
#define JOG_STEPS_PER_CLICK 32

// U-shaped optical sensor debounce time (microseconds)
#define SENSOR_DEBOUNCE_US 15000 // 15ms

// Shutter trigger pulse duration (ms)
#define SHUTTER_PULSE_MS 300

// Post-shutter delay — wait for camera to finish exposure & save file (ms)
#define POST_SHUTTER_DELAY_MS 400

// Mechanical vibration settle time before triggering shutter (ms)
#define SETTLE_DELAY_MS 80

// Film advance timeout protection (ms)
#define FILM_TIMEOUT_MS 4500

#endif // CONFIG_H
