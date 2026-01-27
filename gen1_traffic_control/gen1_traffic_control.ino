// =================================================================
//  Sistema de Tráfico Ciudad Inteligente - Generación 1 (Nivel Bajo)
//  Proyecto: Sistemas Auto-Adaptativos
// =================================================================
//  Este código implementa un sistema de control de tráfico consciente
//  del contexto con cuatro modos de operación predefinidos. Ajusta su
//  comportamiento según condiciones ambientales y de tráfico pero no aprende.

// =================================================================

// -- Definiciones de Pines --
// --- Microcontrolador ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Semáforo 1 ---
#define LR1 5   // Rojo
#define LY1 4   // Amarillo
#define LG1 6   // Verde

// --- Semáforo 2 ---
#define LR2 7   // Rojo
#define LY2 15  // Amarillo
#define LG2 16  // Verde

// --- Botones Peatonales ---
#define P1 2    // Botón para Semáforo 1
#define P2 1    // Botón para Semáforo 2

// --- Sensores de Vehículos (Infrarrojo) ---
#define CNY1 42 // Sensor 1, Dirección 1
#define CNY2 41 // Sensor 2, Dirección 1
#define CNY3 40 // Sensor 3, Dirección 1
#define CNY4 39 // Sensor 4, Dirección 2
#define CNY5 38 // Sensor 5, Dirección 2
#define CNY6 37 // Sensor 6, Dirección 2

// --- Sensores Ambientales ---
#define LDR1 13 // Sensor de Luz 1
#define LDR2 12 // Sensor de Luz 2
#define CO2_PIN 14 // Sensor CO2

// -- Configuración del Sistema --
#define NIGHT_MODE_THRESHOLD 300  // Valor del LDR para activar modo nocturno (LDR lee BAJO en oscuridad)
#define HEAVY_TRAFFIC_DIFF 3      // Diferencia de vehículos para activar modo tráfico pesado
#define PEDESTRIAN_CROSS_TIME 15000 // 15 segundos para cruce peatonal
#define CO2_HIGH_THRESHOLD 600    // Valor de CO2 para activar modo reducción de emisiones

// -- Pantalla LCD --
LiquidCrystal_I2C lcd(0x27, 20, 4); // Dirección I2C 0x27, 20 columnas y 4 filas

// -- Variables de Estado Globales --
enum Estado { NORMAL, NOCTURNO, TRAFICO_PESADO, PEATONAL, EMISION };
Estado estadoActual = NORMAL;

// Contadores acumulativos de vehículos
int vehicleCount1 = 0;
int vehicleCount2 = 0;

// Estados previos de sensores para detección de flancos
bool lastCNY1 = HIGH, lastCNY2 = HIGH, lastCNY3 = HIGH;
bool lastCNY4 = HIGH, lastCNY5 = HIGH, lastCNY6 = HIGH;

// Variables de sensores ambientales
int ldr1Value = 0;
int ldr2Value = 0;
int co2Value = 0;

// Variables para modo peatonal
volatile bool solicitudPeatonal = false;
unsigned long tiempoInicioModoPeatonal = 0;
bool modoPeatonalActivo = false;

// Timer para resetear contadores
unsigned long tiempoUltimoReset = 0;
#define TIEMPO_RESET_CONTADOR 60000 // Resetear cada 60 segundos

// =================================================================
//  CONFIGURACIÓN: Se ejecuta una vez al inicio
// =================================================================
void setup() {
  Serial.begin(115200);

  // --- Inicializar LCD ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando Sistema...");
  delay(1000);
  lcd.clear();

  // --- Inicializar Semáforos (Salidas) ---
  pinMode(LR1, OUTPUT);
  pinMode(LY1, OUTPUT);
  pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT);
  pinMode(LY2, OUTPUT);
  pinMode(LG2, OUTPUT);

  // --- Inicializar Sensores (Entradas) ---
  pinMode(P1, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);
  pinMode(CNY1, INPUT);
  pinMode(CNY2, INPUT);
  pinMode(CNY3, INPUT);
  pinMode(CNY4, INPUT);
  pinMode(CNY5, INPUT);
  pinMode(CNY6, INPUT);
  // Los pines analógicos (LDR, CO2) no necesitan pinMode

  // --- Adjuntar Interrupciones para Botones Peatonales ---
  attachInterrupt(digitalPinToInterrupt(P1), manejarSolicitudPeatonal, FALLING);
  attachInterrupt(digitalPinToInterrupt(P2), manejarSolicitudPeatonal, FALLING);
  
  // Iniciar con todas las luces en rojo
  todasLucesRojas();
}

