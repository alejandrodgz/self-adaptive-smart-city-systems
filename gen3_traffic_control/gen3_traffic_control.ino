// =================================================================
//  Sistema de Tráfico Ciudad Inteligente - Generación 3 (Nivel Alto)
//  Proyecto: Sistemas Auto-Adaptativos
// =================================================================
//  Sistema con componentes intensivos en conocimiento, inferencias
//  y estrategia operacional propia. Auto-regulación y auto-conciencia avanzada

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -- Definiciones de Pines (MISMOS QUE GEN1 Y GEN2) --
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

// -- Configuración Base (ajustada dinámicamente por aprendizaje) --
int NIGHT_MODE_THRESHOLD = 300;
int HEAVY_TRAFFIC_DIFF = 3;
int PEDESTRIAN_CROSS_TIME = 15000;
int CO2_HIGH_THRESHOLD = 600;
int TIEMPO_RESET_CONTADOR = 60000;

// -- Pantalla LCD --
LiquidCrystal_I2C lcd(0x27, 20, 4);

// -- Estados del Sistema --
enum Estado { NORMAL, NOCTURNO, TRAFICO_PESADO, PEATONAL, EMISION, EVENTO_ESPECIAL, PREDICCION_TRAFICO, OPTIMIZADO };
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
unsigned long ultimoEnvioTelemetria = 0;
unsigned long ultimaLecturaSerial = 0;
unsigned long ultimaOptimizacion = 0;

// =================================================================
//  SISTEMA DE APRENDIZAJE Y CONOCIMIENTO (NIVEL ALTO)
// =================================================================

// -- Historial de Patrones --
#define HISTORIAL_SIZE 20
struct RegistroHistorial {
  unsigned long timestamp;
  int vehiculosD1;
  int vehiculosD2;
  int co2;
  int luzPromedio;
  Estado modoUsado;
  int tiempoVerdeUsado;
  int eficiencia; // 0-100, calculada post-facto
};

RegistroHistorial historial[HISTORIAL_SIZE];
int indiceHistorial = 0;
int totalRegistros = 0;

// -- Métricas de Rendimiento --
struct MetricasRendimiento {
  float eficienciaPromedio;
  int cambiosModoFrecuentes;
  int tiempoEsperaPromedio;
  int co2Acumulado;
};

MetricasRendimiento metricas;

// -- Predicción y Optimización --
struct PrediccionTrafico {
  bool traficoAltoEsperado;
  int tiempoEstimado;
  float confianza; // 0.0 - 1.0
};

PrediccionTrafico prediccionActual;

// -- Parámetros Aprendidos (auto-ajustados) --
struct ParametrosAprendidos {
  int umbralTraficoOptimo;
  int umbralCO2Optimo;
  int tiempoVerdeOptimo;
  int tiempoAmarilloOptimo;
  float factorAjusteNocturno;
};

ParametrosAprendidos parametrosAprendidos;

// =================================================================
//  COMUNICACIÓN SERIAL (EXTENDIDA)
// =================================================================
struct DatosInternet {
  bool eventoEspecial;
  bool prediccionTraficoAlto;
  int umbralCO2Ajustado;
  int umbralTraficoAjustado;
  bool modoEmergencia;
  int tiempoVerdeAjustado;
};

DatosInternet datosInternet;
bool datosInternetValidos = false;
String bufferSerial = "";

// =================================================================
//  FUNCIONES DE APRENDIZAJE Y CONOCIMIENTO
// =================================================================

void inicializarAprendizaje() {
  // Inicializar parámetros aprendidos con valores por defecto
  parametrosAprendidos.umbralTraficoOptimo = HEAVY_TRAFFIC_DIFF;
  parametrosAprendidos.umbralCO2Optimo = CO2_HIGH_THRESHOLD;
  parametrosAprendidos.tiempoVerdeOptimo = 10000;
  parametrosAprendidos.tiempoAmarilloOptimo = 3000;
  parametrosAprendidos.factorAjusteNocturno = 0.6;
  
  // Inicializar métricas
  metricas.eficienciaPromedio = 50.0;
  metricas.cambiosModoFrecuentes = 0;
  metricas.tiempoEsperaPromedio = 0;
  metricas.co2Acumulado = 0;
  
  // Inicializar predicción
  prediccionActual.traficoAltoEsperado = false;
  prediccionActual.tiempoEstimado = 0;
  prediccionActual.confianza = 0.0;
}

