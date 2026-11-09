  // Fechadura eletrônica com RFID + servidor HTTP (ESP32)
// Ligação: RC522 SDA=D5 SCK=D18 MOSI=D23 MISO=D19 RST=D27 | Relé S=D32

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <time.h>

// ===== Wi-Fi: troque pelo nome e senha da rede (rede aberta: senha "") =====
const char* SSID  = "NOME_DA_REDE";   // troque pelo nome da sua rede Wi-Fi
const char* SENHA = "SENHA_DA_REDE";  // troque pela senha da sua rede Wi-Fi

// ===== Login da pagina web (HTTP Basic Auth) =====
const char* WEB_USUARIO = "admin";
const char* WEB_SENHA   = "1234";

// ===== Pinos e configuração =====
#define SS_PIN        5
#define RST_PIN       27
#define RELAY_PIN     32
#define TEMPO_ABERTA  3000   // ms que a fechadura fica destravada
#define MAX_CARTOES   20  
#define MAX_LOG       30

MFRC522 rfid(SS_PIN, RST_PIN);
WebServer server(80);       // servidor HTTP na porta 80 (TCP)
Preferences prefs;          // memória flash: a lista sobrevive ao desligar

String cartoes[MAX_CARTOES];   // UIDs autorizados
String nomes[MAX_CARTOES];     // nome do dono de cada cartão
int totalCartoes = 0;

String logAcessos[MAX_LOG];    // mais recente primeiro
int totalLog = 0;

String ultimoUID = "";         // último cartão lido (para cadastrar)
bool portaAberta = false;
unsigned long abriuEm = 0;

// ---------- Utilitários ----------
String horaAtual() {
  struct tm t;
  if (getLocalTime(&t, 10)) {
    char buf[20];
    strftime(buf, sizeof(buf), "%d/%m %H:%M:%S", &t);
    return String(buf);
  }
  return "+" + String(millis() / 1000) + "s";   // sem internet: tempo desde que ligou
}

void registrar(String evento) {
  for (int i = min(totalLog, MAX_LOG - 1); i > 0; i--) logAcessos[i] = logAcessos[i - 1];
  logAcessos[0] = horaAtual() + " - " + evento;
  if (totalLog < MAX_LOG) totalLog++;
  Serial.println(logAcessos[0]);
}

// Evita que um nome digitado injete HTML na página
String escapar(String s) {
  s.replace("&", "&amp;"); s.replace("<", "&lt;"); s.replace(">", "&gt;"); s.replace("\"", "&quot;");
  return s;
}

int buscarCartao(String uid) {
  for (int i = 0; i < totalCartoes; i++) if (cartoes[i] == uid) return i;
  return -1;
}

// ---------- Memória flash ----------
void salvarCartoes() {
  prefs.begin("fechadura", false);
  prefs.clear();
  prefs.putInt("total", totalCartoes);
  for (int i = 0; i < totalCartoes; i++) {
    prefs.putString(("u" + String(i)).c_str(), cartoes[i]);
    prefs.putString(("n" + String(i)).c_str(), nomes[i]);
  }
  prefs.end();
}

void carregarCartoes() {
  prefs.begin("fechadura", true);
  totalCartoes = prefs.getInt("total", 0);
  for (int i = 0; i < totalCartoes; i++) {
    cartoes[i] = prefs.getString(("u" + String(i)).c_str(), "");
    nomes[i]   = prefs.getString(("n" + String(i)).c_str(), "");
  }
  prefs.end();
}

// ---------- Fechadura ----------
void abrirPorta(String origem) {
  digitalWrite(RELAY_PIN, HIGH);
  portaAberta = true;
  abriuEm = millis();
  registrar("LIBERADO - " + origem);
}