// =================================================================
//  BUCLE PRINCIPAL: Se ejecuta continuamente
// =================================================================
void loop() {
  // 1. Leer sensores primero para tener datos actualizados de CO2
  leerTodosSensores();
  
  // 2. Verificar si hay solicitud peatonal (desde interrupción)
  // IMPORTANTE: Modo EMISION tiene prioridad ABSOLUTA - ni peatonal lo interrumpe
  if (solicitudPeatonal && estadoActual != PEATONAL && co2Value <= CO2_HIGH_THRESHOLD) {
    estadoActual = PEATONAL;
    solicitudPeatonal = false;
    modoPeatonalActivo = true;
    tiempoInicioModoPeatonal = millis();
    todasLucesRojas();
  } else if (solicitudPeatonal && co2Value > CO2_HIGH_THRESHOLD) {
    // Si hay solicitud peatonal pero CO2 alto, ignorar y resetear bandera
    solicitudPeatonal = false;
    Serial.println("ALERTA: Solicitud peatonal BLOQUEADA por CO2 alto!");
  }
  
  // 3. Manejar modo peatonal con temporizador no bloqueante
  if (estadoActual == PEATONAL) {
    if (millis() - tiempoInicioModoPeatonal >= PEDESTRIAN_CROSS_TIME) {
      parpadearTodasAmarillas(2);
      estadoActual = NORMAL;
      modoPeatonalActivo = false;
    }
    actualizarPantalla();
    return; // No ejecutar otros modos mientras peatonal está activo
  }

  // 4. Determinar el estado actual basado en el contexto
  determinarEstado();

  // 5. Ejecutar la lógica para el estado actual
  switch (estadoActual) {
    case NORMAL:
      ejecutarModoNormal();
      break;
    case NOCTURNO:
      ejecutarModoNocturno();
      break;
    case TRAFICO_PESADO:
      ejecutarModoTraficoPesado();
      break;
    case EMISION:
      ejecutarModoEmision();
      break;
    case PEATONAL:
      // Manejado arriba con temporizador no bloqueante
      break;
  }
  
  // 6. Actualizar la pantalla con la información actual
  actualizarPantalla();
}

// =================================================================
//  Determinación de Estado y Lectura de Sensores
// =================================================================

void leerTodosSensores() {
  // Leer valores de LDR
  ldr1Value = analogRead(LDR1);
  ldr2Value = analogRead(LDR2);

  // Leer sensor de CO2
  co2Value = analogRead(CO2_PIN);

  // Detectar vehículos con detección de flancos (cada click = +1 vehículo)
  // Dirección 1
  bool cny1 = digitalRead(CNY1);
  bool cny2 = digitalRead(CNY2);
  bool cny3 = digitalRead(CNY3);
  
  // Detectar AMBOS flancos (ascendente y descendente) para funcionar en Wokwi
  if (lastCNY1 != cny1) { vehicleCount1++; Serial.println("CNY1 detectado!"); }
  if (lastCNY2 != cny2) { vehicleCount1++; Serial.println("CNY2 detectado!"); }
  if (lastCNY3 != cny3) { vehicleCount1++; Serial.println("CNY3 detectado!"); }
  
  lastCNY1 = cny1;
  lastCNY2 = cny2;
  lastCNY3 = cny3;
  
  // Dirección 2
  bool cny4 = digitalRead(CNY4);
  bool cny5 = digitalRead(CNY5);
  bool cny6 = digitalRead(CNY6);
  
  if (lastCNY4 != cny4) { vehicleCount2++; Serial.println("CNY4 detectado!"); }
  if (lastCNY5 != cny5) { vehicleCount2++; Serial.println("CNY5 detectado!"); }
  if (lastCNY6 != cny6) { vehicleCount2++; Serial.println("CNY6 detectado!"); }
  
  lastCNY4 = cny4;
  lastCNY5 = cny5;
  lastCNY6 = cny6;
  
  // Resetear contadores cada minuto
  if (millis() - tiempoUltimoReset >= TIEMPO_RESET_CONTADOR) {
    Serial.print("RESET - Total D1: ");
    Serial.print(vehicleCount1);
    Serial.print(" | D2: ");
    Serial.println(vehicleCount2);
    vehicleCount1 = 0;
    vehicleCount2 = 0;
    tiempoUltimoReset = millis();
  }
  
  // Debug en serial
  static unsigned long lastSerialPrint = 0;
  if (millis() - lastSerialPrint > 2000) { // Cada 2 segundos
    Serial.print("Vehiculos D1: "); Serial.print(vehicleCount1);
    Serial.print(" | D2: "); Serial.print(vehicleCount2);
    Serial.print(" | Luz: "); Serial.print((ldr1Value + ldr2Value) / 2);
    Serial.print(" | CO2: "); Serial.println(co2Value);
    lastSerialPrint = millis();
  }
}