void registrarEnHistorial(int vehiculosD1, int vehiculosD2, int co2, int luz, Estado modo, int tiempoVerde) {
  historial[indiceHistorial].timestamp = millis();
  historial[indiceHistorial].vehiculosD1 = vehiculosD1;
  historial[indiceHistorial].vehiculosD2 = vehiculosD2;
  historial[indiceHistorial].co2 = co2;
  historial[indiceHistorial].luzPromedio = luz;
  historial[indiceHistorial].modoUsado = modo;
  historial[indiceHistorial].tiempoVerdeUsado = tiempoVerde;
  historial[indiceHistorial].eficiencia = calcularEficiencia(vehiculosD1, vehiculosD2, co2);
  
  indiceHistorial = (indiceHistorial + 1) % HISTORIAL_SIZE;
  if (totalRegistros < HISTORIAL_SIZE) {
    totalRegistros++;
  }
}

int calcularEficiencia(int v1, int v2, int co2) {
  // Calcular eficiencia basada en:
  // - Balance de tráfico (menos diferencia = mejor)
  // - Niveles de CO2 (menor = mejor)
  // - Flujo continuo (menos cambios = mejor)
  
  int balance = 100 - (abs(v1 - v2) * 10); // Penalizar desbalance
  if (balance < 0) balance = 0;
  
  int co2Score = 100 - (co2 / 10); // Penalizar CO2 alto
  if (co2Score < 0) co2Score = 0;
  if (co2Score > 100) co2Score = 100;
  
  int eficiencia = (balance + co2Score) / 2;
  return eficiencia;
}

void analizarPatrones() {
  if (totalRegistros < 5) return; // Necesitamos datos suficientes
  
  // Analizar patrones en el historial
  int sumaEficiencia = 0;
  int modosNocturnos = 0;
  int modosTraficoPesado = 0;
  int co2Total = 0;
  
  for (int i = 0; i < totalRegistros; i++) {
    sumaEficiencia += historial[i].eficiencia;
    co2Total += historial[i].co2;
    
    if (historial[i].modoUsado == NOCTURNO) modosNocturnos++;
    if (historial[i].modoUsado == TRAFICO_PESADO) modosTraficoPesado++;
  }
  
  metricas.eficienciaPromedio = (float)sumaEficiencia / totalRegistros;
  metricas.co2Acumulado = co2Total / totalRegistros;
  
  // Aprender de patrones exitosos
  if (metricas.eficienciaPromedio > 70) {
    // Sistema funcionando bien, mantener parámetros
  } else if (metricas.eficienciaPromedio < 50) {
    // Sistema necesita ajuste, optimizar parámetros
    optimizarParametros();
  }
}

void optimizarParametros() {
  // Auto-optimización basada en historial
  // Ajustar umbrales y tiempos para mejorar eficiencia
  
  if (totalRegistros < 3) return;
  
  // Analizar qué modos fueron más eficientes
  int eficienciaNocturno = 0, countNocturno = 0;
  int eficienciaTrafico = 0, countTrafico = 0;
  
  for (int i = 0; i < totalRegistros; i++) {
    if (historial[i].modoUsado == NOCTURNO) {
      eficienciaNocturno += historial[i].eficiencia;
      countNocturno++;
    }
    if (historial[i].modoUsado == TRAFICO_PESADO) {
      eficienciaTrafico += historial[i].eficiencia;
      countTrafico++;
    }
  }
  
  // Ajustar umbral de tráfico pesado si es necesario
  if (countTrafico > 0 && eficienciaTrafico / countTrafico < 60) {
    // El modo tráfico pesado no es eficiente, ajustar umbral
    if (HEAVY_TRAFFIC_DIFF > 2) {
      HEAVY_TRAFFIC_DIFF--;
      parametrosAprendidos.umbralTraficoOptimo = HEAVY_TRAFFIC_DIFF;
      Serial.print("OPTIMIZACION: Ajustado umbral trafico a ");
      Serial.println(HEAVY_TRAFFIC_DIFF);
    }
  }
  
  // Ajustar tiempo verde óptimo basado en eficiencia
  int mejorTiempoVerde = 10000;
  int mejorEficiencia = 0;
  
  for (int i = 0; i < totalRegistros; i++) {
    if (historial[i].eficiencia > mejorEficiencia) {
      mejorEficiencia = historial[i].eficiencia;
      mejorTiempoVerde = historial[i].tiempoVerdeUsado;
    }
  }
  
  if (mejorTiempoVerde != parametrosAprendidos.tiempoVerdeOptimo) {
    parametrosAprendidos.tiempoVerdeOptimo = mejorTiempoVerde;
    Serial.print("OPTIMIZACION: Tiempo verde optimo ajustado a ");
    Serial.println(mejorTiempoVerde);
  }
}

