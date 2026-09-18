#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <AccelStepper.h>
#include "config.h"

// --- OLED DISPLAY INIT ---
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool hasOLED = false;

// --- STEPPER MOTOR INIT ---
AccelStepper stepper(AccelStepper::DRIVER, PIN_MOTOR_STEP, PIN_MOTOR_DIR);

// --- OPERATION MODES ---
enum OperationMode {
  MODE_MANUAL, // Turn JOG encoder to move film; press Shutter button to capture
  MODE_SEMI,   // Press Start -> advance 8 holes -> capture -> wait for next Start press
  MODE_AUTO    // Press Start -> (advance 8 holes -> capture -> repeat) continuously
};
OperationMode currentOpMode = MODE_MANUAL;

// --- SYSTEM STATE (STATE MACHINE) ---
enum SystemState {
  STATE_IDLE,
  STATE_RUNNING,       // Advancing film, counting sprocket holes
  STATE_SETTLING,      // Waiting for mechanical vibration to settle
  STATE_TRIGGERING,    // Triggering camera shutter
  STATE_WAIT_EXPOSURE, // Waiting for exposure and file save
  STATE_JOGGING        // Manual JOG encoder movement
};
SystemState currentState = STATE_IDLE;

// --- OPERATING PARAMETERS ---
int speedDelay = DEFAULT_STEP_DELAY_US; // Step delay in microseconds
int holeCount = 0;
int currentFrame = 0;
unsigned long stateTimer = 0;

// --- SENSOR & BUTTON VARIABLES ---
int lastUSensorState = HIGH;
unsigned long lastDebounceTime = 0;

bool lastBtnShutterState = true;
bool lastBtnStartState = true;
unsigned long startBtnPressTime = 0;
bool isJogSpeedAdjustMode = false;

// --- PCF8574 VARIABLES ---
volatile bool flag_pcf_changed = false;
uint8_t lastPcfState = 0xFF; // Initially all HIGH
uint8_t lastJogEncodedState = 0;
uint8_t lastSpdEncodedState = 0;

//
// ================================================================
//
void IRAM_ATTR isr_pcf() {
  flag_pcf_changed = true;
}


//
// 2. GIAO DIỆN WEB (REMOVED)
//

void updateDisplay() {
  if (!hasOLED) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  
  // Current mode
  display.setCursor(0, 0);
  display.print(F("MODE: "));
  if (currentOpMode == MODE_AUTO) display.print(F("AUTO SCAN"));
  else if (currentOpMode == MODE_SEMI) display.print(F("SEMI-AUTO"));
  else {
    if (isJogSpeedAdjustMode) display.print(F("MANUAL (SPD ADJ)"));
    else display.print(F("MANUAL"));
  }

  // Parameters
  display.setCursor(0, 15);
  display.printf("Frm: %02d | H: %d/8", currentFrame, holeCount);
  
  display.setCursor(0, 30);
  display.printf("Spd: %dus", speedDelay);

  // Operating state
  display.setCursor(0, 45);
  switch (currentState) {
    case STATE_IDLE: display.print(F("READY / IDLE")); break;
    case STATE_RUNNING: display.print(F(">> ADVANCING FILM >>")); break;
    case STATE_SETTLING: display.print(F("Settling...")); break;
    case STATE_TRIGGERING: display.print(F("--> CAPTURING <--")); break;
    case STATE_WAIT_EXPOSURE: display.print(F("Saving file...")); break;
    case STATE_JOGGING: display.print(F("Jogging...")); break;
  }
  display.display();
}

void processStartCommand() {
  if (currentState == STATE_IDLE) {
    if (currentOpMode == MODE_AUTO || currentOpMode == MODE_SEMI) {
      currentState = STATE_TRIGGERING;
      stateTimer = millis();
      Serial.println(">>> START: Capture first, then advance <<<");
      updateDisplay();
    }
  }
}

void processStopCommand() {
  currentState = STATE_IDLE;
  stepper.stop();
  stepper.disableOutputs();
  digitalWrite(PIN_SHUTTER_OPTO, LOW); // Ensure shutter is released
  Serial.println(">>> STOPPED <<<");
  updateDisplay();
}



