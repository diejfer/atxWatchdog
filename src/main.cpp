#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Pines GPIO
#define POWER_PIN D1  // GPIO5
#define RESET_PIN D2  // GPIO4

// Archivo de configuracion
#define CONFIG_FILE "/config.json"

// Estructura de configuracion
struct Config {
  char hostname[32];
  char mqtt_server[64];
  int mqtt_port;
  char mqtt_user[32];
  char mqtt_pass[32];
  char client_id[32];
  int power_click_ms;
  int reset_click_ms;
  int watchdog_timeout_ms;
};

// Variables globales
Config config;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP8266WebServer server(80);

unsigned long lastMqttKeepalive = 0;
unsigned long lastSerialKeepalive = 0;
const unsigned long MQTT_KEEPALIVE_INTERVAL = 60000; // 60 segundos
bool serialWatchdogEnabled = false;

// Funciones adelantadas
void loadConfig();
void saveConfig();
void setupWebServer();
void handleRoot();
void handleSaveConfig();
void handleClickPower();
void handleClickReset();
void clickButton(int pin, int duration_ms);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMqtt();
void publishKeepalive();
void checkSerialWatchdog();

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n=================================");
  Serial.println("ATX Watchdog - Iniciando...");
  Serial.println("=================================");

  // Configurar pines
  pinMode(POWER_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);

  Serial.println("GPIOs configurados (D1=Power, D2=Reset)");

  // Inicializar LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Error montando LittleFS");
    return;
  }
  Serial.println("LittleFS montado correctamente");

  // Cargar configuracion
  loadConfig();

  // WiFiManager - Configuracion inicial
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // 3 minutos timeout

  Serial.println("Conectando a WiFi...");
  if (!wifiManager.autoConnect("ATX-Watchdog-Setup")) {
    Serial.println("Fallo al conectar WiFi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Configurar hostname
  if (strlen(config.hostname) > 0) {
    WiFi.hostname(config.hostname);
  }

  // Configurar MQTT
  mqttClient.setServer(config.mqtt_server, config.mqtt_port);
  mqttClient.setCallback(mqttCallback);

  // Iniciar webserver
  setupWebServer();
  server.begin();
  Serial.println("Servidor web iniciado en puerto 80");

  Serial.println("\n=================================");
  Serial.println("Sistema listo!");
  Serial.println("=================================\n");
}

void loop() {
  // Manejar servidor web
  server.handleClient();

  // Mantener conexion MQTT
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  mqttClient.loop();

  // Enviar keepalive MQTT
  unsigned long now = millis();
  if (now - lastMqttKeepalive > MQTT_KEEPALIVE_INTERVAL) {
    publishKeepalive();
    lastMqttKeepalive = now;
  }

  // Verificar watchdog serial
  checkSerialWatchdog();
}

void loadConfig() {
  // Valores por defecto
  strcpy(config.hostname, "atx-watchdog");
  strcpy(config.mqtt_server, "");
  config.mqtt_port = 1883;
  strcpy(config.mqtt_user, "");
  strcpy(config.mqtt_pass, "");
  strcpy(config.client_id, "watchdog-001");
  config.power_click_ms = 200;
  config.reset_click_ms = 200;
  config.watchdog_timeout_ms = 120000; // 2 minutos

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Config no encontrado, usando valores por defecto");
    saveConfig(); // Guardar defaults
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Error parseando config JSON");
    return;
  }

  strlcpy(config.hostname, doc["hostname"] | "atx-watchdog", sizeof(config.hostname));
  strlcpy(config.mqtt_server, doc["mqtt_server"] | "", sizeof(config.mqtt_server));
  config.mqtt_port = doc["mqtt_port"] | 1883;
  strlcpy(config.mqtt_user, doc["mqtt_user"] | "", sizeof(config.mqtt_user));
  strlcpy(config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(config.mqtt_pass));
  strlcpy(config.client_id, doc["client_id"] | "watchdog-001", sizeof(config.client_id));
  config.power_click_ms = doc["power_click_ms"] | 200;
  config.reset_click_ms = doc["reset_click_ms"] | 200;
  config.watchdog_timeout_ms = doc["watchdog_timeout_ms"] | 120000;

  Serial.println("Configuracion cargada desde archivo");
}

