#include <TinyGPS++.h>

#define GPS_BAUDRATE 9600  // The default baudrate of NEO-6M is 9600

TinyGPSPlus gps;  // the TinyGPS++ object

void setup() {
  Serial.begin(115200); // Ubah baud rate menjadi 115200 untuk debug
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, 16, 17); // RX pin = 16, TX pin = 17

  Serial.println(F("ESP32 - GPS module"));
}

void loop() {
  // Mengecek apakah sudah melebihi 5000 millis dan data GPS belum valid
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS data received: check wiring"));
  }

  // Membaca data dari Serial2 (GPS)
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
