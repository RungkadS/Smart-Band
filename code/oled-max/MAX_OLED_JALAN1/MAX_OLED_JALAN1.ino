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

    if (beatsPerMinute < 50) {
      display.println("Kurang");
    } else if (beatsPerMinute >= 50 && beatsPerMinute <= 80) {
      display.println("Cukup");
    } else {
      display.println("Baik");
    }
  

    if (irValue < 50000)
      display.println("No finger?");

    display.display();
  }
}