void predecirTrafico() {
  // Predecir tráfico basado en patrones históricos
  if (totalRegistros < 5) {
    prediccionActual.traficoAltoEsperado = false;
    prediccionActual.confianza = 0.0;
    return;
  }
  
  // Analizar tendencias recientes
  int vehiculosPromedioD1 = 0;
  int vehiculosPromedioD2 = 0;
  int count = 0;
  
  // Analizar últimos 5 registros
  int inicio = (indiceHistorial - 5 + HISTORIAL_SIZE) % HISTORIAL_SIZE;
  for (int i = 0; i < 5 && i < totalRegistros; i++) {
    int idx = (inicio + i) % HISTORIAL_SIZE;
    vehiculosPromedioD1 += historial[idx].vehiculosD1;
    vehiculosPromedioD2 += historial[idx].vehiculosD2;
    count++;
  }
  
  if (count > 0) {
    vehiculosPromedioD1 /= count;
    vehiculosPromedioD2 /= count;
    
    // Si el promedio reciente es alto, predecir tráfico alto
    int promedioTotal = (vehiculosPromedioD1 + vehiculosPromedioD2) / 2;
    if (promedioTotal > 5) {
      prediccionActual.traficoAltoEsperado = true;
      prediccionActual.confianza = min(0.9, promedioTotal / 10.0);
    } else {
      prediccionActual.traficoAltoEsperado = false;
      prediccionActual.confianza = 0.3;
    }
  }
}

Estado determinarModoInteligente() {
  // Estrategia operacional propia basada en conocimiento
  // Combina datos de sensores, historial y predicciones
  
  // PRIORIDAD 1: CO2 Alto
  if (co2Value > parametrosAprendidos.umbralCO2Optimo) {
    return EMISION;
  }
  
  // PRIORIDAD 2: Predicción de tráfico alto (conocimiento)
  if (prediccionActual.traficoAltoEsperado && prediccionActual.confianza > 0.6) {
    return PREDICCION_TRAFICO;
  }
  
  // PRIORIDAD 3: Modo optimizado (si el sistema aprendió un patrón)
  if (metricas.eficienciaPromedio > 75 && abs(vehicleCount1 - vehicleCount2) < 2) {
    return OPTIMIZADO;
  }
  
  // PRIORIDAD 4: Evento especial (internet)
  if (datosInternet.eventoEspecial || datosInternet.modoEmergencia) {
    return EVENTO_ESPECIAL;
  }
  
  // PRIORIDAD 5: Modo Nocturno
  if (ldr1Value < NIGHT_MODE_THRESHOLD && ldr2Value < NIGHT_MODE_THRESHOLD) {
    return NOCTURNO;
  }
  
  // PRIORIDAD 6: Tráfico Pesado (con umbral aprendido)
  if (abs(vehicleCount1 - vehicleCount2) >= parametrosAprendidos.umbralTraficoOptimo) {
    return TRAFICO_PESADO;
  }
  
  // PRIORIDAD 7: Normal
  return NORMAL;
}

// =================================================================
//  COMUNICACIÓN SERIAL
// =================================================================
void procesarComunicacionSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (bufferSerial.length() > 0) {
        parsearComandoSerial(bufferSerial);
        bufferSerial = "";
      }
    } else {
      bufferSerial += c;
    }
  }
}

void parsearComandoSerial(String comando) {
  if (comando.startsWith("EVENTO:")) {
    datosInternet.eventoEspecial = (comando.substring(7).toInt() == 1);
    datosInternetValidos = true;
  }
  else if (comando.startsWith("PREDICCION:")) {
    datosInternet.prediccionTraficoAlto = (comando.substring(11).toInt() == 1);
    datosInternetValidos = true;
  }
  else if (comando.startsWith("CO2_THRESHOLD:")) {
    int nuevoUmbral = comando.substring(14).toInt();
    if (nuevoUmbral > 0) {
      parametrosAprendidos.umbralCO2Optimo = nuevoUmbral;
      CO2_HIGH_THRESHOLD = nuevoUmbral;
      datosInternetValidos = true;
    }
  }
  else if (comando.startsWith("TRAFICO_THRESHOLD:")) {
    int nuevoUmbral = comando.substring(18).toInt();
    if (nuevoUmbral > 0) {
      parametrosAprendidos.umbralTraficoOptimo = nuevoUmbral;
      HEAVY_TRAFFIC_DIFF = nuevoUmbral;
      datosInternetValidos = true;
    }
  }
  else if (comando.startsWith("EMERGENCIA:")) {
    datosInternet.modoEmergencia = (comando.substring(11).toInt() == 1);
    datosInternetValidos = true;
  }
}

