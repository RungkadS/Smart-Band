#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Inisialisasi objek SoftwareSerial untuk komunikasi dengan modul GPS
int RXPin = 3;
int TXPin = 2;

int GPSBaud = 9600;

// Inisialisasi objek TinyGPS++ untuk memproses data GPS
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);

void setup() {
  Serial.begin(9600); // Mulai komunikasi serial dengan kecepatan 9600 baud
  gpsSerial.begin(9600); // Mulai komunikasi serial dengan modul GPS
}

void loop() {
  // Baca data dari modul GPS
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read()))
      displayInfo();
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS Detected");
  } 
}

void displayInfo()
{
  if (gps.location.isValid()) 
  {
  // Jika mendapatkan data lokasi yang valid, tampilkan ke Serial Monitor
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);     
  } else 
  {
    // Jika tidak mendapatkan data lokasi yang valid, tampilkan pesan
    Serial.println("Lokasi tidak valid");
  }
  Serial.println():
  delay(1000);
}
