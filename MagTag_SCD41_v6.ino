// Magtag 296 x 128 display resolution
// 543uA in sleep mode
#include <Adafruit_ThinkInk.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, -1, -1);

SensirionI2CScd4x scd4x;

uint16_t co2;
float    temperature, humidity;

#define maximumReadings 25  // 290 is the maximum number of readings that can be stored in the available space

RTC_DATA_ATTR int Readings[maximumReadings];
RTC_DATA_ATTR int readingCnt;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, so set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2022, 12, 18, 21, 11, 55)); // Add 45 secs for compile and upload delay start at 0-secs
  }
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);               // Switch On-board LED Off
  pinMode(NEOPIXEL_POWER, OUTPUT);     // Switch NeoPixel driver off
  pinMode(SPEAKER_SHUTDOWN, OUTPUT);   // Switch Speaker driver off
  //display.begin(THINKINK_MONO);        // B/W setting
  display.begin(THINKINK_GRAYSCALE4);  // 4 grey levels setting EPD_BLACK, EPD_LIGHT, EPD_DARK
  display_sensor();
  display.display();
  int cnt = 0;
  while (EPD_BUSY) {
    delay(100);
    cnt++;
    if (cnt > 35) break;               // Wait for display to update for 3.5-Secs
  }
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);        // hardware power down mode
  digitalWrite(SPEAKER_SHUTDOWN, LOW); // off
  digitalWrite(NEOPIXEL_POWER, HIGH);  // off
  long long SleepDelay = (60 * 60 + 20) * 1000000LL;
  esp_sleep_enable_timer_wakeup(SleepDelay);
  esp_deep_sleep_start();
}

void loop() {
}

void display_sensor() {
  Serial.println("Waiting for measurement...");
  scd4x.begin(Wire);
  scd4x.startPeriodicMeasurement();// stop potentially previously started measurement
  delay(6000);
  scd4x.readMeasurement(co2, temperature, humidity); // Read Measurement
  scd4x.stopPeriodicMeasurement();
  display.clearBuffer();  // Clear screen
  display.drawLine(170, display.height() / 2, display.width(), display.height() / 2, EPD_BLACK);
  display.drawLine(170, display.height() / 2, 170, display.height(), EPD_BLACK);
  display.drawLine(170, 96, display.width(), 96, EPD_BLACK);
  display.setTextColor(EPD_BLACK);
  DrawString(80, 0,    GetTimeDate(), 1);
  DrawString(10, 90,   String(co2), 5);
  DrawString(190, 70,  String(temperature, 1), 3);
  DrawString(200, 105, String(humidity, 0), 3);
  DrawString(140, 90,  "co2", 1);
  DrawString(140, 117, "ppm", 1);
  DrawString(270, 70,  "TEMP", 1);
  DrawString(270, 85,  "'C", 1);
  DrawString(255, 105, "RH", 1);
  DrawString(255, 120, "%", 1 );
  UpdateSensorReadings();
  DrawIcon(240, 30, 25, co2);
  DrawBattery(0, 0);
  DrawGraph(  5, 11, 150,    45,      0,     2500,  "Co2", Readings,    24,       true,      true);
  //DrawGraph(x, y,  gwidth, gheight, Y1Min, Y1Max, title, DataArray[], readings, auto_scale, barchart_mode)
}

