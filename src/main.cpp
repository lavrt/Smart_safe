#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN  22
#define SS_PIN   21

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("RFID RC522 Test Starting...");
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Вывод информации о версии прошивки
  Serial.print("MFRC522 Firmware Version: 0x");
  Serial.println(mfrc522.PCD_ReadRegister(mfrc522.VersionReg), HEX);
  
  // Проверка подключения
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println("ERROR: MFRC522 not found!");
    while (true);
  }
  
  Serial.println("RFID Reader Ready!");
  Serial.println("Place your RFID card near the reader...");
}

void loop() {
  // Проверка новой карты
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  
  // Чтение карты
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.println("Card detected!");
  
  // Вывод UID карты
  Serial.print("UID length: ");
  Serial.print(mfrc522.uid.size);
  Serial.print(" bytes | UID: ");
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  // ИСПРАВЛЕННАЯ ЧАСТЬ - правильное получение типа карты
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print("PICC type: ");
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  
  Serial.println("-----------------------------");
  
  // Остановка чтения
  mfrc522.PICC_HaltA();
  
  delay(1000);
}
