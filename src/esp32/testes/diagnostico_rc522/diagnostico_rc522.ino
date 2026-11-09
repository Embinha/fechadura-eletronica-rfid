// Diagnóstico do RC522 com ESP32
// Monitor Serial em 115200

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5
#define RST_PIN 27

MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long ultimoAviso = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin();
  rfid.PCD_Init();
  delay(100);

  Serial.println("\n=== DIAGNOSTICO RC522 ===");
  rfid.PCD_DumpVersionToSerial();

  Serial.print("Autoteste do chip: ");
  bool ok = rfid.PCD_PerformSelfTest();
  Serial.println(ok ? "PASSOU" : "FALHOU");

  // O autoteste reinicia o chip, entao inicializa de novo
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);
  rfid.PCD_AntennaOn();

  Serial.print("Ganho da antena: 0x");
  Serial.println(rfid.PCD_GetAntennaGain(), HEX);
  Serial.println("Aproxime o cartao...");
}

void loop() {
  // A cada 3 segundos mostra que o programa esta rodando
  if (millis() - ultimoAviso > 3000) {
    Serial.println("...procurando cartao");
    ultimoAviso = millis();
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("Cartao detectado, mas falhou ao ler o UID");
    return;
  }

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
