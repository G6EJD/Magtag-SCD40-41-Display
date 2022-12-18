#include <Adafruit_ThinkInk.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, -1, -1);

SensirionI2CScd4x scd4x;

uint16_t co2;
float    temperature, humidity;
  
void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  Wire.begin();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);               // On-board LED Off
  pinMode(NEOPIXEL_POWER, OUTPUT);     // 
  pinMode(SPEAKER_SHUTDOWN, OUTPUT);
  display.begin(THINKINK_MONO);
  display.setTextSize(2);
  display_sensor();
  display.display();
  delay(1500);
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);        // hardware power down mode
  digitalWrite(SPEAKER_SHUTDOWN, LOW); // off
  digitalWrite(NEOPIXEL_POWER, HIGH);  // off
  long SleepDelay = 60 * 60 * 1000000;
  esp_sleep_enable_timer_wakeup(SleepDelay);
  esp_deep_sleep_start();
}

void loop() {
}

void display_sensor() {
  Serial.println("Waiting for measurement...");
  // stop potentially previously started measurement
  scd4x.begin(Wire);
  scd4x.startPeriodicMeasurement();
  delay(5000);
  scd4x.readMeasurement(co2, temperature, humidity); // Read Measurement
  scd4x.stopPeriodicMeasurement();
  display.clearBuffer();
  display.setTextSize(2);
  display.setTextColor(EPD_BLACK);
  display.setCursor(5, 20); display.print("Co2 :" + String(co2) + "ppm");
  display.setCursor(5, 50); display.print("Temp:" + String(temperature, 1) + "'C");
  display.setCursor(5, 80); display.print("Humi:" + String(humidity, 0) + "%");
  String Level = "Good";
  if (co2 >= 1000 && co2 < 2000) Level = "Poor";
  if (co2 >= 2000 && co2 < 5000) Level = "V.Bad";
  display.setCursor(160, 45);
  display.setTextSize(4);
  display.print(Level);
}
