// =================================================================
//  Sistema de Tráfico Ciudad Inteligente - Generación 2
//  Proyecto: Sistemas Auto-Adaptativos - CON CONECTIVIDAD WiFi
// =================================================================
//  Basado en Gen1 refactorizado + Comunicación WiFi con servidor

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =================================================================
//  CONFIGURACIÓN WiFi - MODIFICAR ESTOS VALORES
// =================================================================
const char* WIFI_SSID = "S23";        // Nombre de tu red WiFi
const char* WIFI_PASSWORD = "tatianahoyos";     // Contraseña WiFi
const char* SERVER_URL = "http://10.65.229.75:5000/api/traffic";  // IP de tu PC

// Intervalo de envío de datos (ms)
#define SEND_INTERVAL 5000

// -- Definiciones de Pines --
#define LR1 5   // Semáforo 1 - Rojo
#define LY1 4   // Semáforo 1 - Amarillo
#define LG1 6   // Semáforo 1 - Verde
#define LR2 7   // Semáforo 2 - Rojo
#define LY2 15  // Semáforo 2 - Amarillo
#define LG2 16  // Semáforo 2 - Verde

#define P1 2    // Botón Peatonal 1
#define P2 1    // Botón Peatonal 2

#define CNY1 42 // Sensores Dirección 1
#define CNY2 41
#define CNY3 40
#define CNY4 39 // Sensores Dirección 2
#define CNY5 38
#define CNY6 37

#define LDR1 13 // Sensores de Luz
#define LDR2 12
#define CO2_PIN 14 // Sensor CO2

// -- Configuración --
#define NIGHT_MODE_THRESHOLD 300
#define HEAVY_TRAFFIC_DIFF 3
#define PEDESTRIAN_CROSS_TIME 15000
#define CO2_HIGH_THRESHOLD 600
#define TIEMPO_RESET_CONTADOR 60000

// -- Pantalla LCD --
LiquidCrystal_I2C lcd(0x27, 20, 4);

// -- Estados del Sistema --
enum Estado { NORMAL, NOCTURNO, TRAFICO_PESADO, PEATONAL, EMISION };
Estado estadoActual = NORMAL;

// -- Fases del Semáforo (Máquina de Estados) --
enum FaseSemaforo { 
  FASE_TL1_VERDE,
  FASE_TL1_AMARILLO,
  FASE_TL1_ROJO,
  FASE_TL2_VERDE,
  FASE_TL2_AMARILLO,
  FASE_TL2_ROJO
};
FaseSemaforo faseActual = FASE_TL1_VERDE;

// -- Tiempos por Modo --
struct TiemposModo {
  int verdeDir1;
  int amarilloDir1;
  int verdeDir2;
  int amarilloDir2;
};

// -- Variables de Control de Tiempo --
unsigned long tiempoInicioFase = 0;
unsigned long duracionFaseActual = 0;

// -- Contadores de Vehículos --
int vehicleCount1 = 0;
int vehicleCount2 = 0;
bool lastCNY1 = HIGH, lastCNY2 = HIGH, lastCNY3 = HIGH;
bool lastCNY4 = HIGH, lastCNY5 = HIGH, lastCNY6 = HIGH;

// -- Sensores Ambientales --
int ldr1Value = 0;
int ldr2Value = 0;
int co2Value = 0;

// -- Modo Peatonal --
volatile bool solicitudPeatonal = false;
unsigned long tiempoInicioModoPeatonal = 0;

// -- Timers --
unsigned long tiempoUltimoReset = 0;
unsigned long ultimaLecturaSensores = 0;
unsigned long ultimaActualizacionLCD = 0;
unsigned long ultimoEnvioServidor = 0;

// -- WiFi Status --
bool wifiConnected = false;

// -- Control Remoto --
bool modoForzadoPorServidor = false;
unsigned long tiempoModoForzado = 0;
#define DURACION_MODO_FORZADO 30000  // 30 segundos de bloqueo

// =================================================================
//  FUNCIONES WiFi
// =================================================================
void conectarWiFi() {
  Serial.println("\n=== Conectando a WiFi ===");
  Serial.print("Red: ");
  Serial.println(WIFI_SSID);
  
  lcd.clear();
  lcd.print("Conectando WiFi...");
  lcd.setCursor(0, 1);
  lcd.print(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(intentos % 20, 2);
    lcd.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n¡WiFi Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.print("WiFi Conectado!");
    lcd.setCursor(0, 1);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println("\nError: No se pudo conectar");
    lcd.clear();
    lcd.print("WiFi Error");
    lcd.setCursor(0, 1);
    lcd.print("Modo Offline");
    delay(2000);
  }
}

void verificarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println("WiFi desconectado. Reintentando...");
    conectarWiFi();
  }
}

String obtenerNombreEstado() {
  switch(estadoActual) {
    case NORMAL: return "NORMAL";
    case NOCTURNO: return "NOCTURNO";
    case TRAFICO_PESADO: return "TRAFICO_PESADO";
    case PEATONAL: return "PEATONAL";
    case EMISION: return "EMISION";
    default: return "DESCONOCIDO";
  }
}

String obtenerNombreFase() {
  switch(faseActual) {
    case FASE_TL1_VERDE: return "TL1_VERDE";
    case FASE_TL1_AMARILLO: return "TL1_AMARILLO";
    case FASE_TL1_ROJO: return "TL1_ROJO";
    case FASE_TL2_VERDE: return "TL2_VERDE";
    case FASE_TL2_AMARILLO: return "TL2_AMARILLO";
    case FASE_TL2_ROJO: return "TL2_ROJO";
    default: return "DESCONOCIDA";
  }
}

void enviarDatosServidor() {
  if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin WiFi - datos no enviados");
    return;
  }
  
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  
  // Construir JSON con datos del sistema
  String json = "{";
  json += "\"estado\":\"" + obtenerNombreEstado() + "\",";
  json += "\"fase\":\"" + obtenerNombreFase() + "\",";
  json += "\"vehiculos_dir1\":" + String(vehicleCount1) + ",";
  json += "\"vehiculos_dir2\":" + String(vehicleCount2) + ",";
  json += "\"ldr1\":" + String(ldr1Value) + ",";
  json += "\"ldr2\":" + String(ldr2Value) + ",";
  json += "\"co2\":" + String(co2Value) + ",";
  json += "\"wifi_rssi\":" + String(WiFi.RSSI());
  json += "}";
  
  Serial.print("Enviando: ");
  Serial.println(json);
  
  int httpCode = http.POST(json);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("Respuesta servidor (");
    Serial.print(httpCode);
    Serial.print("): ");
    Serial.println(response);
    
    // Procesar comandos del servidor
    procesarComandoServidor(response);
  } else {
    Serial.print("Error HTTP: ");
    Serial.println(http.errorToString(httpCode));
  }
  
  http.end();
}

// =================================================================
//  PROCESAR COMANDOS DEL SERVIDOR
// =================================================================
void procesarComandoServidor(String response) {
  // Buscar si hay un comando en la respuesta (con o sin espacio)
  int cmdIndex = response.indexOf("\"command\":");
  if (cmdIndex == -1) {
    return; // No hay comando
  }
  
  // Buscar el inicio del valor del comando (después de las comillas)
  int cmdStart = response.indexOf("\"", cmdIndex + 10);
  if (cmdStart == -1) return;
  cmdStart++; // Saltar la comilla de apertura
  
  int cmdEnd = response.indexOf("\"", cmdStart);
  if (cmdEnd == -1) return;
  
  String comando = response.substring(cmdStart, cmdEnd);
  Serial.print(">>> COMANDO RECIBIDO: ");
  Serial.println(comando);
  
  // Ejecutar el comando
  if (comando == "PEATONAL") {
    solicitudPeatonal = true;
    Serial.println(">>> Activando modo PEATONAL desde servidor");
  }
  else if (comando == "NORMAL") {
    estadoActual = NORMAL;
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    duracionFaseActual = 0;
    modoForzadoPorServidor = true;
    tiempoModoForzado = millis();
    Serial.println(">>> Forzando modo NORMAL desde servidor (30s)");
  }
  else if (comando == "NOCTURNO") {
    estadoActual = NOCTURNO;
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    duracionFaseActual = 0;
    modoForzadoPorServidor = true;
    tiempoModoForzado = millis();
    Serial.println(">>> Forzando modo NOCTURNO desde servidor (30s)");
  }
  
  // Mostrar en LCD que se recibió comando
  lcd.clear();
  lcd.print("CMD Servidor:");
  lcd.setCursor(0, 1);
  lcd.print(comando);
  delay(1000);
}

