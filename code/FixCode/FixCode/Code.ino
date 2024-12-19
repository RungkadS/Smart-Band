//SDA=21 for oled and max
//SCL=22 for oled and max
//SIG=SVP for gsr
#define BLYNK_TEMPLATE_ID "TMPL6kkXLOZeK"
#define BLYNK_TEMPLATE_NAME "gps"
#define BLYNK_AUTH_TOKEN "ofbNEDM0wqBEURSL6n6Mwoin_AQQUdsS"

const int GSR_Pin = A0;  // Pin analog untuk input dari sensor GSR
int sensorValue = 0;     // Variabel untuk menyimpan nilai sensor
int baselineValue = 0;   // Nilai baseline untuk perbandingan
int numReadings = 50;    // Jumlah pembacaan untuk kalibrasi
int readings[50];        // Array untuk menyimpan pembacaan kalibrasi

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h> // Untuk ESP32
#include <BlynkSimpleEsp32.h> // Untuk ESP32
#include <TinyGPS++.h>

const char* ssid = "wahyukontol";
const char* password = "0987654321";
char auth[] = BLYNK_AUTH_TOKEN; // Auth Token dari Blynk

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define I2C_ADDRESS   0x3C // Alamat I2C untuk OLED, sesuaikan jika perlu

MAX30105 particleSensor;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, OLED_RESET);
TinyGPSPlus gps;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
unsigned long lastUpdate = 0; // Time of last display update
const unsigned long UPDATE_INTERVAL = 2000; // Interval for display update in milliseconds

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX pin = 16, TX pin = 17

  // Inisialisasi Blynk
  Blynk.begin(auth, ssid, password);

  Serial.println("Calibrating...");
  for (int i = 0; i < numReadings; i++) {
    readings[i] = analogRead(GSR_Pin);
    delay(50);
  }

  // Hitung rata-rata nilai baseline
  for (int i = 0; i < numReadings; i++) {
    baselineValue += readings[i];
  }
  baselineValue /= numReadings;
  Serial.print("Baseline Value: ");
  Serial.println(baselineValue);
  delay(2000);

  Serial.println("Initializing...");

  // Initialize OLED display
  if (!display.begin(I2C_ADDRESS, OLED_RESET)) {
    Serial.println(F("OLED allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  sensorValue = analogRead(GSR_Pin);

  // Hitung perbedaan dari nilai baseline
  
  int diff = sensorValue - baselineValue;
  
  String emotionalResponse;
  int stressLevel = 0;

  if (sensorValue > 210) {
    emotionalResponse = "normal";
  } else if (sensorValue <= 210 && sensorValue > 110) {
    emotionalResponse = "Stressed";
  } else if (sensorValue <= 110 && sensorValue > 1) {
    emotionalResponse = "HIghly Stressed";
  } else {
    emotionalResponse = "no finger";
  }

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Display on OLED every 2 seconds
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("IR: ");
    display.println(irValue);
    display.print("BPM: ");
    display.println(beatsPerMinute);
    display.print("Avg BPM: ");
    display.println(beatAvg);

    Serial.print("GSR Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Difference: ");
    Serial.print(diff);
    Serial.print(" | Emotional Response: ");
    Serial.println(emotionalResponse);

    display.print("Stress Level: ");
    display.println(stressLevel);

    // Print to Serial
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);
    Serial.println(stressLevel);

    if (irValue < 50000)
      Serial.print(" No finger?");
      
    Serial.println();

    // Kirim data ke Blynk
    Blynk.virtualWrite(V0, irValue); // Kirim nilai IR ke Virtual Pin V0
    Blynk.virtualWrite(V1, beatsPerMinute); // Kirim BPM ke Virtual Pin V1
    Blynk.virtualWrite(V2, beatAvg); // Kirim rata-rata BPM ke Virtual Pin V2
    Blynk.virtualWrite(V3, stressLevel);
    Blynk.virtualWrite(V4, emotionalResponse);
    Blynk.virtualWrite(V5, sensorValue);
    //Blynk.virtualWrite(V3, emotionalResponse); // Kirim respon emosional ke Virtual Pin V3

String bpmCategory;
    if (beatsPerMinute < 70) {
      bpmCategory = "Normal";
    } else if (beatsPerMinute >= 70 && beatsPerMinute <= 80) {
      bpmCategory = "Sedang";
    } else {
      bpmCategory = "Tinggi";
    }

    // Determine stress level based on the algorithm
    if (bpmCategory == "Normal") {
      stressLevel = 0;
    } else if (bpmCategory == "Sedang") {
      if (emotionalResponse == "normal") {
        stressLevel = 0;
      } else if (emotionalResponse == "stressed") {
        stressLevel = 1;
      } else if (emotionalResponse == "Highly Stressed") {
        stressLevel = 2;
      }
    } else if (bpmCategory == "Tinggi") {
      if (emotionalResponse == "normal") {
        stressLevel = 2;
      } else if (emotionalResponse == "stressed") {
        stressLevel = 3;
      } else if (emotionalResponse == "Highly Stressed") {
        stressLevel = 3;
      }
    }

    if (irValue < 50000)
      display.println("No finger?");

    // Add the code to display GPS date and time
    if (gps.date.isValid() && gps.time.isValid()) {
      display.setCursor(0, 50);
      display.print("Date: ");
      display.print(gps.date.year());
      display.print("-");
      display.print(gps.date.month());
      display.print("-");
      display.print(gps.date.day());
      display.setCursor(0, 60);
      display.print("Time: ");
      display.print(gps.time.hour());
      display.print(":");
      display.print(gps.time.minute());
      display.print(":");
      display.print(gps.time.second());
    } else {
      display.setCursor(0, 50);
      display.print("Date&Time: INVALID");
    }

    display.display();
  }

  // GPS data reading
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      // Jika data lokasi valid, tampilkan informasi
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat(), 6); // Menampilkan 6 digit desimal
        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng(), 6); // Menampilkan 6 digit desimal
        Serial.print(F("- altitude: "));
        if (gps.altitude.isValid())
          Serial.println(gps.altitude.meters());
        else
          Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }

      // Menampilkan kecepatan jika data valid
      Serial.print(F("- speed: "));
      if (gps.speed.isValid()) {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      } else {
        Serial.println(F("INVALID"));
      }

      // Menampilkan tanggal dan waktu GPS jika valid
      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.println();     
    }
  }
}