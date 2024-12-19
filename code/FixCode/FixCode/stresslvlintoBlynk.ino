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
//#include <ESP8266WiFi.h> // Untuk ESP8266
#include <BlynkSimpleEsp32.h> // Untuk ESP32
//#include <BlynkSimpleEsp8266.h> // Untuk ESP8266

const char* ssid = "hospot";
const char* password = "18042003";
char auth[] = BLYNK_AUTH_TOKEN; // Auth Token dari Blynk


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define I2C_ADDRESS   0x3C // Alamat I2C untuk OLED, sesuaikan jika perlu

MAX30105 particleSensor;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, OLED_RESET);

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
  if (sensorValue <= 400 && sensorValue > 200) {
    emotionalResponse = "normal";
  } else if (sensorValue <= 200 && sensorValue > 80) {
    emotionalResponse = "Stressed";
  } else if (sensorValue <= 80 && sensorValue > 1) {
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
    display.print("Stress Level: ");
    display.println(stressLevel);

    Serial.print("GSR Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Difference: ");
    Serial.print(diff);
    Serial.print(" | Emotional Response: ");
    Serial.println(emotionalResponse);

    // Print to Serial
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);
    

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
    if (beatsPerMinute < 80) {
      display.println("normal");
    } else if (beatsPerMinute >= 80 && beatsPerMinute <= 110) {
      display.println("sedang");
    } else {
      display.println("tinggi");
    }


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

    display.display();
  }
}