// =================================================================
void setup() {
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Gen2 - WiFi");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1000);

  pinMode(LR1, OUTPUT); pinMode(LY1, OUTPUT); pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT); pinMode(LY2, OUTPUT); pinMode(LG2, OUTPUT);
  pinMode(P1, INPUT_PULLUP); pinMode(P2, INPUT_PULLUP);
  pinMode(CNY1, INPUT); pinMode(CNY2, INPUT); pinMode(CNY3, INPUT);
  pinMode(CNY4, INPUT); pinMode(CNY5, INPUT); pinMode(CNY6, INPUT);

  attachInterrupt(digitalPinToInterrupt(P1), manejarSolicitudPeatonal, FALLING);
  attachInterrupt(digitalPinToInterrupt(P2), manejarSolicitudPeatonal, FALLING);
  
  todasLucesRojas();
  
  // Conectar WiFi
  conectarWiFi();
  
  tiempoInicioFase = millis();
  lcd.clear();
}

// =================================================================
void loop() {
  unsigned long tiempoActual = millis();
  
  // ===== LEER SENSORES CONSTANTEMENTE (cada 50ms) =====
  if (tiempoActual - ultimaLecturaSensores >= 50) {
    leerSensores();
    ultimaLecturaSensores = tiempoActual;
  }
  
  // ===== ENVIAR DATOS AL SERVIDOR (cada 5 segundos) =====
  if (tiempoActual - ultimoEnvioServidor >= SEND_INTERVAL) {
    enviarDatosServidor();
    ultimoEnvioServidor = tiempoActual;
  }
  
  // ===== VERIFICAR WiFi (cada 30 segundos) =====
  static unsigned long ultimaVerificacionWiFi = 0;
  if (tiempoActual - ultimaVerificacionWiFi >= 30000) {
    verificarWiFi();
    ultimaVerificacionWiFi = tiempoActual;
  }
  
  // ===== MANEJAR SOLICITUD PEATONAL =====
  if (solicitudPeatonal && estadoActual != PEATONAL && co2Value <= CO2_HIGH_THRESHOLD) {
    activarModoPeatonal();
  } else if (solicitudPeatonal && co2Value > CO2_HIGH_THRESHOLD) {
    solicitudPeatonal = false;
    Serial.println("BLOQUEADO: Modo peatonal cancelado por CO2 alto");
  }
  
  // ===== EJECUTAR MODO PEATONAL =====
  if (estadoActual == PEATONAL) {
    ejecutarModoPeatonal(tiempoActual);
    return;
  }
  
  // ===== DETERMINAR MODO DE OPERACIÓN =====
  determinarModo();
  
  // ===== EJECUTAR MÁQUINA DE ESTADOS DEL SEMÁFORO =====
  ejecutarMaquinaEstados(tiempoActual);
  
  // ===== ACTUALIZAR LCD (cada 2 segundos) =====
  if (tiempoActual - ultimaActualizacionLCD >= 2000) {
    actualizarLCD();
    ultimaActualizacionLCD = tiempoActual;
  }
  
  // ===== RESETEAR CONTADORES =====
  if (tiempoActual - tiempoUltimoReset >= TIEMPO_RESET_CONTADOR) {
    Serial.print("RESET D1:");Serial.print(vehicleCount1);
    Serial.print(" D2:");Serial.println(vehicleCount2);
    vehicleCount1 = 0;
    vehicleCount2 = 0;
    tiempoUltimoReset = tiempoActual;
  }
}

// =================================================================
//  LECTURA DE SENSORES
// =================================================================
void leerSensores() {
  ldr1Value = analogRead(LDR1);
  ldr2Value = analogRead(LDR2);
  co2Value = analogRead(CO2_PIN);
  
  bool cny1 = digitalRead(CNY1);
  bool cny2 = digitalRead(CNY2);
  bool cny3 = digitalRead(CNY3);
  bool cny4 = digitalRead(CNY4);
  bool cny5 = digitalRead(CNY5);
  bool cny6 = digitalRead(CNY6);
  
  if (lastCNY1 != cny1) { vehicleCount1++; lastCNY1 = cny1; }
  if (lastCNY2 != cny2) { vehicleCount1++; lastCNY2 = cny2; }
  if (lastCNY3 != cny3) { vehicleCount1++; lastCNY3 = cny3; }
  if (lastCNY4 != cny4) { vehicleCount2++; lastCNY4 = cny4; }
  if (lastCNY5 != cny5) { vehicleCount2++; lastCNY5 = cny5; }
  if (lastCNY6 != cny6) { vehicleCount2++; lastCNY6 = cny6; }
}