//
// 3. SETUP
//
void setup() {
  Serial.begin(115200);

  // Configure motor GPIO pins
  pinMode(PIN_MOTOR_STEP, OUTPUT);
  pinMode(PIN_MOTOR_DIR, OUTPUT);
  pinMode(PIN_MOTOR_EN, OUTPUT);

  // Configure AccelStepper
  stepper.setEnablePin(PIN_MOTOR_EN);
  stepper.setPinsInverted(true, false, true); // Invert DIR; EN is active LOW
  stepper.setMinPulseWidth(20);
  stepper.setAcceleration(800.0);
  stepper.setMaxSpeed((1000000.0 / DEFAULT_STEP_DELAY_US) * 0.6);
  stepper.disableOutputs();

  // Configure shutter optocoupler output
  pinMode(PIN_SHUTTER_OPTO, OUTPUT);
  digitalWrite(PIN_SHUTTER_OPTO, LOW);

  // Configure U-shaped optical sensor
  pinMode(PIN_U_SENSOR, INPUT_PULLUP);

  // Start I2C bus (shared by OLED and PCF8574T)
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  // --- SCAN I2C BUS ---
  Serial.println("\n--- SCANNING I2C BUS FOR DEVICES ---");
  int foundCount = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.printf("-> Device found at: 0x%02X\n", i);
      foundCount++;
    }
  }
  if (foundCount == 0) {
    Serial.println("-> NO I2C DEVICES FOUND ON BUS!");
  }
  Serial.println("--------------------------------------\n");

  // --- CHECK PCF8574T ---
  Wire.beginTransmission(I2C_ADDRESS_PCF);
  byte errorPCF = Wire.endTransmission();
  if (errorPCF == 0) {
    Serial.printf("[OK] PCF8574T found at address: 0x%02X\n", I2C_ADDRESS_PCF);
    // Configure PCF8574T interrupt
    pinMode(PIN_PCF_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_PCF_INT), isr_pcf, FALLING);
    Wire.beginTransmission(I2C_ADDRESS_PCF);
    Wire.write(0xFF); // Set all P0-P7 pins as input (pull-up)
    Wire.endTransmission();
  } else {
    Serial.println("[ERROR] PCF8574T not found. Check SDA/SCL wiring!");
  }

  // --- CHECK OLED ---
  Wire.beginTransmission(OLED_I2C_ADDRESS);
  byte errorOLED = Wire.endTransmission();
  if (errorOLED == 0) {
    if (display.begin(OLED_I2C_ADDRESS, true)) {
      Serial.printf("[OK] OLED display found at address: 0x%02X\n", OLED_I2C_ADDRESS);
      hasOLED = true;
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(10, 20);
      display.println(F("Have a nice day !!!"));
      display.display();
      delay(1500);
    }
  } else {
    Serial.printf("[ERROR] OLED not found at address: 0x%02X\n", OLED_I2C_ADDRESS);
  }

  updateDisplay();
}