void lerCartao() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  ultimoUID = uid;

  int i = buscarCartao(uid);
  if (i >= 0) abrirPorta("Cartao " + uid + " (" + nomes[i] + ")");
  else        registrar("NEGADO - cartao desconhecido " + uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ---------- Páginas HTTP ----------
// Pede usuario e senha antes de qualquer rota (HTTP Basic Auth)
bool autorizado() {
  if (server.authenticate(WEB_USUARIO, WEB_SENHA)) return true;
  if (server.hasHeader("Authorization"))   // tentou entrar com senha errada
    registrar("LOGIN RECUSADO - " + server.client().remoteIP().toString());
  server.requestAuthentication(BASIC_AUTH, "Fechadura", "Acesso negado: login necessario");
  return false;
}

void voltarInicio() {
  server.sendHeader("Location", "/");
  server.send(303);
}

void paginaInicial() {   // GET /
  if (!autorizado()) return;
  String h = "<!DOCTYPE html><html lang='pt-br'><head><meta charset='utf-8'>"
             "<meta name='viewport' content='width=device-width,initial-scale=1'>"
             "<title>Fechadura</title><style>"
             "body{font-family:sans-serif;max-width:640px;margin:20px auto;padding:0 12px}"
             "button{padding:8px 14px;cursor:pointer}input{padding:6px;margin:2px}"
             "table{border-collapse:collapse;width:100%}td,th{border:1px solid #ccc;padding:6px;text-align:left}"
             ".abrir{font-size:18px;background:#1e2761;color:#fff;border:0;border-radius:6px}"
             "</style></head><body><h1>Fechadura eletronica</h1>";

  h += "<p>Status: <b>" + String(portaAberta ? "DESTRAVADA" : "TRAVADA") + "</b></p>";
  h += "<form method='POST' action='/abrir'><button class='abrir'>Abrir porta</button></form>";

  h += "<h2>Cartoes autorizados</h2><table><tr><th>UID</th><th>Nome</th><th></th></tr>";
  for (int i = 0; i < totalCartoes; i++) {
    h += "<tr><td>" + cartoes[i] + "</td><td>" + escapar(nomes[i]) + "</td><td>"
         "<form method='POST' action='/apagar' style='margin:0'>"
         "<input type='hidden' name='uid' value='" + cartoes[i] + "'>"
         "<button>Apagar</button></form></td></tr>";
  }
  if (totalCartoes == 0) h += "<tr><td colspan='3'>Nenhum cartao cadastrado</td></tr>";
  h += "</table>";

  h += "<h2>Cadastrar cartao</h2><p>Aproxime o cartao do leitor e clique em \"Usar ultimo lido\".</p>"
       "<form method='POST' action='/cadastrar'>"
       "<input id='uid' name='uid' placeholder='UID' required>"
       "<button type='button' onclick=\"fetch('/ultimo').then(r=>r.text()).then(t=>{uid.value=t||'';if(!t)alert('Nenhum cartao lido ainda')})\">Usar ultimo lido</button><br>"
       "<input name='nome' placeholder='Nome do dono' maxlength='30' required>"
       "<button>Cadastrar</button></form>";

  h += "<h2>Log de acessos</h2><table id='log'>";
  for (int i = 0; i < totalLog; i++) h += "<tr><td>" + escapar(logAcessos[i]) + "</td></tr>";
  if (totalLog == 0) h += "<tr><td>Sem registros</td></tr>";
  h += "</table><p><a href='/'>Atualizar</a></p>"
       "<script>setInterval(()=>fetch('/log').then(r=>r.text()).then(t=>document.getElementById('log').innerHTML=t),2000)</script>"
       "</body></html>";

  server.send(200, "text/html", h);
}

void rotaAbrir() {       // POST /abrir
  if (!autorizado()) return;
  abrirPorta("Navegador (" + server.client().remoteIP().toString() + ")");
  voltarInicio();
}

void rotaCadastrar() {   // POST /cadastrar
  if (!autorizado()) return;
  String uid = server.arg("uid");  uid.trim();  uid.toUpperCase();
  String nome = server.arg("nome"); nome.trim();
  if (uid.length() == 0 || nome.length() == 0) { server.send(400, "text/plain", "UID e nome sao obrigatorios"); return; }
  if (buscarCartao(uid) >= 0) { server.send(409, "text/plain", "Cartao ja cadastrado"); return; }
  if (totalCartoes >= MAX_CARTOES) { server.send(507, "text/plain", "Limite de cartoes atingido"); return; }

  cartoes[totalCartoes] = uid;
  nomes[totalCartoes] = nome;
  totalCartoes++;
  salvarCartoes();
  registrar("Cartao cadastrado: " + uid + " (" + nome + ")");
  voltarInicio();
}

void rotaApagar() {      // POST /apagar
  if (!autorizado()) return;
  String uid = server.arg("uid");
  int i = buscarCartao(uid);
  if (i < 0) { server.send(404, "text/plain", "Cartao nao encontrado"); return; }

  String nome = nomes[i];
  for (int j = i; j < totalCartoes - 1; j++) { cartoes[j] = cartoes[j + 1]; nomes[j] = nomes[j + 1]; }
  totalCartoes--;
  salvarCartoes();
  registrar("Cartao removido: " + uid + " (" + nome + ")");
  voltarInicio();
}

void rotaLog() {         // GET /log  (a pagina pede a cada 2 s: polling)
  if (!autorizado()) return;
  String h = "";
  for (int i = 0; i < totalLog; i++) h += "<tr><td>" + escapar(logAcessos[i]) + "</td></tr>";
  if (totalLog == 0) h += "<tr><td>Sem registros</td></tr>";
  server.send(200, "text/html", h);
}

void rotaUltimo() {      // GET /ultimo
  if (!autorizado()) return;
  server.send(200, "text/plain", ultimoUID);
}

// ---------- Programa principal ----------
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // começa travada

  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  carregarCartoes();
  Serial.printf("%d cartoes carregados da memoria\n", totalCartoes);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(SSID, SENHA);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConectado! Acesse: http://" + WiFi.localIP().toString());

  configTime(-3 * 3600, 0, "pool.ntp.org");   // horário de Brasília (se tiver internet)

  server.on("/",          HTTP_GET,  paginaInicial);
  server.on("/abrir",     HTTP_POST, rotaAbrir);
  server.on("/cadastrar", HTTP_POST, rotaCadastrar);
  server.on("/apagar",    HTTP_POST, rotaApagar);
  server.on("/ultimo",    HTTP_GET,  rotaUltimo);
  server.on("/log",       HTTP_GET,  rotaLog);
  server.begin();

  registrar("Sistema iniciado");
}

void loop() {
  server.handleClient();
  lerCartao();

  // Trava de novo depois do tempo, sem travar o servidor com delay()
  if (portaAberta && millis() - abriuEm >= TEMPO_ABERTA) {
    digitalWrite(RELAY_PIN, LOW);
    portaAberta = false;
  }
}
