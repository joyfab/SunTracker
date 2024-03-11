// Elevation motor dSPIN® Driver 
//  (Raspberry Pi Pico RP2040-Zero). by joy. mars 2024.
#include <Arduino.h>
#include "PioSPI.h"
PioSPI spiBus(4, 3, 2, 5, SPI_MODE3, 5000000); //MOSI, MISO, SCK, CS, SPI_MODE, FREQUENCY
#include "L6470.h"
#define dSPIN_RESET       9  // RESET 
#define dSPIN_BUSYN       6  // BSYN  
#define Flag              7  // Flag
#define SW               14  // Home Switch (in or out)
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, 16, NEO_GRB + NEO_KHZ800);
char blop;
String input = "";
void setup()  {
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(dSPIN_RESET, OUTPUT);
  pinMode(dSPIN_BUSYN, INPUT);
  pixels.begin();
  pixels.clear();
  for (int i = 0; i < 1; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 1, 0));
    pixels.show();
  }
  dSPIN_init();
  dSPIN_SetParam(dSPIN_STEP_MODE,
                 !dSPIN_SYNC_EN |
                 dSPIN_STEP_SEL_1_128 |
                 dSPIN_SYNC_SEL_64);
  dSPIN_SetParam(dSPIN_MAX_SPEED, MaxSpdCalc(200));
  dSPIN_SetParam(dSPIN_FS_SPD, FSCalc(0x3FF));
  dSPIN_SetParam(dSPIN_ACC, 10);
  dSPIN_SetParam(dSPIN_DEC, 10);
  dSPIN_SetParam(dSPIN_OCD_TH, dSPIN_OCD_TH_2250mA);
  dSPIN_SetParam(dSPIN_CONFIG,
                 dSPIN_CONFIG_PWM_DIV_1 |
                 dSPIN_CONFIG_PWM_MUL_2 |
                 dSPIN_CONFIG_SR_180V_us |
                 dSPIN_CONFIG_OC_SD_DISABLE |
                 dSPIN_CONFIG_VS_COMP_DISABLE |
                 dSPIN_CONFIG_SW_MODE |
                 dSPIN_CONFIG_SW_USER |
                 dSPIN_CONFIG_INT_16MHZ);
  dSPIN_SetParam(dSPIN_KVAL_RUN,  20);
  dSPIN_SetParam(dSPIN_KVAL_ACC,  30);
  dSPIN_SetParam(dSPIN_KVAL_DEC,  30);
  dSPIN_SetParam(dSPIN_KVAL_HOLD, 10);
  dSPIN_GetStatus();
}
void loop()  {
  if (Serial1.available())
    while (Serial1.available() > 0) {
      char inChar = (char)Serial1.read();
      input += inChar;
    }
  if (input.length() >= 1) {
    {
      blop = Serial1.read();
    }
    dSPIN_SoftStop();
    dSPIN_SetParam(dSPIN_MAX_SPEED, MaxSpdCalc(200));
    if (input == "E") {  // Azimut A1000 = Go to 1000. E Elevation E1000 = Go to 1000.
      dSPIN_GoTo(Serial1.parseInt());
      pixels.clear();
      for (int i = 0; i < 1; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 10));
        pixels.show();
      }
      while (digitalRead(dSPIN_BUSYN) == LOW);
    }
    if (input == "J") {  // Azimut homing. J Elevation homing.
      dSPIN_GoUntil(ACTION_RESET, REV, 10000);
      pixels.clear();
      for (int i = 0; i < 1; i++) {
        pixels.setPixelColor(i, pixels.Color(10, 0, 0));
        pixels.show();
      }
      while (digitalRead(dSPIN_BUSYN) == LOW);
      dSPIN_ResetPos();
      dSPIN_GoTo(0);
      while (digitalRead(dSPIN_BUSYN) == LOW);
      dSPIN_ResetPos();
      dSPIN_GoTo(Serial1.parseInt());
      dSPIN_SetParam(dSPIN_MAX_SPEED, MaxSpdCalc(50));
      while (digitalRead(dSPIN_BUSYN) == LOW);
      dSPIN_ResetPos();
    }
    input = "";
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 1, 0));
      pixels.show();
    }
  }
}
