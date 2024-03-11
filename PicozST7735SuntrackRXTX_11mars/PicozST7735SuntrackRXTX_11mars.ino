
#include <Arduino.h>
#include <Wire.h>
#include "PioSPI.h"
PioSPI spiBus(4, 3, 2, 5, SPI_MODE3, 10000000); //MOSI, MISO, SCK, CS, SPI_MODE, FREQUENCY
#include "at24c256.h"
at24c256 eeprom(0x50);
#include <Adafruit_GFX.h>            // Core graphics library
#include <Adafruit_ST7735.h>         // Hardware-specific library for ST7735
#define BK        3    //            BK
#define TFT_CS    5    //            CS
#define TFT_RST  10    //           RST   Or set to -1 and connect to Arduino RESET pin
#define TFT_DC    6    // 10         DC
#define TFT_MOSI  4    // Data out  SDA
#define TFT_SCLK  2    // Clock out SCL
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
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
unsigned int Angl;            // Angle minimum elevation
int Status = 0;               // Jour/NUit
char blop;
String input = "";

void setup() {
  Serial.begin(9600);   // usb
  Serial1.begin(9600);  // RX/TX commandes moteurs
  Serial2.begin(9600);  // Ble control
  pinMode(BK, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(1);
  digitalWrite(TFT_RST, LOW);
  delay(1);
  digitalWrite(TFT_RST, HIGH);
  delay(1);
  digitalWrite(BK, HIGH);
  spiBus.begin(); // wake up the SPI bus.
  spiBus.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));
  spiBus.endTransaction();
  Wire.setSDA(12);
  Wire.setSCL(13);
  Wire.setClock(100000);
  Wire.begin();
  eeprom.init();
  AzOffset = eeprom.read(30000);
  ElOffset = eeprom.read(30001);
  AzBack = eeprom.read(30002);
  ElBack = eeprom.read(30003);
  Angl = eeprom.read(30004);
  pixels.begin();
  pixels.clear();
  for (int i = 0; i < 1; i++) {
    pixels.setPixelColor(i, pixels.Color(1, 0, 0));
    pixels.show();
  }
  spiBus.begin(); // wake up the SPI bus.
  spiBus.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));
  spiBus.endTransaction();
  tft.initR(INITR_MINI160x80_PLUGIN);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 5);
  tft.setTextSize(1 );
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(1, 0);
  tft.print("PicoSunTracker");
  delay(2000);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(1, 20);
  tft.print("By Joy.");
  delay(2000);
  Serial1.print("I");
  Serial1.print(AzBack * 100);
  Serial1.println("");
  Serial1.print("J");
  Serial1.print(ElBack * 100);
  Serial1.println("");
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  digitalWrite(BK, LOW);
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
void track()  {
  ReadDS3231();
  helios.calcSunPos(Clock.getYear(), Clock.getMonth(Century), Clock.getDate(), Clock.getHour(h12, PM), Clock.getMinute(), Clock.getSecond(), 5.62879, 44.31636); // write here the decimal.
  dAzimuth = helios.dAzimuth;
  dElevation = helios.dElevation;
  float ElReach = ((dElevation * 204800 / 36) + (ElOffset * 1000) - 100000);  // Réducteur 1/80 = 80 * 200 * 128 = 2048000 / 360 = 5688,888 µsteps/degree
  float AzReach = ((dAzimuth * 128000 / 36) + (AzOffset * 1000) - 100000);  // Réducteur 1/50 = 50 * 200 * 128 = 1280000 / 360 = 3555,555 µsteps/degree

  if (ElReach <= 12000) {           // Limit minimum Elevation (5°)
    ElReach = 12000;
  }
  if (ElReach >= 500000) {          // Limit maximum Elevation (88°)
    ElReach = 500000;
  }
  if (AzReach < 20000) {           // Limit minimum Azimuth (56°)
    AzReach = 20000;
  }
  if (AzReach > 999999) {           // Limit maximum Azimuth (281°)
    AzReach = 999999;
  }
  if (dElevation < Angl)  {              // nuit
    Status = 0;
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(1, 0, 0));
      pixels.show();
    }
  }
  if (dElevation > Angl)  {              // jour
    Status = 1;
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 1, 0));
      pixels.show();
    }
  }
  if (Status == 0) {
    tft.setCursor(116, 20);
    tft.setTextSize(1 );
    tft.setTextColor(ST77XX_RED);
    tft.print("NUIT");
    AzReach = 909180;                // Az 253°
    ElReach = 364672;                //  El 81°
  }
  if (Status == 1) {
    tft.setCursor(116, 20);
    tft.setTextSize(1 );
    tft.setTextColor(ST77XX_GREEN);
    tft.print("JOUR");
  }
  if (Clock.getHour(h12, PM) == 0 ) {
    tft.fillRect(10, 0, 140, 8, ST77XX_BLACK);
  }
  if (Clock.getMinute() == 0 ) {
    tft.fillRect(16, 10, 14, 8, ST77XX_BLACK);
  }
  if (Clock.getSecond() == 0)  {
    tft.fillRect(36, 10, 14, 8, ST77XX_BLACK);
    tft.fillRect(90, 10, 26, 8, ST77XX_BLACK);
    tft.fillRect(114, 18, 30, 10, ST77XX_BLACK);
  }
  if ((Clock.getSecond() == 0) || (Clock.getSecond() == 5) || (Clock.getSecond() == 10) || (Clock.getSecond() == 15) || (Clock.getSecond() == 20) || (Clock.getSecond() == 25) || (Clock.getSecond() == 30) || (Clock.getSecond() == 35) || (Clock.getSecond() == 40) || (Clock.getSecond() == 45) || (Clock.getSecond() == 50) || (Clock.getSecond() == 55)) {
    pixels.clear();
    for (int i = 0; i < 1; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 1));
      pixels.show();
    }
    tft.fillRect(60, 40, 60, 18, ST77XX_BLACK);
    tft.setCursor(72, 40);
    tft.setTextColor(ST77XX_CYAN);
    tft.print(AzReach, 0);
    tft.setCursor(72, 50);
    tft.setTextColor(ST77XX_CYAN);
    tft.print(ElReach, 0);
    Serial1.print("A");
    Serial1.print(AzReach, 0);
    Serial1.println("");
    Serial1.print("E");
    Serial1.print(ElReach, 0);
    Serial1.println("");
  }
  if (Clock.getSecond() >= 0)  {
    tft.fillRect(56, 10, 14, 8, ST77XX_BLACK);
    tft.fillRect(20, 40, 40, 18, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 0);
    tft.setTextColor(ST77XX_WHITE);
    if (Clock.getDoW() == 1 ) {
      tft.print("Dimanche ");
    }
    if (Clock.getDoW() == 2 ) {
      tft.print("   Lundi ");
    }
    if (Clock.getDoW() == 3 ) {
      tft.print("   Mardi ");
    }
    if (Clock.getDoW() == 4 ) {
      tft.print("Mercredi ");
    }
    if (Clock.getDoW() == 5 ) {
      tft.print("   Jeudi ");
    }
    if (Clock.getDoW() == 6 ) {
      tft.print("Vendredi ");
    }
    if (Clock.getDoW() == 7 ) {
      tft.print("  Samedi ");
    }
    tft.print(Clock.getDate(), DEC);
    tft.print(" ");
    if (Clock.getMonth(Century) == 1 ) {
      tft.print("Janvier");
    }
    if (Clock.getMonth(Century) == 2 ) {
      tft.print("Fevrier");
    }
    if (Clock.getMonth(Century) == 3 ) {
      tft.print("Mars");
    }
    if (Clock.getMonth(Century) == 4 ) {
      tft.print("Avril");
    }
    if (Clock.getMonth(Century) == 5 ) {
      tft.print("Mai");
    }
    if (Clock.getMonth(Century) == 6 ) {
      tft.print("Juin");
    }
    if (Clock.getMonth(Century) == 7 ) {
      tft.print("Juillet");
    }
    if (Clock.getMonth(Century) == 8 ) {
      tft.print("Aout");
    }
    if (Clock.getMonth(Century) == 9 ) {
      tft.print("Septembre");
    }
    if (Clock.getMonth(Century) == 10 ) {
      tft.print("Octobre");
    }
    if (Clock.getMonth(Century) == 11 ) {
      tft.print("Novembre");
    }
    if (Clock.getMonth(Century) == 12 ) {
      tft.print("Decembre");
    }
    tft.print(" 20");
    tft.print(Clock.getYear(), DEC);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(20, 10);
    if (Clock.getHour(h12, PM) <= 9)  {
      tft.print("0");
    }
    tft.print(Clock.getHour(h12, PM)); // GMT+2
    tft.print(":");
    if (Clock.getMinute() <= 9)  {
      tft.print("0");
    }
    tft.print(Clock.getMinute());
    tft.print(":");
    if (Clock.getSecond() <= 9)  {
      tft.print("0");
    }
    tft.print(Clock.getSecond());
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(72, 10);
    tft.print("UT");
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(86, 10);
    tft.print(Clock.getTemperature(), 1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(114, 10);
    tft.print("C");
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(4, 20);
    tft.print("Lon: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" 5.62879");
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(86, 16);
    tft.print(".");
    tft.setCursor(86, 20);
    tft.print(" E");
    tft.setCursor(4, 30);
    tft.print("Lat: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print("44.31636");
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(86, 26);
    tft.print(".");
    tft.setCursor(86, 30);
    tft.print(" N");
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(4, 40);
    tft.print("Az:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(dAzimuth, 2);
    tft.setCursor(4, 50);      // RED
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("El:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(dElevation, 2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(4, 60);
    tft.print("An:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(Angl);
    tft.setCursor(34, 60);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Oa");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(AzOffset);
    tft.setCursor(66, 60);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("e");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(ElOffset);
    tft.setCursor(96, 60);
    tft.setTextColor(ST77XX_ORANGE);
    tft.print("Ba");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(AzBack);
    tft.setCursor(128, 60);
    tft.setTextColor(ST77XX_ORANGE);
    tft.print("e");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(ElBack);
  }
  delay(600);
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
      digitalWrite(BK, HIGH);
    }
  if (input.length() >= 1) {
    {
      blop = Serial2.read();
    }
    if (input == "Olon") {                   // Test commands
      digitalWrite(BK, HIGH);
      Serial2.println("");
      Serial2.println("LCD ON");
    }
    if (input == "Olof") {                   // Test commands
      digitalWrite(BK, LOW);
      Serial2.println("");
      Serial2.println("LCD OFF");
    }
    if (input == "Wind") {  // Position vent horizontale
      Serial2.println("Wind Position");
      Serial1.print("A909180");  // 255°  night position (west azimut)
      Serial1.println("");
      Serial1.print("E364672");  // 66 + 21 = 87°  night position (horizontal)
      Serial1.println("");
      delay(60000);
    }
    if (input == "Home") {              // Homing
      Serial2.println(" ");
      Serial2.println("Homing...");             // Homing back off.
      Serial1.print("I");
      Serial1.print(AzBack * 100);
      Serial1.println("");
      Serial1.print("J");
      Serial1.print(ElBack * 100);
      Serial1.println("");
      delay(60000);
    }
    if (input == "Rsto") {            // Reset Offset to 10
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
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
      Serial2.print("Angl = ");
      Angl = 5;
      eeprom.write(30004, 5);
      Serial2.println(Angl, DEC);
      Serial2.println("");
    }
    if (input == "Az+") {                   // Azimuth Offset +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("AzOffset = ");
      AzOffset = AzOffset + 1;
      Serial2.println(AzOffset, DEC);
      eeprom.write(30000, AzOffset);
    }
    if (input == "Az-") {                   // Azimuth Offset -
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("AzOffset = ");
      AzOffset = AzOffset - 1;
      Serial2.println(AzOffset, DEC);
      eeprom.write(30000, AzOffset);
    }
    if (input == "El+") {                   // Elevation Offset +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("ElOffset = ");
      ElOffset = ElOffset + 1;
      Serial2.println(ElOffset, DEC);
      eeprom.write(30001, ElOffset);
    }
    if (input == "El-") {                   // Elevation Offset -
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("ElOffset = ");
      ElOffset = ElOffset - 1;
      Serial2.println(ElOffset, DEC);
      eeprom.write(30001, ElOffset);
    }
    if (input == "AzB+") {                   // Azimuth Back +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("AzBack = ");
      AzBack = AzBack + 1;
      Serial2.println(AzBack, DEC);
      eeprom.write(30002, AzBack);
    }
    if (input == "AzB-") {                   // Azimuth Back -
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("AzBack = ");
      AzBack = AzBack - 1;
      Serial2.println(AzBack, DEC);
      eeprom.write(30002, AzBack);
    }
    if (input == "ElB+") {                   // Elevation Back  +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("ElBack= ");
      ElBack = ElBack + 1;
      Serial2.println(ElBack, DEC);
      eeprom.write(30003, ElBack);
      delay(10);
    }
    if (input == "ElB-") {                   // Elevation Offset -
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("ElBack= ");
      ElBack = ElBack - 1;
      Serial2.println(ElBack, DEC);
      eeprom.write(30003, ElBack);
    }
    if (input == "Ang+") {                   // Elevation Back  +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("Ang = ");
      Angl = Angl + 1;
      Serial2.println(Angl, DEC);
      eeprom.write(30004, Angl);
    }
    if (input == "Ang-") {                   // Elevation Back  +
      tft.fillRect(20, 58, 136, 10, ST77XX_BLACK);
      Serial2.println("");
      Serial2.print("Ang = ");
      Angl = Angl - 1;
      Serial2.println(Angl, DEC);
      eeprom.write(30004, Angl);
    }
    if (input == "Ctrl") {
      tft.fillRect(30, 70, 116, 10, ST77XX_BLUE);
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
      Serial2.print("Az : ");
      Serial2.print(" Offset ");
      Serial2.print(AzOffset);
      Serial2.print(" Back ");
      Serial2.println(AzBack);
      Serial2.print("El : ");
      Serial2.print(" Offset ");
      Serial2.print(ElOffset);
      Serial2.print(" Back ");
      Serial2.println(ElBack);
      Serial2.print("Ang : ");
      Serial2.println(Angl);
    }
    input = "";
  }
  tft.fillRect(30, 70, 116, 10, ST77XX_BLACK);
}
void loop()  {
  track();
  checkcom();
}
