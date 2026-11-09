// Teste do módulo relé com ESP32
// Relé: S -> D32 | + -> VIN | - -> GND
// O relé deve dar um "clique" a cada 2 segundos

#define RELAY_PIN 32

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // começa desligado
  Serial.println("Teste do rele iniciado");
}

void loop() {
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Rele LIGADO");
  delay(2000);

  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Rele DESLIGADO");
  delay(2000);
}