void determinarEstado() {
  // PRIORIDAD 1: Verificar CO2 Alto (máxima prioridad ambiental)
  // Este modo NO desaparece hasta que baje el CO2
  if (co2Value > CO2_HIGH_THRESHOLD) {
    estadoActual = EMISION;
  }
  // PRIORIDAD 2: Verificar Modo Nocturno
  else if (ldr1Value < NIGHT_MODE_THRESHOLD && ldr2Value < NIGHT_MODE_THRESHOLD) {
    estadoActual = NOCTURNO;
  } 
  // PRIORIDAD 3: Verificar Tráfico Pesado
  else if (abs(vehicleCount1 - vehicleCount2) >= HEAVY_TRAFFIC_DIFF) {
    estadoActual = TRAFICO_PESADO;
  } 
  // Por defecto Modo Normal
  else {
    estadoActual = NORMAL;
  }
}

void actualizarPantalla() {
  static unsigned long ultimaActualizacion = 0;
  static int ultimoSegundoPeatonal = -1;
  
  // Para modo peatonal, solo actualizar cuando cambie el segundo
  if (estadoActual == PEATONAL) {
    int segundosRestantes = (PEDESTRIAN_CROSS_TIME - (millis() - tiempoInicioModoPeatonal)) / 1000;
    if (segundosRestantes == ultimoSegundoPeatonal) {
      return; // No actualizar si no ha cambiado el segundo
    }
    ultimoSegundoPeatonal = segundosRestantes;
  } else {
    ultimoSegundoPeatonal = -1;
    // Para otros modos, actualizar cada 500ms
    if (millis() - ultimaActualizacion < 500) {
      return;
    }
  }
  
  ultimaActualizacion = millis();
  lcd.clear();
  
  // Línea 1: Modo Actual
  lcd.setCursor(0, 0);
  switch(estadoActual) {
    case NORMAL: lcd.print("Modo: NORMAL"); break;
    case NOCTURNO: lcd.print("Modo: NOCTURNO"); break;
    case TRAFICO_PESADO: lcd.print("Modo: T. PESADO"); break;
    case EMISION: lcd.print("Modo: EMISION CO2"); break;
    case PEATONAL: 
      lcd.print("Modo: PEATONAL ");
      lcd.print(ultimoSegundoPeatonal);
      lcd.print("s");
      break;
  }

  // Línea 2: Datos de Vehículos (acumulados)
  lcd.setCursor(0, 1);
  lcd.print("Vehiculos: D1=");
  lcd.print(vehicleCount1);
  lcd.print(" D2=");
  lcd.print(vehicleCount2);
  lcd.print("  "); // Limpiar caracteres extras
  
  // Línea 3: Datos ambientales
  lcd.setCursor(0, 2);
  lcd.print("Luz: ");
  lcd.print((ldr1Value + ldr2Value) / 2);
  lcd.print("  ");
  
  // Línea 4: CO2 y tiempo hasta reset
  lcd.setCursor(0, 3);
  lcd.print("CO2:");
  lcd.print(co2Value);
  lcd.print(" Reset:");
  lcd.print((TIEMPO_RESET_CONTADOR - (millis() - tiempoUltimoReset)) / 1000);
  lcd.print("s  ");
}


// =================================================================
//  Manejadores de Interrupción
// =================================================================

void manejarSolicitudPeatonal() {
  // Solo establecer bandera - NO ejecutar código bloqueante en interrupción
  solicitudPeatonal = true;
}

// =================================================================
//  Modos de Operación
// =================================================================

void ejecutarModoNormal() {
  // Ciclo estándar: 10s Verde, 3s Amarillo
  ejecutarCicloTrafico(10000, 3000);
}

void ejecutarModoNocturno() {
  // Ciclo más rápido: 6s Verde, 2s Amarillo
  ejecutarCicloTrafico(6000, 2000);
}