// =================================================================
//  DETERMINACIÓN DE MODO
// =================================================================
void determinarModo() {
  // Si el modo fue forzado por el servidor, no cambiar hasta que expire
  if (modoForzadoPorServidor) {
    if (millis() - tiempoModoForzado < DURACION_MODO_FORZADO) {
      return; // Mantener modo forzado
    } else {
      modoForzadoPorServidor = false;
      Serial.println(">>> Modo forzado expirado, volviendo a automático");
    }
  }
  
  Estado modoAnterior = estadoActual;
  
  if (co2Value > CO2_HIGH_THRESHOLD) {
    estadoActual = EMISION;
  } else if (ldr1Value < NIGHT_MODE_THRESHOLD && ldr2Value < NIGHT_MODE_THRESHOLD) {
    estadoActual = NOCTURNO;
  } else if (abs(vehicleCount1 - vehicleCount2) >= HEAVY_TRAFFIC_DIFF) {
    estadoActual = TRAFICO_PESADO;
  } else {
    estadoActual = NORMAL;
  }
  
  if (modoAnterior != estadoActual && estadoActual != PEATONAL) {
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    Serial.print("CAMBIO DE MODO: ");
    Serial.println(obtenerNombreEstado());
  }
}

// =================================================================
//  OBTENER TIEMPOS SEGÚN MODO
// =================================================================
TiemposModo obtenerTiempos() {
  TiemposModo t;
  
  switch(estadoActual) {
    case NORMAL:
      t.verdeDir1 = 10000; t.amarilloDir1 = 3000;
      t.verdeDir2 = 10000; t.amarilloDir2 = 3000;
      break;
    case NOCTURNO:
      t.verdeDir1 = 6000; t.amarilloDir1 = 2000;
      t.verdeDir2 = 6000; t.amarilloDir2 = 2000;
      break;
    case TRAFICO_PESADO:
      if (vehicleCount1 > vehicleCount2) {
        t.verdeDir1 = 15000; t.amarilloDir1 = 3000;
        t.verdeDir2 = 5000;  t.amarilloDir2 = 3000;
      } else {
        t.verdeDir1 = 5000;  t.amarilloDir1 = 3000;
        t.verdeDir2 = 15000; t.amarilloDir2 = 3000;
      }
      break;
    case EMISION:
      t.verdeDir1 = 20000; t.amarilloDir1 = 2000;
      t.verdeDir2 = 20000; t.amarilloDir2 = 2000;
      break;
    default:
      t.verdeDir1 = 10000; t.amarilloDir1 = 3000;
      t.verdeDir2 = 10000; t.amarilloDir2 = 3000;
  }
  
  return t;
}

// =================================================================
//  MÁQUINA DE ESTADOS DEL SEMÁFORO
// =================================================================
void ejecutarMaquinaEstados(unsigned long tiempoActual) {
  TiemposModo tiempos = obtenerTiempos();
  
  if (tiempoActual - tiempoInicioFase < duracionFaseActual) {
    return;
  }
  
  tiempoInicioFase = tiempoActual;
  FaseSemaforo siguienteFase = faseActual;
  
  switch(faseActual) {
    case FASE_TL1_VERDE:
      establecerLuces(false, true, false, false, false, true);
      duracionFaseActual = tiempos.amarilloDir1;
      siguienteFase = FASE_TL1_AMARILLO;
      break;
    case FASE_TL1_AMARILLO:
      todasLucesRojas();
      duracionFaseActual = 1000;
      siguienteFase = FASE_TL1_ROJO;
      break;
    case FASE_TL1_ROJO:
      establecerLuces(false, false, true, true, false, false);
      duracionFaseActual = tiempos.verdeDir2;
      siguienteFase = FASE_TL2_VERDE;
      break;
    case FASE_TL2_VERDE:
      establecerLuces(false, false, true, false, true, false);
      duracionFaseActual = tiempos.amarilloDir2;
      siguienteFase = FASE_TL2_AMARILLO;
      break;
    case FASE_TL2_AMARILLO:
      todasLucesRojas();
      duracionFaseActual = 1000;
      siguienteFase = FASE_TL2_ROJO;
      break;
    case FASE_TL2_ROJO:
      establecerLuces(true, false, false, false, false, true);
      duracionFaseActual = tiempos.verdeDir1;
      siguienteFase = FASE_TL1_VERDE;
      break;
  }
  
  faseActual = siguienteFase;
}

// =================================================================
//  MODO PEATONAL
// =================================================================
void activarModoPeatonal() {
  estadoActual = PEATONAL;
  solicitudPeatonal = false;
  tiempoInicioModoPeatonal = millis();
  Serial.println("MODO PEATONAL ACTIVADO");
}