String GetTimeDate() {
  DateTime now = rtc.now();
  String TimeDate = "";
  TimeDate  = (now.hour()   < 10 ? "0" : "") + String(now.hour())   + ":";
  TimeDate += (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":";
  TimeDate += (now.second() < 10 ? "0" : "") + String(now.second()) + "  (";
  TimeDate += (now.day()    < 10 ? "0" : "") + String(now.day())    + "/" + (now.month() < 10 ? "0" : "") + String(now.month()) + "/";
  TimeDate += (now.year()   < 10 ? "0" : "") + String(now.year(), DEC) + ")";
  return TimeDate;
}

// <  600         Very Happy
// >= 600  < 1000 Happy
// >= 1000 < 1500 Glum
// >= 1500 < 2000 Sad
// >= 2000 < 2500 Very Sad
// >= 2500        Grim

void DrawIcon(int x, int y, int diameter, int co2) {
  if (co2 < 600) {                // Very Happy
    DrawFeatures(x, y, diameter, true);
    DrawHappy(x, y, diameter);
    DrawGlum(x, y, diameter);
  }
  if (co2 >= 600 && co2 < 1000) { // Happy
    DrawFeatures(x, y, diameter, true);
    DrawHappy(x, y, diameter);
  }
  if (co2 >= 1000 && co2 < 1500) { // Glum
    DrawFeatures(x, y, diameter, false);
    DrawGlum(x, y, diameter);
  }
  if (co2 >= 1500 && co2 < 2000) { // Sad
    DrawFeatures(x, y, diameter, false);
    DrawSad(x, y, diameter);
  }
  if (co2 >= 2000 && co2 < 2500) { // Sad
    DrawFeatures(x, y, diameter, false);
    DrawSad(x, y, diameter);
    DrawGlum(x, y, diameter);
  }
  if (co2 >= 2500)               { // Grim
    DrawFeatures(x, y, diameter, false);
    DrawGrim(x, y, diameter);
  }
}

void DrawFeatures(int x, int y, int diameter, bool light) {
  display.drawCircle(x, y, diameter, EPD_BLACK);
  display.drawCircle(x, y, diameter + 1, EPD_BLACK);
  if (light) {
    display.drawCircle(x - diameter / 3, y - diameter / 3, diameter / 6, EPD_BLACK);
    display.drawCircle(x + diameter / 3, y - diameter / 3, diameter / 6, EPD_BLACK);
  }
  else
  {
    display.fillCircle(x - diameter / 3, y - diameter / 3, diameter / 6, EPD_BLACK);
    display.fillCircle(x + diameter / 3, y - diameter / 3, diameter / 6, EPD_BLACK);
  }
}

void DrawHappy(int x, int y, int diameter) {
  y = y + diameter / 3;
  float start_angle = 0.81, end_angle = 2.230;
  int r = 20;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), EPD_BLACK);
    display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), EPD_BLACK);
  }
}

void  DrawGlum(int x, int y, int diameter) {
  display.drawLine(x - diameter / 2, y + diameter / 2,     x + diameter / 2, y + diameter / 2, EPD_BLACK);
  display.drawLine(x - diameter / 2, y + diameter / 2 + 1, x + diameter / 2, y + diameter / 2 + 1, EPD_BLACK);
}

void DrawSad(int x, int y, int diameter) {
  y = y + diameter * 1.5;
  float start_angle = 4.1, end_angle = 5.5;
  int r = 20;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), EPD_BLACK);
    display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), EPD_BLACK);
  }
}

void DrawGrim(int x, int y, int diameter) {
  DrawString(x - 5, y + 2, "x", 2);
}

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  float voltage = analogRead(4) / 4096.0 * 2.6;
  if (voltage > 1 ) { // Only display if there is a valid reading
    Serial.println("Voltage = " + String(voltage));
    DrawString(x + 5, y, String(voltage, 1) + "v", 1);
  }
}

void UpdateSensorReadings() {
  for (int i = 1; i < 24; i++) {
    Readings[i] = Readings[i + 1];
  }
  Readings[24] = (int)co2;
}

void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, int DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up in units of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
#define Spacer       2      // Space between barchart elements
  int maxYscale = -10000;
  int minYscale =  10000;
  float last_x, last_y;
  float x2, y2;
  int   xSpacing;
  if (auto_scale == true) {
    for (int i = 1; i <= readings; i++ ) { // Adjusted graph range
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale + 0.5);
  }
  // Graph the data
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  display.drawRect(x_pos + 2, y_pos - 2, gwidth + 2, gheight + y_minor_axis, EPD_DARK);
  // Draw the data
  xSpacing = (gwidth - readings * Spacer) / readings; 
  for (int gx = 1; gx <= readings; gx++) {
    x2 = x_pos + gx * (xSpacing + Spacer); 
    y2 = y_pos + ((Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1);
    if (barchart_mode) {
      display.fillRect(x2 + 2, y2, xSpacing, y_pos + gheight - y2 + 3, EPD_BLACK);
    } else {
      display.drawLine(last_x, last_y, x2, y2, EPD_BLACK);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 24
  for (int spacing = 0; spacing < y_minor_axis; spacing++) {
    for (int d = 1; d <= number_of_dashes; d++) { // Draw dashed graph grid lines
      display.drawFastHLine(x_pos + d * (xSpacing + Spacer), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), EPD_DARK);
    }
  }
  DrawString(x_pos + gwidth / 2, y_pos + gheight + 5, "24Hr", 1);
  DrawString(x_pos + gwidth + 5, y_pos,               String(maxYscale), 1);
  DrawString(x_pos + gwidth + 5, y_pos + gheight - 5, String(minYscale), 1);
}

void DrawString(int x, int y, String textLine, int Size) {
  display.setTextSize(Size);
  display.setCursor(x, y);
  display.print(textLine);
}

// 256 Lines of code

//https://www.google.com/search?q=good+poor+bad+icons&oq=good+poor+bad+icons&aqs=edge..69i57j0i546l3.9048j0j1&sourceid=chrome&ie=UTF-8#imgrc=GkOL9H3gJ_XKoM&imgdii=a259u5mwe9r1vM
