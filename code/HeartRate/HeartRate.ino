// Bylnk IoT Coding
#define BLYNK_TEMPLATE_ID "TMPL63Z7zbgiH"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "LdPzn5vwhNI7Bu2RGrXLnshm4jdYqF5R"

// Libraries yang dipakai
#include "DFRobot_BloodOxygen_S.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wifi connection
char ssid[] = "Akutau";
char pass[] = "sayajuga1";

//Definisi jalur I2C 
#define LCD_ADDR 0x27 // Jalur I2C LCD 
#define BUZZER_PIN 13  // pin di GPIO 13 atau D7
#define BUZZER_DURATION 2000  // Duration to activate the buzzer in milliseconds (2 seconds)
#define I2C_COMMUNICATION  // Use I2C for communication

#ifdef I2C_COMMUNICATION
#define I2C_ADDRESS 0x57
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS);
#else
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
SoftwareSerial mySerial(4, 5);  // Jalur I2C D1 dan D2 (GPI0 4 dan GPIO 5)
DFRobot_BloodOxygen_S_SoftWareUart MAX30102(&mySerial, 115200);
#else
DFRobot_BloodOxygen_S_HardWareUart MAX30102(&Serial1, 115200);
#endif
#endif

// Kalibrasi alar
const float SPO2_CALIBRATION_FACTOR = 1;    // Adjust as needed
const float HEART_RATE_CALIBRATION_FACTOR = 0.94;  // Adjust as needed

// isiasi nilai sensor awal
float spo2Value = 0;
float heartRateValue = 0;

//aktifkan jalur IoT dan inisiasi alat yang digunakan
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

void activateBuzzer();

// Inisiasi setup
void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);

  // Initialize MAX30102 sensor
  while (false == MAX30102.begin()) {
    Serial.println("MAX30102 init fail!");
    delay(1000); // 1 second
  }
  Serial.println("MAX30102 init success!");
  Serial.println("Start measuring...");

  MAX30102.sensorStartCollect();


  delay(2000);  // 2 seconds

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("KEL 6");
  lcd.setCursor(0, 1);
  lcd.print("Nadi & Oksigen");

  // Initialize buzzer 
  pinMode(BUZZER_PIN, OUTPUT);
}

// Coding looping
void loop() {
  Blynk.run();
  delay(1500);

  // Coding untuk sensor MAX30102 DF ROBOT
  MAX30102.getHeartbeatSPO2();
  spo2Value = MAX30102._sHeartbeatSPO2.SPO2 * SPO2_CALIBRATION_FACTOR;
  heartRateValue = MAX30102._sHeartbeatSPO2.Heartbeat * HEART_RATE_CALIBRATION_FACTOR;
  float temperatureValue = MAX30102.getTemperature_C();

  // Display denyut nadi dan kadar oksigen di LCD
  lcd.setCursor(0, 0);
  lcd.print("nadi: ");
  lcd.print(heartRateValue);
  lcd.print(" BPM ");

  lcd.setCursor(0, 1);
  lcd.print("Oksigen:");
  lcd.print(spo2Value);
  lcd.print(" %");

  // Display denyut nadi, kadar oksigen, dan suhu board di serial monitor
  Serial.print("SPO2 is: ");
  Serial.print(spo2Value);
  Serial.println("%");
  Serial.print("Heart rate is: ");
  Serial.print(heartRateValue);
  Serial.println(" BPM");
  Serial.print("Board temperature is: ");
  Serial.print(temperatureValue);
  Serial.println(" ℃");

   // Pengiriman data ke IoT Bylnk
  Blynk.virtualWrite(V1, heartRateValue);
  Blynk.virtualWrite(V2, spo2Value);

  // Pengkodisian sensor (if dan else)
  if (spo2Value <= 0 || heartRateValue <= 0) {
    Serial.println("No hand/finger detected. Restarting detection and recalibration.");
    lcd.clear();
    lcd.print("Tidak terdeteksi");
    lcd.setCursor(0, 1);
    lcd.print("letakkan jari");
    Blynk.logEvent("tidak_deteksi","tidak deteksi tidak terdeteksi harap masukkan jari anda!!");
    delay(1700);

  } else {
    // Pengkodisian jika mendeteksi jari untuk denyut nadi (if dan else)
    if (heartRateValue <= 60) {
      Serial.println("Warning: Dangerously low heart rate. Waspada Bahaya!");
      activateBuzzer();
      Blynk.logEvent("kesehatan_sakit","bahaya kondisi anda tidak sehat");
    } else if (heartRateValue >= 120) {
      Serial.println("Warning: Dangerously high heart rate. Waspada Bahaya!");
      activateBuzzer();
      Blynk.logEvent("kesehatan_sakit","bahaya kondisi anda tidak sehat");
    } else {
      Serial.println("Heart rate is normal.");
      delay(1000); 
    }

    // Pengkodisian jika mendeteksi jari untuk kadar oksigen (if dan else)
    if (spo2Value < 90) {
      Serial.println("Warning: Low blood oxygen saturation. Kadar oksigen rendah!");
      Blynk.logEvent("kesehatan_sakit","bahaya kondisi anda tidak sehat");
      activateBuzzer();
    } else {
      Serial.println("Blood oxygen saturation is normal.");
    }
    }
  }

// inisiasi Buzzer dengan digitalwrite
void activateBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(BUZZER_DURATION);
  digitalWrite(BUZZER_PIN, LOW);
}