void ejecutarModoTraficoPesado() {
  // Priorizar la dirección con más vehículos
  if (vehicleCount1 > vehicleCount2) {
    // Verde más largo para dirección 1
    ejecutarCicloIndividual(1, 15000, 3000); // 15s verde para Semáforo 1
    ejecutarCicloIndividual(2, 5000, 3000);  // 5s verde para Semáforo 2
  } else {
    // Verde más largo para dirección 2
    ejecutarCicloIndividual(1, 5000, 3000);  // 5s verde para Semáforo 1
    ejecutarCicloIndividual(2, 15000, 3000); // 15s verde para Semáforo 2
  }
}

void ejecutarModoEmision() {
  // Modo Reducción de Emisiones: Verde largo para flujo continuo
  // Objetivo: Minimizar paradas = menos ralentí = menos CO2
  // Verde: 20s (muy largo), Amarillo: 2s (corto)
  ejecutarCicloTrafico(20000, 2000);
}

// Modo peatonal ahora se maneja en el loop principal sin delays bloqueantes

// =================================================================
//  Funciones Auxiliares de Control de Semáforos
// =================================================================

// Ejecuta un ciclo completo para ambos semáforos
void ejecutarCicloTrafico(int tiempoVerde, int tiempoAmarillo) {
  ejecutarCicloIndividual(1, tiempoVerde, tiempoAmarillo);
  ejecutarCicloIndividual(2, tiempoVerde, tiempoAmarillo);
}

// Ejecuta una fase de un ciclo de semáforo (Verde -> Amarillo -> Rojo)
void ejecutarCicloIndividual(int direccion, int tiempoVerde, int tiempoAmarillo) {
  if (direccion == 1) {
    // Semáforo 1 Verde, Semáforo 2 Rojo
    digitalWrite(LG1, HIGH);
    digitalWrite(LY1, LOW);
    digitalWrite(LR1, LOW);
    digitalWrite(LR2, HIGH);
    digitalWrite(LY2, LOW);
    digitalWrite(LG2, LOW);
    if (esperaSeguraInterrupcion(tiempoVerde)) return; // Verificar interrupción

    // Semáforo 1 Amarillo, Semáforo 2 Rojo
    digitalWrite(LG1, LOW);
    digitalWrite(LY1, HIGH);
    if (esperaSeguraInterrupcion(tiempoAmarillo)) return; // Verificar interrupción

  } else { // direccion == 2
    // Semáforo 2 Verde, Semáforo 1 Rojo
    digitalWrite(LR1, HIGH);
    digitalWrite(LY1, LOW);
    digitalWrite(LG1, LOW);
    digitalWrite(LG2, HIGH);
    digitalWrite(LY2, LOW);
    digitalWrite(LR2, LOW);
    if (esperaSeguraInterrupcion(tiempoVerde)) return; // Verificar interrupción

    // Semáforo 2 Amarillo, Semáforo 1 Rojo
    digitalWrite(LG2, LOW);
    digitalWrite(LY2, HIGH);
    if (esperaSeguraInterrupcion(tiempoAmarillo)) return; // Verificar interrupción
  }
  
  todasLucesRojas();
  delay(1000); // Espera todo-rojo para seguridad
}

void todasLucesRojas() {
  digitalWrite(LG1, LOW);
  digitalWrite(LY1, LOW);
  digitalWrite(LR1, HIGH);
  digitalWrite(LG2, LOW);
  digitalWrite(LY2, LOW);
  digitalWrite(LR2, HIGH);
}

void parpadearTodasAmarillas(int veces) {
  todasLucesApagadas();
  for (int i = 0; i < veces; i++) {
    digitalWrite(LY1, HIGH);
    digitalWrite(LY2, HIGH);
    delay(500);
    digitalWrite(LY1, LOW);
    digitalWrite(LY2, LOW);
    delay(500);
  }
}

void todasLucesApagadas() {
    digitalWrite(LR1, LOW);
    digitalWrite(LY1, LOW);
    digitalWrite(LG1, LOW);
    digitalWrite(LR2, LOW);
    digitalWrite(LY2, LOW);
    digitalWrite(LG2, LOW);
}

// Función de espera personalizada que verifica interrupciones peatonales
// Retorna verdadero si ocurrió una interrupción, falso en caso contrario
// NOTA: Modo EMISION no puede ser interrumpido ni por peatonales
bool esperaSeguraInterrupcion(int ms) {
  unsigned long inicio = millis();
  while (millis() - inicio < ms) {
    // Solo permitir interrupción peatonal si NO estamos en modo EMISION
    if (estadoActual == PEATONAL && co2Value <= CO2_HIGH_THRESHOLD) {
      return true; // Abortar espera si se presionó botón peatonal
    }
    delay(10); // Pequeña espera para prevenir espera ocupada
  }
  return false;
}
