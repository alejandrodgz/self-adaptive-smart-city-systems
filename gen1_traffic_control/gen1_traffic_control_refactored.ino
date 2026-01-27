// =================================================================
//  Sistema de Tráfico Ciudad Inteligente - Generación 1 (Nivel Bajo)
//  Proyecto: Sistemas Auto-Adaptativos - VERSIÓN REFACTORIZADA
// =================================================================
//  Sistema NO BLOQUEANTE con máquina de estados
//  Lee sensores constantemente, responde rápido a cambios

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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
  FASE_TL1_VERDE,      // Semáforo 1 verde
  FASE_TL1_AMARILLO,   // Semáforo 1 amarillo
  FASE_TL1_ROJO,       // Transición todo rojo
  FASE_TL2_VERDE,      // Semáforo 2 verde
  FASE_TL2_AMARILLO,   // Semáforo 2 amarillo
  FASE_TL2_ROJO        // Transición todo rojo
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

// =================================================================
void setup() {
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Sistema OPTIMIZADO");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1500);
  lcd.clear();

  pinMode(LR1, OUTPUT); pinMode(LY1, OUTPUT); pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT); pinMode(LY2, OUTPUT); pinMode(LG2, OUTPUT);
  pinMode(P1, INPUT_PULLUP); pinMode(P2, INPUT_PULLUP);
  pinMode(CNY1, INPUT); pinMode(CNY2, INPUT); pinMode(CNY3, INPUT);
  pinMode(CNY4, INPUT); pinMode(CNY5, INPUT); pinMode(CNY6, INPUT);

  attachInterrupt(digitalPinToInterrupt(P1), manejarSolicitudPeatonal, FALLING);
  attachInterrupt(digitalPinToInterrupt(P2), manejarSolicitudPeatonal, FALLING);
  
  todasLucesRojas();
  tiempoInicioFase = millis();
}

// =================================================================
void loop() {
  unsigned long tiempoActual = millis();
  
  // ===== LEER SENSORES CONSTANTEMENTE (cada 50ms) =====
  if (tiempoActual - ultimaLecturaSensores >= 50) {
    leerSensores();
    ultimaLecturaSensores = tiempoActual;
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
    return; // No ejecutar semáforos normales
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
  // Sensores ambientales
  ldr1Value = analogRead(LDR1);
  ldr2Value = analogRead(LDR2);
  co2Value = analogRead(CO2_PIN);
  
  // Sensores de vehículos - Detección de cambios
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
  
  // Si cambió el modo, reiniciar fase
  if (modoAnterior != estadoActual && estadoActual != PEATONAL) {
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    Serial.print("CAMBIO DE MODO: ");
    Serial.println(estadoActual);
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
  
  // Verificar si completó la fase actual
  if (tiempoActual - tiempoInicioFase < duracionFaseActual) {
    return; // Aún no termina la fase
  }
  
  // Avanzar a siguiente fase
  tiempoInicioFase = tiempoActual;
  FaseSemaforo siguienteFase = faseActual;
  
  switch(faseActual) {
    case FASE_TL1_VERDE:
      establecerLuces(false, true, false, false, false, true); // TL1 amarillo, TL2 rojo
      duracionFaseActual = tiempos.amarilloDir1;
      siguienteFase = FASE_TL1_AMARILLO;
      break;
      
    case FASE_TL1_AMARILLO:
      todasLucesRojas(); // Todo rojo (seguridad)
      duracionFaseActual = 1000;
      siguienteFase = FASE_TL1_ROJO;
      break;
      
    case FASE_TL1_ROJO:
      establecerLuces(false, false, true, true, false, false); // TL1 rojo, TL2 verde
      duracionFaseActual = tiempos.verdeDir2;
      siguienteFase = FASE_TL2_VERDE;
      break;
      
    case FASE_TL2_VERDE:
      establecerLuces(false, false, true, false, true, false); // TL1 rojo, TL2 amarillo
      duracionFaseActual = tiempos.amarilloDir2;
      siguienteFase = FASE_TL2_AMARILLO;
      break;
      
    case FASE_TL2_AMARILLO:
      todasLucesRojas(); // Todo rojo (seguridad)
      duracionFaseActual = 1000;
      siguienteFase = FASE_TL2_ROJO;
      break;
      
    case FASE_TL2_ROJO:
      establecerLuces(true, false, false, false, false, true); // TL1 verde, TL2 rojo
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
  // No cambiar luces aquí - se manejan en ejecutarModoPeatonal
  Serial.println("MODO PEATONAL ACTIVADO - Transicion Amarilla");
}

void ejecutarModoPeatonal(unsigned long tiempoActual) {
  unsigned long tiempoTranscurrido = tiempoActual - tiempoInicioModoPeatonal;
  
  // FASE 1: Transición Amarilla (primeros 3 segundos)
  if (tiempoTranscurrido < 3000) {
    // Ambos semáforos en amarillo
    digitalWrite(LG1, LOW);
    digitalWrite(LY1, HIGH);
    digitalWrite(LR1, LOW);
    digitalWrite(LG2, LOW);
    digitalWrite(LY2, HIGH);
    digitalWrite(LR2, LOW);
    
    // Actualizar LCD
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
  // FASE 2: Todo Rojo - Cruce Peatonal (3s a 18s = 15 segundos)
  else if (tiempoTranscurrido < 3000 + PEDESTRIAN_CROSS_TIME) {
    todasLucesRojas();
    
    // Actualizar LCD con cuenta regresiva
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
  // FASE 3: Parpadeo Amarillo (18s a 20s = 2 segundos)
  else if (tiempoTranscurrido < 3000 + PEDESTRIAN_CROSS_TIME + 2000) {
    int ciclo = (tiempoTranscurrido - 3000 - PEDESTRIAN_CROSS_TIME) / 500;
    if (ciclo % 2 == 0) {
      digitalWrite(LY1, HIGH);
      digitalWrite(LY2, HIGH);
    } else {
      todasLucesApagadas();
    }
  }
  // FASE 4: Finalizar
  else {
    estadoActual = NORMAL;
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    duracionFaseActual = 0; // Forzar inicio inmediato de la fase
    establecerLuces(true, false, false, false, false, true); // TL1 verde, TL2 rojo
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
  
  // Línea 1: Modo
  lcd.setCursor(0, 0);
  switch(estadoActual) {
    case NORMAL: lcd.print("Modo: NORMAL"); break;
    case NOCTURNO: lcd.print("Modo: NOCTURNO"); break;
    case TRAFICO_PESADO: lcd.print("Modo: T. PESADO"); break;
    case EMISION: lcd.print("Modo: EMISION CO2"); break;
    case PEATONAL: return; // Manejado en ejecutarModoPeatonal
  }
  
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
  
  // Línea 4: Fase actual
  lcd.setCursor(0, 3);
  switch(faseActual) {
    case FASE_TL1_VERDE: lcd.print("TL1:VERDE TL2:ROJO"); break;
    case FASE_TL1_AMARILLO: lcd.print("TL1:AMAR TL2:ROJO"); break;
    case FASE_TL2_VERDE: lcd.print("TL1:ROJO TL2:VERDE"); break;
    case FASE_TL2_AMARILLO: lcd.print("TL1:ROJO TL2:AMAR"); break;
    default: lcd.print("TRANSICION"); break;
  }
}

// =================================================================
//  INTERRUPCIONES
// =================================================================
void manejarSolicitudPeatonal() {
  solicitudPeatonal = true;
}
