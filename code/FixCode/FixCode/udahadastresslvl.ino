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

  // Calculate the difference from the baseline value
  int diff = sensorValue - baselineValue;

  // Determine emotional response based on GSR sensor value
  String emotionalResponse;
  int stressLevel = 0;

  if (sensorValue > 210) {
    emotionalResponse = "normal";
  } else if (sensorValue <= 210 && sensorValue > 110) {
    emotionalResponse = "stressed";
  } else if (sensorValue >= 110 && sensorValue < 1) {
    emotionalResponse = "Highly Stressed";
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

  // Update the display every UPDATE_INTERVAL milliseconds
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    
    // Determine BPM category
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
    display.print("GSR Value: ");
    display.println(sensorValue);
    display.print("Difference: ");
    display.println(diff);
    display.print("Emotional Response: ");
    display.println(emotionalResponse);
    display.print("Stress Level: ");
    display.println(stressLevel);

    Serial.print("GSR Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Difference: ");
    Serial.print(diff);
    Serial.print(" | Emotional Response: ");
    Serial.print(emotionalResponse);
    Serial.print(" | Stress Level: ");
    Serial.println(stressLevel);

    if (irValue < 50000)
      display.println("No finger?");

    display.display();
  }
}