void saveConfig() {
  JsonDocument doc;

  doc["hostname"] = config.hostname;
  doc["mqtt_server"] = config.mqtt_server;
  doc["mqtt_port"] = config.mqtt_port;
  doc["mqtt_user"] = config.mqtt_user;
  doc["mqtt_pass"] = config.mqtt_pass;
  doc["client_id"] = config.client_id;
  doc["power_click_ms"] = config.power_click_ms;
  doc["reset_click_ms"] = config.reset_click_ms;
  doc["watchdog_timeout_ms"] = config.watchdog_timeout_ms;

  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Error abriendo archivo config para escritura");
    return;
  }

  serializeJson(doc, file);
  file.close();
  Serial.println("Configuracion guardada");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSaveConfig);
  server.on("/power", HTTP_POST, handleClickPower);
  server.on("/reset", HTTP_POST, handleClickReset);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ATX Watchdog Config</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:#f0f0f0}";
  html += "h1{color:#333;text-align:center}";
  html += ".card{background:white;padding:20px;margin:20px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += "label{display:block;margin:10px 0 5px;font-weight:bold}";
  html += "input{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}";
  html += "button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;width:100%;margin:5px 0}";
  html += "button:hover{background:#45a049}";
  html += ".danger{background:#f44336}.danger:hover{background:#da190b}";
  html += ".warning{background:#ff9800}.warning:hover{background:#e68900}";
  html += "</style></head><body>";
  html += "<h1>ATX Watchdog</h1>";

  html += "<div class='card'><h2>Configuracion MQTT</h2>";
  html += "<form action='/save' method='POST'>";
  html += "<label>Hostname:</label><input name='hostname' value='" + String(config.hostname) + "'>";
  html += "<label>MQTT Server:</label><input name='mqtt_server' value='" + String(config.mqtt_server) + "'>";
  html += "<label>MQTT Port:</label><input type='number' name='mqtt_port' value='" + String(config.mqtt_port) + "'>";
  html += "<label>MQTT User:</label><input name='mqtt_user' value='" + String(config.mqtt_user) + "'>";
  html += "<label>MQTT Password:</label><input type='password' name='mqtt_pass' value='" + String(config.mqtt_pass) + "'>";
  html += "<label>Client ID:</label><input name='client_id' value='" + String(config.client_id) + "'>";
  html += "<label>Power Click (ms):</label><input type='number' name='power_click_ms' value='" + String(config.power_click_ms) + "'>";
  html += "<label>Reset Click (ms):</label><input type='number' name='reset_click_ms' value='" + String(config.reset_click_ms) + "'>";
  html += "<label>Watchdog Timeout (ms):</label><input type='number' name='watchdog_timeout_ms' value='" + String(config.watchdog_timeout_ms) + "'>";
  html += "<button type='submit'>Guardar Configuracion</button>";
  html += "</form></div>";

  html += "<div class='card'><h2>Control Manual</h2>";
  html += "<button class='warning' onclick='fetch(\"/power\",{method:\"POST\"}).then(()=>alert(\"Power click enviado\"))'>POWER CLICK</button>";
  html += "<button class='danger' onclick='fetch(\"/reset\",{method:\"POST\"}).then(()=>alert(\"Reset click enviado\"))'>RESET CLICK</button>";
  html += "</div>";

  html += "<div class='card'><h2>Estado</h2>";
  html += "<p><b>IP:</b> " + WiFi.localIP().toString() + "</p>";
  html += "<p><b>MQTT:</b> " + String(mqttClient.connected() ? "Conectado" : "Desconectado") + "</p>";
  html += "<p><b>Topic CMD:</b> /watchdog/" + String(config.client_id) + "/cmd</p>";
  html += "<p><b>Topic Status:</b> /watchdog/" + String(config.client_id) + "/status</p>";
  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  if (server.hasArg("hostname")) strlcpy(config.hostname, server.arg("hostname").c_str(), sizeof(config.hostname));
  if (server.hasArg("mqtt_server")) strlcpy(config.mqtt_server, server.arg("mqtt_server").c_str(), sizeof(config.mqtt_server));
  if (server.hasArg("mqtt_port")) config.mqtt_port = server.arg("mqtt_port").toInt();
  if (server.hasArg("mqtt_user")) strlcpy(config.mqtt_user, server.arg("mqtt_user").c_str(), sizeof(config.mqtt_user));
  if (server.hasArg("mqtt_pass")) strlcpy(config.mqtt_pass, server.arg("mqtt_pass").c_str(), sizeof(config.mqtt_pass));
  if (server.hasArg("client_id")) strlcpy(config.client_id, server.arg("client_id").c_str(), sizeof(config.client_id));
  if (server.hasArg("power_click_ms")) config.power_click_ms = server.arg("power_click_ms").toInt();
  if (server.hasArg("reset_click_ms")) config.reset_click_ms = server.arg("reset_click_ms").toInt();
  if (server.hasArg("watchdog_timeout_ms")) config.watchdog_timeout_ms = server.arg("watchdog_timeout_ms").toInt();

  saveConfig();

  server.send(200, "text/html", "<html><body><h1>Configuracion guardada!</h1><p>Reiniciando en 3 segundos...</p><script>setTimeout(()=>window.location='/',3000)</script></body></html>");

  delay(3000);
  ESP.restart();
}