void ejecutarModoPeatonal(unsigned long tiempoActual) {
  unsigned long tiempoTranscurrido = tiempoActual - tiempoInicioModoPeatonal;
  
  if (tiempoTranscurrido < 3000) {
    digitalWrite(LG1, LOW); digitalWrite(LY1, HIGH); digitalWrite(LR1, LOW);
    digitalWrite(LG2, LOW); digitalWrite(LY2, HIGH); digitalWrite(LR2, LOW);
    
    static int ultimoSegundoAmarillo = -1;
    int segundosAmarillo = 3 - (tiempoTranscurrido / 1000);
    if (segundosAmarillo != ultimoSegundoAmarillo) {
      ultimoSegundoAmarillo = segundosAmarillo;
      lcd.clear();
      lcd.print("PREPARANDO CRUCE");
      lcd.setCursor(0, 1);
      lcd.print("Amarillo: ");
      lcd.print(segundosAmarillo);
      lcd.print("s");
    }
  }
  else if (tiempoTranscurrido < 3000 + PEDESTRIAN_CROSS_TIME) {
    todasLucesRojas();
    
    static int ultimoSegundo = -1;
    int segundosRestantes = (3000 + PEDESTRIAN_CROSS_TIME - tiempoTranscurrido) / 1000;
    if (segundosRestantes != ultimoSegundo) {
      ultimoSegundo = segundosRestantes;
      lcd.clear();
      lcd.print("Modo: PEATONAL");
      lcd.setCursor(0, 1);
      lcd.print("Cruce Seguro: ");
      lcd.print(segundosRestantes);
      lcd.print("s");
    }
  }
  else if (tiempoTranscurrido < 3000 + PEDESTRIAN_CROSS_TIME + 2000) {
    int ciclo = (tiempoTranscurrido - 3000 - PEDESTRIAN_CROSS_TIME) / 500;
    if (ciclo % 2 == 0) {
      digitalWrite(LY1, HIGH);
      digitalWrite(LY2, HIGH);
    } else {
      todasLucesApagadas();
    }
  }
  else {
    estadoActual = NORMAL;
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    duracionFaseActual = 0;
    establecerLuces(true, false, false, false, false, true);
    Serial.println("MODO PEATONAL FINALIZADO");
  }
}

// =================================================================
//  CONTROL DE LUCES
// =================================================================
void establecerLuces(bool g1, bool y1, bool r1, bool g2, bool y2, bool r2) {
  digitalWrite(LG1, g1); digitalWrite(LY1, y1); digitalWrite(LR1, r1);
  digitalWrite(LG2, g2); digitalWrite(LY2, y2); digitalWrite(LR2, r2);
}

void todasLucesRojas() {
  establecerLuces(false, false, true, false, false, true);
}

void todasLucesApagadas() {
  establecerLuces(false, false, false, false, false, false);
}

// =================================================================
//  LCD
// =================================================================
void actualizarLCD() {
  lcd.clear();
  
  // Línea 1: Modo + WiFi
  lcd.setCursor(0, 0);
  switch(estadoActual) {
    case NORMAL: lcd.print("NORMAL"); break;
    case NOCTURNO: lcd.print("NOCTURNO"); break;
    case TRAFICO_PESADO: lcd.print("T.PESADO"); break;
    case EMISION: lcd.print("EMISION"); break;
    case PEATONAL: return;
  }
  lcd.setCursor(12, 0);
  lcd.print(wifiConnected ? "WiFi:OK" : "WiFi:--");
  
  // Línea 2: Vehículos
  lcd.setCursor(0, 1);
  lcd.print("D1=");
  lcd.print(vehicleCount1);
  lcd.print(" D2=");
  lcd.print(vehicleCount2);
  
  // Línea 3: Ambiente
  lcd.setCursor(0, 2);
  lcd.print("Luz:");
  lcd.print((ldr1Value + ldr2Value) / 2);
  lcd.print(" CO2:");
  lcd.print(co2Value);
  
  // Línea 4: Fase + RSSI
  lcd.setCursor(0, 3);
  switch(faseActual) {
    case FASE_TL1_VERDE: lcd.print("TL1:V TL2:R"); break;
    case FASE_TL1_AMARILLO: lcd.print("TL1:A TL2:R"); break;
    case FASE_TL2_VERDE: lcd.print("TL1:R TL2:V"); break;
    case FASE_TL2_AMARILLO: lcd.print("TL1:R TL2:A"); break;
    default: lcd.print("TRANS"); break;
  }
  if (wifiConnected) {
    lcd.setCursor(14, 3);
    lcd.print(WiFi.RSSI());
    lcd.print("dB");
  }
}

// =================================================================
//  INTERRUPCIONES
// =================================================================
void manejarSolicitudPeatonal() {
  solicitudPeatonal = true;
}
