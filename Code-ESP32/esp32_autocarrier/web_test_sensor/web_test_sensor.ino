#include <Wire.h>
#include "../config.h"

void setup() {
  Serial.begin(115200);
  
  // U-Sensor pin
  pinMode(PIN_U_SENSOR, INPUT_PULLUP);
  
  // Initialize I2C for PCF8574T
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.beginTransmission(I2C_ADDRESS_PCF);
  Wire.write(0xFF); // Set all 8 pins as input (pull-up)
  Wire.endTransmission();
  
  Serial.println("===============================");
  Serial.println("   SENSOR & BUTTON TEST START  ");
  Serial.println("===============================");
}

void loop() {
  // Read U-Sensor
  int u_sensor = digitalRead(PIN_U_SENSOR);
  
  // Read PCF8574
  uint8_t pcfState = 0xFF;
  Wire.requestFrom(I2C_ADDRESS_PCF, 1);
  if (Wire.available()) {
    pcfState = Wire.read();
  }
  
  int enc_jog_clk = (pcfState >> PCF_PIN_ENC_JOG_CLK) & 1;
  int enc_jog_dt = (pcfState >> PCF_PIN_ENC_JOG_DT) & 1;
  int enc_spd_clk = (pcfState >> PCF_PIN_ENC_SPD_CLK) & 1;
  int enc_spd_dt = (pcfState >> PCF_PIN_ENC_SPD_DT) & 1;
  int mode_auto = (pcfState >> PCF_PIN_MODE_AUTO) & 1;
  int mode_manual = (pcfState >> PCF_PIN_MODE_MANUAL) & 1;
  int btn_shutter = (pcfState >> PCF_PIN_BTN_SHUTTER) & 1;
  int btn_start = (pcfState >> PCF_PIN_BTN_START) & 1;
  
  // Print values
  Serial.print("U_Sens:"); Serial.print(u_sensor);
  Serial.print(" | JOG_CLK:"); Serial.print(enc_jog_clk);
  Serial.print(" | JOG_DT:"); Serial.print(enc_jog_dt);
  Serial.print(" | SPD_CLK:"); Serial.print(enc_spd_clk);
  Serial.print(" | SPD_DT:"); Serial.print(enc_spd_dt);
  Serial.print(" | M_AUTO:"); Serial.print(mode_auto);
  Serial.print(" | M_MAN:"); Serial.print(mode_manual);
  Serial.print(" | B_Shut:"); Serial.print(btn_shutter);
  Serial.print(" | B_Start:"); Serial.println(btn_start);
  
  delay(500); // 500ms
}