void handleClickPower() {
  Serial.println("Click POWER desde web");
  clickButton(POWER_PIN, config.power_click_ms);
  server.send(200, "text/plain", "OK");
}

void handleClickReset() {
  Serial.println("Click RESET desde web");
  clickButton(RESET_PIN, config.reset_click_ms);
  server.send(200, "text/plain", "OK");
}

void clickButton(int pin, int duration_ms) {
  Serial.printf("Ejecutando click en pin %d por %d ms\n", pin, duration_ms);
  digitalWrite(pin, LOW);
  delay(duration_ms);
  digitalWrite(pin, HIGH);
}

void reconnectMqtt() {
  static unsigned long lastAttempt = 0;
  unsigned long now = millis();

  if (now - lastAttempt < 5000) return; // Intentar cada 5 segundos
  lastAttempt = now;

  if (strlen(config.mqtt_server) == 0) return; // No configurado

  Serial.print("Conectando a MQTT...");

  String clientId = String(config.client_id);
  bool connected = false;

  if (strlen(config.mqtt_user) > 0) {
    connected = mqttClient.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass);
  } else {
    connected = mqttClient.connect(clientId.c_str());
  }

  if (connected) {
    Serial.println("MQTT conectado!");

    // Suscribirse al topic de comandos
    String cmdTopic = "/watchdog/" + String(config.client_id) + "/cmd";
    mqttClient.subscribe(cmdTopic.c_str());
    Serial.printf("Suscrito a: %s\n", cmdTopic.c_str());

    // Publicar que estamos online
    publishKeepalive();
  } else {
    Serial.printf("fallo, rc=%d\n", mqttClient.state());
  }
}

void publishKeepalive() {
  if (!mqttClient.connected()) return;

  String statusTopic = "/watchdog/" + String(config.client_id) + "/status";

  JsonDocument doc;
  doc["status"] = "online";
  doc["uptime"] = millis() / 1000;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish(statusTopic.c_str(), payload.c_str());
  Serial.println("Keepalive MQTT enviado");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Mensaje MQTT recibido en %s: ", topic);

  // Parsear JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.println("Error parseando JSON");
    return;
  }

  serializeJson(doc, Serial);
  Serial.println();

  // Procesar comandos
  const char* cmd = doc["cmd"];
  if (cmd == nullptr) return;

  if (strcmp(cmd, "power") == 0) {
    int duration = doc["duration"] | config.power_click_ms;
    Serial.printf("Comando POWER recibido (duracion: %d ms)\n", duration);
    clickButton(POWER_PIN, duration);
  }
  else if (strcmp(cmd, "reset") == 0) {
    int duration = doc["duration"] | config.reset_click_ms;
    Serial.printf("Comando RESET recibido (duracion: %d ms)\n", duration);
    clickButton(RESET_PIN, duration);
  }
  else {
    Serial.printf("Comando desconocido: %s\n", cmd);
  }
}

void checkSerialWatchdog() {
  // Verificar si hay datos en el serial (keepalive del host)
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("KEEPALIVE") || line.startsWith("PING")) {
      lastSerialKeepalive = millis();
      serialWatchdogEnabled = true;
      Serial.println("ACK"); // Responder al host
    }
  }

  // Verificar timeout del watchdog serial
  if (serialWatchdogEnabled) {
    unsigned long now = millis();
    if (now - lastSerialKeepalive > config.watchdog_timeout_ms) {
      Serial.println("WATCHDOG TIMEOUT! Ejecutando POWER CLICK");
      clickButton(POWER_PIN, config.power_click_ms);
      serialWatchdogEnabled = false; // Deshabilitar hasta recibir nuevo keepalive
      lastSerialKeepalive = now;
    }
  }
}
