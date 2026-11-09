// Teste do leitor RC522 com ESP32
// Aproxime o cartão: o UID aparece no Serial Monitor (115200)

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5   // SDA do RC522
#define RST_PIN 27  // RST do RC522

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();          // SCK=18, MISO=19, MOSI=23 (padrão do ESP32)
  rfid.PCD_Init();
  delay(100);

  // Confere se o ESP32 está conversando com o RC522
  byte versao = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  if (versao == 0x00 || versao == 0xFF) {
    Serial.println("ERRO: RC522 nao encontrado. Confira os fios e a solda.");
  } else {
    Serial.print("RC522 OK (versao 0x");
    Serial.print(versao, HEX);
    Serial.println("). Aproxime o cartao...");
  }
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