//
// 4. MAIN LOOP
//
void loop() {
  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();

  // ----------------------------------------------------
  // READ I2C PCF8574T (BUTTONS & ENCODER)
  // ----------------------------------------------------
  if (flag_pcf_changed) {
    flag_pcf_changed = false;
    
    Wire.requestFrom(I2C_ADDRESS_PCF, 1);
    if (Wire.available()) {
      uint8_t pcfState = Wire.read();

      // 1. Decode mode toggle switch (MTS-103)
      bool isAutoMode = !((pcfState >> PCF_PIN_MODE_AUTO) & 1);
      bool isManualMode = !((pcfState >> PCF_PIN_MODE_MANUAL) & 1);
      
      OperationMode newMode;
      if (isAutoMode) newMode = MODE_AUTO;
      else if (isManualMode) newMode = MODE_MANUAL;
      else newMode = MODE_SEMI;

      if (newMode != currentOpMode && currentState == STATE_IDLE) {
        currentOpMode = newMode;
        updateDisplay();
      }

      // 2. Decode Shutter button (P4)
      bool btnShutter = (pcfState >> PCF_PIN_BTN_SHUTTER) & 1;
      if (lastBtnShutterState == true && btnShutter == false) { // Falling edge (HIGH -> LOW)
        if (currentState == STATE_IDLE && currentOpMode == MODE_MANUAL) {
          currentState = STATE_TRIGGERING;
          stateTimer = currentMillis;
          updateDisplay();
        }
      }
      lastBtnShutterState = btnShutter;

      // 3. Decode Start/Stop button (P5)
      bool btnStart = (pcfState >> PCF_PIN_BTN_START) & 1;
      if (lastBtnStartState == true && btnStart == false) { // Falling edge (HIGH -> LOW)
        startBtnPressTime = currentMillis;
        if (currentOpMode == MODE_MANUAL) {
          if (isJogSpeedAdjustMode) {
            isJogSpeedAdjustMode = false;
            updateDisplay();
          }
        } else {
          if (currentState == STATE_IDLE) {
            processStartCommand();
          } else if (currentState == STATE_RUNNING || currentState == STATE_JOGGING) {
            processStopCommand();
          }
        }
      }
      lastBtnStartState = btnStart;

      // 4. Decode JOG encoder (P0, P1)
      uint8_t jogMSB = (pcfState >> PCF_PIN_ENC_JOG_CLK) & 1;
      uint8_t jogLSB = (pcfState >> PCF_PIN_ENC_JOG_DT) & 1;
      uint8_t jogEncoded = (jogMSB << 1) | jogLSB;
      uint8_t jogSum = (lastJogEncodedState << 2) | jogEncoded;

      int encoderJogDelta = 0;
      if (jogSum == 0b1101 || jogSum == 0b0100 || jogSum == 0b0010 || jogSum == 0b1011) encoderJogDelta = 1;
      if (jogSum == 0b1110 || jogSum == 0b0111 || jogSum == 0b0001 || jogSum == 0b1000) encoderJogDelta = -1;
      lastJogEncodedState = jogEncoded;

      if (encoderJogDelta != 0 && currentState == STATE_IDLE && currentOpMode == MODE_MANUAL) {
        if (isJogSpeedAdjustMode) {
          speedDelay -= encoderJogDelta * 50;
          if (speedDelay < 100) speedDelay = 100;
          if (speedDelay > 3000) speedDelay = 3000;
          stepper.setMaxSpeed((1000000.0 / speedDelay) * 0.6);
          updateDisplay();
        } else {
          long steps = abs(encoderJogDelta) * JOG_STEPS_PER_CLICK;
          if (encoderJogDelta < 0) steps = -steps;
          stepper.enableOutputs();
          stepper.setAcceleration(800.0);
          stepper.setMaxSpeed((1000000.0 / speedDelay) * 1.2); // 2x speed for JOG
          stepper.move(steps);
          currentState = STATE_JOGGING;
          updateDisplay();
        }
      }

      lastPcfState = pcfState;
    }
  }

  // Hold Start button 2 seconds in MANUAL mode to enter speed-adjust mode
  if (!lastBtnStartState && currentOpMode == MODE_MANUAL && !isJogSpeedAdjustMode) {
    if (currentMillis - startBtnPressTime >= 2000) {
      isJogSpeedAdjustMode = true;
      updateDisplay();
    }
  }

  // ----------------------------------------------------
  // MOTOR CONTROL
  // ----------------------------------------------------
  stepper.run();

  // ----------------------------------------------------
  // STATE MACHINE
  // ----------------------------------------------------
  switch (currentState) {
    case STATE_IDLE:
      // Nothing to do, waiting for command
      break;
      
    case STATE_JOGGING:
      if (stepper.distanceToGo() == 0) {
        // Do NOT disable motor here — keep magnetic brake engaged
        currentState = STATE_IDLE;
        updateDisplay();
      }
      break;

    case STATE_RUNNING:
      // Poll U-shaped optical sensor for sprocket holes
      {
        int sensorVal = digitalRead(PIN_U_SENSOR);
        if (sensorVal != lastUSensorState) {
          if (currentMicros - lastDebounceTime > SENSOR_DEBOUNCE_US) {
            lastUSensorState = sensorVal;
            lastDebounceTime = currentMicros; // Reset debounce timer
            // Falling edge (LOW) = sprocket hole detected
            if (sensorVal == LOW) {
              holeCount++;
              Serial.printf("Hole detected: %d/8\n", holeCount);
              updateDisplay();
              
              if (holeCount >= HOLES_FULL_FRAME) {
                // All 8 holes counted -> hard brake to prevent overshoot
                stepper.setAcceleration(20000.0); // Max deceleration for instant stop
                stepper.stop();
                currentState = STATE_SETTLING;
                stateTimer = currentMillis;
                Serial.println("8 holes counted. Waiting for vibration to settle...");
                updateDisplay();
              }
            }
          }
        }
      }
      break;

    case STATE_SETTLING:
      if (stepper.distanceToGo() == 0) {
        if (currentMillis - stateTimer >= SETTLE_DELAY_MS) {
          if (currentOpMode == MODE_AUTO) {
            currentState = STATE_TRIGGERING;
            stateTimer = currentMillis;
          } else {
            currentState = STATE_IDLE;
            Serial.println("SEMI cycle complete. Waiting for next Start command...");
          }
          updateDisplay();
        }
      } else {
        // Keep resetting timer until motor fully stops
        stateTimer = currentMillis;
      }
      break;

    case STATE_TRIGGERING:
      digitalWrite(PIN_SHUTTER_OPTO, HIGH); // Trigger camera shutter
      if (currentMillis - stateTimer >= SHUTTER_PULSE_MS) {
        digitalWrite(PIN_SHUTTER_OPTO, LOW);
        currentState = STATE_WAIT_EXPOSURE;
        stateTimer = currentMillis;
        currentFrame++; // Increment frame counter
        updateDisplay();
      }
      break;

    case STATE_WAIT_EXPOSURE:
      // Wait for camera to finish exposure and release shutter
      if (currentMillis - stateTimer >= POST_SHUTTER_DELAY_MS) {
        if (currentOpMode == MODE_AUTO || currentOpMode == MODE_SEMI) {
          // Advance to next frame
          holeCount = 0;
          stepper.enableOutputs();
          stepper.setAcceleration(800.0); // Restore smooth acceleration
          stepper.setMaxSpeed((1000000.0 / speedDelay) * 0.6);
          stepper.setCurrentPosition(0);
          stepper.move(1000000);
          currentState = STATE_RUNNING;
          Serial.println("Advancing to next frame...");
        } else {
          // MANUAL mode: capture only, no auto-advance
          currentState = STATE_IDLE;
        }
        updateDisplay();
      }
      break;
  }
}

