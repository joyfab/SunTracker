//  Two axis X/Y Azimut/Elevation Suntracker.RX/TX mode.
//  (Raspberry Pi Pico RP2040-Zero). by joy. mars 2024.
#include <Arduino.h>
#include <Wire.h>
#include "at24c256.h"
at24c256 eeprom(0x50);
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, 16, NEO_GRB + NEO_KHZ800);
#include <Helios.h>
Helios helios;                // sun/earth positioning calculator
double dAzimuth;              // Azimut angle
double dElevation;            // Elevation angle
#include <DS3231.h>           // RTC
DS3231 Clock;
bool Century = false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;
byte year, month, date, DoW, hour, minute, second;
unsigned int AzOffset;        // Azimuth Offset
unsigned int ElOffset;        // Elevation Offset
unsigned int AzBack;          // Retrait Switch1 Azimuth
unsigned int ElBack;          // Retrait Switch2 Elevation
unsigned int Ang;             // Angle minimum elevation
int Status = 0;               // Jour/NUit
char blop;
String input = "";
void setup() {
  Serial.begin(9600);   // usb
  Serial1.begin(9600);  // RX/TX commandes moteurs
  Serial2.begin(9600);  // Ble control
  Wire.setSDA(12);
  Wire.setSCL(13);
  Wire.setClock(100000);
  Wire.begin();
  eeprom.init();
  AzOffset = eeprom.read(30000);
  ElOffset = eeprom.read(30001);
  AzBack = eeprom.read(30002);
  ElBack = eeprom.read(30003);
  Ang = eeprom.read(30004);
  pixels.begin();
  pixels.clear();
  for (int i = 0; i < 1; i++) {
    pixels.setPixelColor(i, pixels.Color(1, 0, 0));
    pixels.show();
  }
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFlipMode(0);
  u8x8.setInverseFont(0);
  u8x8.clear();
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  u8x8.setCursor(1, 0);
  u8x8.print("PicoSunTracker");
  u8x8.setCursor(1, 2);
  u8x8.print("By Joy.");
  u8x8.refreshDisplay();
  delay(2000);
  Serial1.print("I");
  Serial1.print(AzBack * 100);
  Serial1.println("");
  Serial1.print("J");
  Serial1.print(ElBack * 100);
  Serial1.println("");
  u8x8.setCursor(1, 4);
  u8x8.print("Homing...");
  u8x8.refreshDisplay();
  delay(2000);
  u8x8.clear();
  u8x8.setPowerSave(1);
}
void ReadDS3231()  {
  int second, minute, hour, date, month, year, temperature;
  second = Clock.getSecond();
  minute = Clock.getMinute();
  hour = Clock.getHour(h12, PM);
  date = Clock.getDate();
  month = Clock.getMonth(Century);
  year = Clock.getYear();
  temperature = Clock.getTemperature();
}
void checkcom()  {
  ReadDS3231();
  if (Serial2.available())                   //   BLE control
    while (Serial2.available() > 0) {
      char inChar = (char)Serial2.read();
      input += inChar;
      pixels.clear();
      for (int i = 0; i < 1; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 250));
        pixels.show();
      }
      u8x8.setPowerSave(0);
    }
  if (input.length() >= 1) {
    {
      blop = Serial2.read();
    }
    if (input == "Olon") {                   // Test commands
      u8x8.setPowerSave(0);
      Serial2.println("");
      Serial2.println("LCD ON");
    }
    if (input == "Olof") {                   // Test commands
      u8x8.setPowerSave(1);
      Serial2.println("");
      Serial2.println("LCD OFF");
    }
    if (input == "Wind") {  // Position vent horizontale
      Serial2.println("Wind Position");
      Serial1.print("A909180");  // 255°  night position (west azimut)
      Serial1.println("");
      Serial1.print("E364672");  // 66 + 21 = 87°  night position (horizontal)
      Serial1.println("");
      delay(61000);
    }
    if (input == "Home") {              // Homing
      Serial2.println(" ");
      Serial2.println("Homing...");             // Homing back off.
      Serial1.print("I");
      Serial1.print(AzBack * 100);
      Serial1.println("");
      delay(100);
      Serial1.print("J");
      Serial1.print(ElBack * 100);
      Serial1.println("");
      delay(60000);
      Serial2.println("Restart..");
      delay(100);
    }
    if (input == "Rsto") {            // Reset Offset to 10
      Serial2.println("");
      Serial2.print("AzOffset = ");
      AzOffset = 100;
      eeprom.write(30000, 100);
      Serial2.print(AzOffset, DEC);
      Serial2.println("");
      Serial2.print("ElOffset = ");
      ElOffset = 100;
      eeprom.write(30001, 100);
      Serial2.print(ElOffset, DEC);
      Serial2.println("");
      Serial2.print("AzBack = ");
      AzBack = 100;
      eeprom.write(30002, 100);
      Serial2.print(AzBack, DEC);
      Serial2.println("");
      Serial2.print("ElBack = ");
      ElBack = 100;
      eeprom.write(30003, 100);
      Serial2.print(ElBack, DEC);
      Serial2.println("");
      Serial2.print("Ang = ");
      Ang = 5;
      eeprom.write(30004, 5);
      Serial2.println(Ang, DEC);
      Serial2.println("");
    }
    if (input == "Az+") {                   // Azimuth Offset +
      Serial2.println("");
      Serial2.print("AzOffset = ");
      AzOffset = AzOffset + 1;
      Serial2.println(AzOffset, DEC);
      eeprom.write(30000, AzOffset);
    }
    if (input == "Az-") {                   // Azimuth Offset -
      Serial2.println("");
      Serial2.print("AzOffset = ");
      AzOffset = AzOffset - 1;
      Serial2.println(AzOffset, DEC);
      eeprom.write(30000, AzOffset);
    }
    if (input == "El+") {                   // Elevation Offset +
      Serial2.println("");
      Serial2.print("ElOffset = ");
      ElOffset = ElOffset + 1;
      Serial2.println(ElOffset, DEC);
      eeprom.write(30001, ElOffset);
    }
    if (input == "El-") {                   // Elevation Offset -
      Serial2.println("");
      Serial2.print("ElOffset = ");
      ElOffset = ElOffset - 1;
      Serial2.println(ElOffset, DEC);
      eeprom.write(30001, ElOffset);
    }
    if (input == "AzB+") {                   // Azimuth Back +
      Serial2.println("");
      Serial2.print("AzBack = ");
      AzBack = AzBack + 1;
      Serial2.println(AzBack, DEC);
      eeprom.write(30002, AzBack);
    }
    if (input == "AzB-") {                   // Azimuth Back -
      Serial2.println("");
      Serial2.print("AzBack = ");
      AzBack = AzBack - 1;
      Serial2.println(AzBack, DEC);
      eeprom.write(30002, AzBack);
    }
    if (input == "ElB+") {                   // Elevation Back  +
      Serial2.println("");
      Serial2.print("ElBack= ");
      ElBack = ElBack + 1;
      Serial2.println(ElBack, DEC);
      eeprom.write(30003, ElBack);
    }
    if (input == "ElB-") {                   // Elevation Offset -
      Serial2.println("");
      Serial2.print("ElBack= ");
      ElBack = ElBack - 1;
      Serial2.println(ElBack, DEC);
      eeprom.write(30003, ElBack);
    }
    if (input == "Ang+") {                   // Elevation Back  +
      Serial2.println("");
      Serial2.print("Ang = ");
      Ang = Ang + 1;
      Serial2.println(Ang, DEC);
      eeprom.write(30004, Ang);
    }
    if (input == "Ang-") {                   // Elevation Back  +
      Serial2.println("");
      Serial2.print("Ang = ");
      Ang = Ang - 1;
      Serial2.println(Ang, DEC);
      eeprom.write(30004, Ang);
    }
    if (input == "Ctrl") {
      Serial2.println("");
      if (Clock.getDoW() == 1 ) {
        Serial2.print("Dimanche ");
      }
      if (Clock.getDoW() == 2 ) {
        Serial2.print("Lundi ");
      }
      if (Clock.getDoW() == 3 ) {
        Serial2.print("Mardi ");
      }
      if (Clock.getDoW() == 4 ) {
        Serial2.print("Mercredi ");
      }
      if (Clock.getDoW() == 5 ) {
        Serial2.print("Jeudi ");
      }
      if (Clock.getDoW() == 6 ) {
        Serial2.print("Vendredi ");
      }
      if (Clock.getDoW() == 7 ) {
        Serial2.print("Samedi ");
      }
      Serial2.print(Clock.getDate(), DEC);
      Serial2.print(" ");
      if (Clock.getMonth(Century) == 1 ) {
        Serial2.print("Janvier ");
      }
      if (Clock.getMonth(Century) == 2 ) {
        Serial2.print("Fevrier ");
      }
      if (Clock.getMonth(Century) == 3 ) {
        Serial2.print("Mars ");
      }
      if (Clock.getMonth(Century) == 4 ) {
        Serial2.print("Avril ");
      }
      if (Clock.getMonth(Century) == 5 ) {
        Serial2.print("Mai ");
      }
      if (Clock.getMonth(Century) == 6 ) {
        Serial2.print("Juin ");
      }
      if (Clock.getMonth(Century) == 7 ) {
        Serial2.print("Juillet ");
      }
      if (Clock.getMonth(Century) == 8 ) {
        Serial2.print("Aout ");
      }
      if (Clock.getMonth(Century) == 9 ) {
        Serial2.print("Septembre ");
      }
      if (Clock.getMonth(Century) == 10 ) {
        Serial2.print("Octobre ");
      }
      if (Clock.getMonth(Century) == 11 ) {
        Serial2.print("Novembre ");
      }
      if (Clock.getMonth(Century) == 12 ) {
        Serial2.print("Decembre ");
      }
      Serial2.print("20");
      Serial2.print(Clock.getYear(), DEC);
      Serial2.println("");
      Serial2.print(Clock.getHour(h12, PM), DEC);
      Serial2.print(":");
      Serial2.print(Clock.getMinute(), DEC);
      Serial2.print(":");
      Serial2.print(Clock.getSecond(), DEC);
      Serial2.print(" UT ");
      if (Status == 0)  {
        Serial2.println("Nuit ");
      }
      if (Status == 1)  {
        Serial2.println("Jour ");
      }
      Serial2.print("Temp RTC : ");
      Serial2.print(Clock.getTemperature(), 1);
      Serial2.println("C");
      Serial2.print("Temp CPU : ");
      Serial2.print(analogReadTemp(), 1);
      Serial2.println("C");
      Serial2.print("Azimuth  : ");
      Serial2.println(dAzimuth, 2);
      Serial2.print("Elevation: ");
      Serial2.println(dElevation, 2);
      Serial2.print(" Offset ");
      Serial2.print(AzOffset);
      Serial2.print(" Back ");
      Serial2.println(AzBack);
      Serial2.print(" Offset ");
      Serial2.print(ElOffset);
      Serial2.print(" Back ");
      Serial2.println(ElBack);
      Serial2.print("Ang : ");
      Serial2.println(Ang);
    }
    input = "";
  }
  delay(10);
}
void track() {              // exemple longitude et latitude : 5°37'43.66"E,44°18'58.91"N  => 5.62879,44.31636 decimal.
  ReadDS3231();            //see https://www.coordonnees-gps.fr/conversion-coordonnees-gps  5 chiffres après un point (.) et non pas virgule(,) comme ceci:
  helios.calcSunPos(Clock.getYear(), Clock.getMonth(Century), Clock.getDate(), Clock.getHour(h12, PM), Clock.getMinute(), Clock.getSecond(), 5.62879, 44.31636);
  dAzimuth = helios.dAzimuth;
  dElevation = helios.dElevation;
  float ElReach = ((dElevation * 204800 / 36) + (ElOffset * 1000) - 100000);  // Réducteur 1/80 = 80 * 200 * 128 = 2048000 / 360 = 5688,888 µsteps/degree
  float AzReach = ((dAzimuth * 128000 / 36) + (AzOffset * 1000) - 100000);  // Réducteur 1/50 = 50 * 200 * 128 = 1280000 / 360 = 3555,555 µsteps/degree
  if (ElReach <= 20000) {           // Limit minimum Elevation (5°)
    ElReach = 20000;
  }
  if (ElReach >= 500000) {          // Limit maximum Elevation (88°)
    ElReach = 500000;
  }
  if (AzReach < 20000) {           // Limit minimum Azimuth (56°)
    AzReach = 20000;
  }
  if (AzReach > 920000) {           // Limit maximum Azimuth (276°)
    AzReach = 920000;
  }
  if (dElevation < Ang)  {          // nuit
    Status = 0;
    AzReach = 909180;                // Az 253°
    ElReach = 364672;                //  El 81°
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(1, 0, 0));
      pixels.show();
    }
  }
  if (dElevation > Ang)  {         // jour
    Status = 1;
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 1, 0));
      pixels.show();
    }
  }
  if ((Clock.getSecond() == 0) || (Clock.getSecond() == 5) || (Clock.getSecond() == 10) || (Clock.getSecond() == 15) || (Clock.getSecond() == 20) || (Clock.getSecond() == 25) || (Clock.getSecond() == 30) || (Clock.getSecond() == 35) || (Clock.getSecond() == 40) || (Clock.getSecond() == 45) || (Clock.getSecond() == 50) || (Clock.getSecond() == 55)) {
    Serial1.print("A");
    Serial1.print(AzReach, 0);
    Serial1.println("");
    Serial1.print("E");
    Serial1.print(ElReach, 0);
    Serial1.println("");
    u8x8.clear();
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
    u8x8.setCursor(1, 0);
    if (Clock.getDoW() == 1 ) {
      u8x8.print("Dim ");
    }
    if (Clock.getDoW() == 2 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Lun ");
    }
    if (Clock.getDoW() == 3 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Mar ");
    }
    if (Clock.getDoW() == 4 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Mer ");
    }
    if (Clock.getDoW() == 5 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Jeu ");
    }
    if (Clock.getDoW() == 6 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Ven ");
    }
    if (Clock.getDoW() == 7 ) {
      u8x8.setCursor(0, 0);
      u8x8.print("Sam ");
    }
    u8x8.print(Clock.getDate(), DEC);
    u8x8.print(" ");
    if (Clock.getMonth(Century) == 1 ) {
      u8x8.print("Jan ");
    }
    if (Clock.getMonth(Century) == 2 ) {
      u8x8.print("Fev ");
    }
    if (Clock.getMonth(Century) == 3 ) {
      u8x8.print("Mar ");
    }
    if (Clock.getMonth(Century) == 4 ) {
      u8x8.print("Avr ");
    }
    if (Clock.getMonth(Century) == 5 ) {
      u8x8.print("Mai ");
    }
    if (Clock.getMonth(Century) == 6 ) {
      u8x8.print("Jun ");
    }
    if (Clock.getMonth(Century) == 7 ) {
      u8x8.print("Jul ");
    }
    if (Clock.getMonth(Century) == 8 ) {
      u8x8.print("Aou ");
    }
    if (Clock.getMonth(Century) == 9 ) {
      u8x8.print("Sep ");
    }
    if (Clock.getMonth(Century) == 10 ) {
      u8x8.print("Oct ");
    }
    if (Clock.getMonth(Century) == 11 ) {
      u8x8.print("Nov ");
    }
    if (Clock.getMonth(Century) == 12 ) {
      u8x8.print("Dec ");
    }
    u8x8.print("20");
    u8x8.print(Clock.getYear(), DEC);
    u8x8.setCursor(1, 1);
    if (Clock.getHour(h12, PM) <= 9)  {
      u8x8.print("0");
    }
    u8x8.print(Clock.getHour(h12, PM)); // GMT+2
    u8x8.print("h");
    if (Clock.getMinute() <= 9)  {
      u8x8.print("0");
    }
    u8x8.print(Clock.getMinute());
    u8x8.print("m");
    if (Clock.getSecond() <= 9)  {
      u8x8.print("0");
    }
    u8x8.print(Clock.getSecond());
    u8x8.print("s UT");
    u8x8.setCursor(2, 2);
    u8x8.print("T ");
    u8x8.print(Clock.getTemperature(), 1);
    if (Status == 0) {
      u8x8.print("C Nuit");
    }
    if (Status == 1) {
      u8x8.print("C Jour");
    }
    u8x8.setCursor(1, 3);
    u8x8.print("Ang : ");
    u8x8.print(Ang);
    u8x8.setCursor(0, 4);
    u8x8.print("Az :");
    u8x8.print(dAzimuth, 2);
    u8x8.setCursor(0, 5);
    u8x8.print("El :");
    u8x8.print(dElevation, 2);
    u8x8.setCursor(0, 6);
    u8x8.print("AzReach :");
    u8x8.print(AzReach, 0);
    u8x8.setCursor(0, 7);
    u8x8.print("ElReach :");
    u8x8.print(ElReach, 0);
    u8x8.refreshDisplay();
    delay(1000);
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 1, 10));
      pixels.show();
    }
    delay(10);
  }
}
void loop() {
  track();
  checkcom();
}