void enviarTelemetria() {
  Serial.print("TELEMETRIA:{");
  Serial.print("\"vehiculos_d1\":"); Serial.print(vehicleCount1); Serial.print(",");
  Serial.print("\"vehiculos_d2\":"); Serial.print(vehicleCount2); Serial.print(",");
  Serial.print("\"co2\":"); Serial.print(co2Value); Serial.print(",");
  Serial.print("\"luz_promedio\":"); Serial.print((ldr1Value + ldr2Value) / 2); Serial.print(",");
  Serial.print("\"eficiencia_promedio\":"); Serial.print(metricas.eficienciaPromedio); Serial.print(",");
  Serial.print("\"modo\":\"");
  switch(estadoActual) {
    case NORMAL: Serial.print("NORMAL"); break;
    case NOCTURNO: Serial.print("NOCTURNO"); break;
    case TRAFICO_PESADO: Serial.print("TRAFICO_PESADO"); break;
    case PEATONAL: Serial.print("PEATONAL"); break;
    case EMISION: Serial.print("EMISION"); break;
    case EVENTO_ESPECIAL: Serial.print("EVENTO_ESPECIAL"); break;
    case PREDICCION_TRAFICO: Serial.print("PREDICCION_TRAFICO"); break;
    case OPTIMIZADO: Serial.print("OPTIMIZADO"); break;
  }
  Serial.print("\",");
  Serial.print("\"prediccion_trafico\":"); Serial.print(prediccionActual.traficoAltoEsperado ? "true" : "false"); Serial.print(",");
  Serial.print("\"confianza_prediccion\":"); Serial.print(prediccionActual.confianza);
  Serial.print("}");
  Serial.println();
}

// =================================================================
void setup() {
  Serial.begin(115200);
  
  inicializarAprendizaje();
  
  datosInternet.eventoEspecial = false;
  datosInternet.prediccionTraficoAlto = false;
  datosInternet.modoEmergencia = false;
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Gen3: Nivel ALTO");
  lcd.setCursor(0, 1);
  lcd.print("Sistema Inteligente");
  delay(2000);
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
  
  Serial.println("GEN3_INICIADO");
}

