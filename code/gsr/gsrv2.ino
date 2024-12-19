#include <WiFi.h> // Untuk ESP32

const int GSR_Pin = A0;  // Pin analog untuk input dari sensor GSR
int sensorValue = 0;     // Variabel untuk menyimpan nilai sensor
int baselineValue = 0;   // Nilai baseline untuk perbandingan
int numReadings = 50;    // Jumlah pembacaan untuk kalibrasi
int readings[50];        // Array untuk menyimpan pembacaan kalibrasi

void setup() {
  Serial.begin(9600);

  // Kalibrasi sensor untuk mendapatkan baseline awal
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
}

void loop() {
  // Membaca nilai dari sensor GSR
  sensorValue = analogRead(GSR_Pin);

  // Hitung perbedaan dari nilai baseline
  int diff = sensorValue - baselineValue;

  // Kategori respon emosional
  String emotionalResponse;
  if (sensorValue > 300) {
    emotionalResponse = "normal";
  } else if (sensorValue <= 300 && sensorValue > 100) {
    emotionalResponse = "Stressed";
  } else if (sensorValue >= 100 && sensorValue < 30) {
    emotionalResponse = "HIghly Stressed";
  } else {
    emotionalResponse = "no finger";
  }

  // Mengirimkan nilai sensor dan respon emosional ke Serial Monitor
  Serial.print("GSR Value: ");
  Serial.print(sensorValue);
  Serial.print(" | Difference: ");
  Serial.print(diff);
  Serial.print(" | Emotional Response: ");
  Serial.println(emotionalResponse);

  // Tunggu 100ms sebelum membaca nilai berikutnya
  delay(100);
}