// =================================================================
void loop() {
  unsigned long tiempoActual = millis();
  
  // ===== PROCESAR COMUNICACIÓN SERIAL =====
  if (tiempoActual - ultimaLecturaSerial >= 100) {
    procesarComunicacionSerial();
    ultimaLecturaSerial = tiempoActual;
  }
  
  // ===== LEER SENSORES =====
  if (tiempoActual - ultimaLecturaSensores >= 50) {
    leerSensores();
    ultimaLecturaSensores = tiempoActual;
  }
  
  // ===== PREDECIR TRÁFICO (cada 10 segundos) =====
  static unsigned long ultimaPrediccion = 0;
  if (tiempoActual - ultimaPrediccion >= 10000) {
    predecirTrafico();
    ultimaPrediccion = tiempoActual;
  }
  
  // ===== ANALIZAR PATRONES Y OPTIMIZAR (cada 30 segundos) =====
  if (tiempoActual - ultimaOptimizacion >= 30000) {
    analizarPatrones();
    optimizarParametros();
    ultimaOptimizacion = tiempoActual;
  }
  
  // ===== ENVIAR TELEMETRÍA =====
  if (tiempoActual - ultimoEnvioTelemetria >= 5000) {
    enviarTelemetria();
    ultimoEnvioTelemetria = tiempoActual;
  }
  
  // ===== MANEJAR SOLICITUD PEATONAL =====
  if (solicitudPeatonal && estadoActual != PEATONAL && co2Value <= parametrosAprendidos.umbralCO2Optimo && !datosInternet.modoEmergencia) {
    activarModoPeatonal();
  } else if (solicitudPeatonal && (co2Value > parametrosAprendidos.umbralCO2Optimo || datosInternet.modoEmergencia)) {
    solicitudPeatonal = false;
  }
  
  // ===== EJECUTAR MODO PEATONAL =====
  if (estadoActual == PEATONAL) {
    ejecutarModoPeatonal(tiempoActual);
    return;
  }
  
  // ===== DETERMINAR MODO (INTELIGENTE) =====
  Estado modoAnterior = estadoActual;
  estadoActual = determinarModoInteligente();
  
  // Registrar cambio de modo en historial
  if (modoAnterior != estadoActual) {
    TiemposModo tiempos = obtenerTiempos();
    registrarEnHistorial(vehicleCount1, vehicleCount2, co2Value, 
                        (ldr1Value + ldr2Value) / 2, estadoActual, tiempos.verdeDir1);
    
    faseActual = FASE_TL1_VERDE;
    tiempoInicioFase = millis();
    Serial.print("CAMBIO DE MODO: ");
    Serial.print(estadoActual);
    Serial.print(" (Eficiencia: ");
    Serial.print(metricas.eficienciaPromedio);
    Serial.println(")");
  }
  
  // ===== EJECUTAR MÁQUINA DE ESTADOS =====
  ejecutarMaquinaEstados(tiempoActual);
  
  // ===== ACTUALIZAR LCD =====
  if (tiempoActual - ultimaActualizacionLCD >= 2000) {
    actualizarLCD();
    ultimaActualizacionLCD = tiempoActual;
  }
  
  // ===== RESETEAR CONTADORES =====
  if (tiempoActual - tiempoUltimoReset >= TIEMPO_RESET_CONTADOR) {
    // Registrar estado final antes de reset
    TiemposModo tiempos = obtenerTiempos();
    registrarEnHistorial(vehicleCount1, vehicleCount2, co2Value, 
                        (ldr1Value + ldr2Value) / 2, estadoActual, tiempos.verdeDir1);
    
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
//  OBTENER TIEMPOS SEGÚN MODO (CON PARÁMETROS APRENDIDOS)
// =================================================================
TiemposModo obtenerTiempos() {
  TiemposModo t;
  
  switch(estadoActual) {
    case NORMAL:
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      break;
      
    case NOCTURNO:
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * parametrosAprendidos.factorAjusteNocturno;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * parametrosAprendidos.factorAjusteNocturno;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      break;
      
    case TRAFICO_PESADO:
      if (vehicleCount1 > vehicleCount2) {
        t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 1.5;
        t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
        t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 0.5;
        t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      } else {
        t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 0.5;
        t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
        t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 1.5;
        t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      }
      break;
      
    case EMISION:
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 2.0;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 2.0;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      break;
      
    case EVENTO_ESPECIAL:
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 1.8;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 1.8;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo * 0.67;
      break;
      
    case PREDICCION_TRAFICO:
      if (vehicleCount1 > vehicleCount2) {
        t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 1.8;
        t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
        t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 0.8;
        t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      } else {
        t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo * 0.8;
        t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
        t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo * 1.8;
        t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      }
      break;
      
    case OPTIMIZADO:
      // Modo optimizado: usar parámetros aprendidos directamente
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
      break;
      
    default:
      t.verdeDir1 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir1 = parametrosAprendidos.tiempoAmarilloOptimo;
      t.verdeDir2 = parametrosAprendidos.tiempoVerdeOptimo;
      t.amarilloDir2 = parametrosAprendidos.tiempoAmarilloOptimo;
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
    digitalWrite(LG1, LOW);
    digitalWrite(LY1, HIGH);
    digitalWrite(LR1, LOW);
    digitalWrite(LG2, LOW);
    digitalWrite(LY2, HIGH);
    digitalWrite(LR2, LOW);
    
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
  
  // Línea 1: Modo
  lcd.setCursor(0, 0);
  switch(estadoActual) {
    case NORMAL: lcd.print("Modo: NORMAL"); break;
    case NOCTURNO: lcd.print("Modo: NOCTURNO"); break;
    case TRAFICO_PESADO: lcd.print("Modo: T. PESADO"); break;
    case EMISION: lcd.print("Modo: EMISION CO2"); break;
    case EVENTO_ESPECIAL: lcd.print("Modo: EVENTO ESP"); break;
    case PREDICCION_TRAFICO: lcd.print("Modo: PREDICCION"); break;
    case OPTIMIZADO: lcd.print("Modo: OPTIMIZADO"); break;
    case PEATONAL: return;
  }
  
  // Línea 2: Vehículos y Eficiencia
  lcd.setCursor(0, 1);
  lcd.print("D1=");
  lcd.print(vehicleCount1);
  lcd.print(" D2=");
  lcd.print(vehicleCount2);
  lcd.print(" E:");
  lcd.print((int)metricas.eficienciaPromedio);
  
  // Línea 3: Ambiente
  lcd.setCursor(0, 2);
  lcd.print("Luz:");
  lcd.print((ldr1Value + ldr2Value) / 2);
  lcd.print(" CO2:");
  lcd.print(co2Value);
  if (prediccionActual.traficoAltoEsperado) {
    lcd.print(" P*");
  }
  
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

