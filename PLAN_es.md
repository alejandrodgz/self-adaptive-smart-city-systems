# Sistema de Tráfico de Ciudad Inteligente - Plan de Implementación
## Proyecto de Sistemas Auto-Adaptativos

---

## 📋 Resumen del Sistema

### Componentes Físicos
- **ESP32-S3 DevKit**: Controlador principal
- **2 Semáforos** (Simulación de intersección):
  - Semáforo 1: LR1 (Rojo), LY1 (Amarillo), LG1 (Verde) - Pines 5, 4, 6
  - Semáforo 2: LR2 (Rojo), LY2 (Amarillo), LG2 (Verde) - Pines 7, 15, 16
- **6 Sensores Infrarrojos (CNY70)**:
  - Dirección 1: CNY1, CNY2, CNY3 (Pines 42, 41, 40) - Detección y conteo de vehículos
  - Dirección 2: CNY4, CNY5, CNY6 (Pines 39, 38, 37) - Detección y conteo de vehículos
- **2 Botones Peatonales**: P1 (Pin 1), P2 (Pin 2) - Solicitudes de cruce
- **2 Sensores de Luz (LDR)**: LDR1 (Pin 13), LDR2 (Pin 12) - Detección de luz ambiental
- **1 Sensor de CO2**: CO2 (Pin 14) - Monitoreo de calidad del aire
- **Pantalla LCD 20x4 I2C**: Visualización de información en tiempo real

### Propósito del Sistema
Simular una intersección urbana inteligente que se adapta a:
- Densidad y patrones de flujo de tráfico
- Necesidades de cruce de peatones
- Condiciones ambientales (niveles de luz, calidad del aire)
- Datos externos de la infraestructura de la ciudad

---

## 🎯 Generación 1: NIVEL BAJO (10 pts)
### **Auto-ajuste + Conciencia del Contexto**

### Características Clave
- ✅ Control de lazo cerrado con setpoints ajustables
- ✅ Conciencia del contexto (entiende las condiciones del entorno)
- ✅ Múltiples Modos de Operación del Sistema (SOM)
- ✅ Ajuste automático de setpoints basado en sensores
- ❌ SIN aprendizaje de la experiencia
- ❌ SIN cambios estructurales

### Estrategia de Implementación

#### **Modos de Operación (4 modos)**

1. **MODO NORMAL** (Por defecto)
   - Ciclo de semáforo estándar
   - Verde1: 10s → Amarillo1: 3s → Rojo1 → Verde2: 10s → Amarillo2: 3s → Rojo2
   - Tiempos equilibrados para ambas direcciones

2. **MODO NOCTURNO** (Luz baja detectada por LDR1 y LDR2)
   - **Ajuste de setpoint**: Ciclos más rápidos, tiempos de espera reducidos
   - Verde1: 6s → Amarillo1: 2s → Rojo1 → Verde2: 6s → Amarillo2: 2s → Rojo2
   - Disparador: `LDR1 < 30% Y LDR2 < 30%`
   - Pantalla: "MODO NOCTURNO"

3. **MODO TRÁFICO PESADO** (Alta densidad de vehículos)
   - **Ajuste de setpoint**: Extender el tiempo de verde para la dirección congestionada
   - Dirección 1 congestionada: Verde1: 15s (Dirección 2: Verde2: 5s)
   - Dirección 2 congestionada: Verde2: 15s (Dirección 1: Verde1: 5s)
   - Disparador: `Diferencia de conteo de vehículos > 3` (detectado por sensores CNY)
   - Pantalla: "TRÁFICO PESADO DIR X"

4. **MODO PRIORIDAD PEATONAL**
   - **Ajuste de setpoint**: Respuesta rápida al botón peatonal
   - Cuando se presiona P1 o P2 → transición inmediata a Rojo para los vehículos
   - Mantener ROJO durante 15 segundos (tiempo de cruce peatonal)
   - Amarillo intermitente antes de volver al ciclo normal
   - Pantalla: "CRUCE PEATONAL"

#### **Lógica de Sensores**

| Sensor | Función | Lógica de Decisión |
|---|---|---|
| **CNY1-3** | Contar vehículos en dirección 1 | Incrementar contador cuando un objeto bloquea el sensor |
| **CNY4-6** | Contar vehículos en dirección 2 | Incrementar contador cuando un objeto bloquea el sensor |
| **LDR1, LDR2** | Detectar día/noche | Si ambos < 30% → MODO NOCTURNO |
| **CO2** | Monitorear calidad del aire | Mostrar nivel de contaminación (BAJO/MEDIO/ALTO) |
| **P1, P2** | Solicitud peatonal | Interrumpir ciclo actual → MODO PEATONAL |

#### **Contenido de la Pantalla LCD**
```
Línea 1: [NOMBRE DEL MODO]
Línea 2: S1:VERDE 10s | S2:ROJO
Línea 3: Autos: 5 | 3
Línea 4: Aire: MEDIO CO2:420
```

#### **Flujo de Control**
```
Bucle:
1. Leer todos los sensores (CNY, LDR, CO2)
2. Comprobar botones peatonales (interrupción)
3. Determinar el modo actual según las condiciones
4. Ajustar los setpoints del semáforo para el modo actual
5. Ejecutar el ciclo del semáforo con los tiempos ajustados
6. Actualizar la pantalla LCD
7. Repetir
```

### Por Qué es Generación 1
✅ **Auto-ajuste**: Cambia los setpoints (tiempos) según las condiciones  
✅ **Conciencia del contexto**: Entiende el entorno (luz, tráfico, peatones)  
✅ **Modos predefinidos**: Todos los comportamientos están pre-programados  
❌ **Sin aprendizaje**: No mejora con la experiencia pasada  
❌ **Sin cambio estructural**: La arquitectura permanece fija  

---

## 🎯 Generación 2: NIVEL MEDIO (20 pts)
### **Auto-adaptación + Auto-conciencia + Sistema de Sistemas**

### Características Clave
- ✅ Todas las características de la Generación 1
- ✅ **Auto-adaptación**: Aprende y modifica su comportamiento con el tiempo
- ✅ **Auto-conciencia**: Entiende su rol en una red de ciudad más grande
- ✅ **Comunicación en serie**: Intercambia datos con sistemas externos
- ✅ **Cambios estructurales**: Puede modificar sus algoritmos de control
- ✅ **Memoria de experiencia**: Almacena y analiza datos históricos

### Estrategia de Implementación

#### **Nuevas Capacidades Añadidas**

1. **Sistema de Aprendizaje - Análisis de Patrones de Tráfico**
   - Almacenar el historial de conteo de vehículos para cada hora del día
   - Array: `historialTrafico[24][2]` → 24 horas, 2 direcciones
   - Después de 1 semana de datos: Identificar horas pico automáticamente
   - **Tiempos adaptativos**: Duración luz verde = f(promedio_histórico)
   - Ejemplo: Si Dir1 históricamente tiene 2x de tráfico a las 8AM → Verde1 = 18s

2. **Comunicación Serial con PC (Integración con Internet)**
   - **Enviar a PC/Nube**:
     ```json
     {
       "id_interseccion": "INT_01",
       "timestamp": 1737676800,
       "vehiculos_dir1": 23,
       "vehiculos_dir2": 15,
       "nivel_co2": 450,
       "modo_actual": "TRÁFICO_PESADO"
     }
     ```
   - **Recibir desde PC**:
     ```json
     {
       "comando": "MODO_EVENTO",
       "tipo_evento": "ESTADIO",
       "direccion_prioritaria": 2,
       "duracion": 120
     }
     ```
   - Ejemplos de integración:
     - API del clima → Lluvia detectada → Aumentar tiempo de luz amarilla (seguridad)
     - Eventos de la ciudad → Concierto cercano → Priorizar dirección específica
     - Vehículo de emergencia → Coordinación de "ola verde"
     - Accidentes de tráfico → Re-enrutar ajustando tiempos

3. **Auto-conciencia en Sistema de Sistemas**
   - Entiende que es la "Intersección A" en una red de tráfico más grande
   - Recibe el estado de intersecciones cercanas vía serial
   - Se coordina con los vecinos:
     - Si la Intersección B está congestionada → Reducir el flujo hacia B
     - Crear "olas verdes" para arterias principales
   - Reporta sus métricas de eficiencia: `tiempo_espera_promedio`, `rendimiento`

4. **Modificación Adaptativa del Comportamiento**
   - **Creación dinámica de modos**: Si un patrón se repite (ej. cada sábado a las 6PM)
     → Crear nuevo modo "SÁBADO_TARDE" automáticamente
   - **Optimización de parámetros**: Ajustar continuamente las relaciones verde/rojo
   - **Cambio estructural**: Habilitar/deshabilitar ciertos sensores si son redundantes

5. **Optimización de Tráfico Basada en CO2**
   - Si CO2 > 600 ppm → modo "REDUCCIÓN_EMISIONES"
   - Minimizar el parar y arrancar (el ralentí contamina más)
   - Luces verdes más largas para mantener el flujo de tráfico
   - Enviar alerta al panel de la ciudad: "Alta contaminación en Intersección A"

#### **Pantalla LCD Mejorada**
```
Línea 1: [ADAPTATIVO] Aprend:ON
Línea 2: S1:12s S2:8s ETA:5s
Línea 3: 1H:15 prom | Red:OK
Línea 4: CO2:520 Modo:APREND_2
```

#### **Estructuras de Datos**
```cpp
struct DatosTrafico {
  int hora;
  int conteo_dir1;
  int conteo_dir2;
  int co2_prom;
  long timestamp;
};

DatosTrafico historial[168]; // 1 semana de datos por hora

struct EstadoRed {
  bool conectado;
  int congestion_vecinos[4]; // 4 intersecciones cercanas
  String comando_ciudad;
};
```

### Por Qué es Generación 2
✅ **Auto-adaptación**: Aprende patrones de tráfico y modifica su comportamiento  
✅ **Auto-conciencia**: Conoce su rol en la red de la ciudad  
✅ **Basado en experiencia**: Optimiza a partir de datos históricos  
✅ **Cambios estructurales**: Puede modificar algoritmos de operación  
✅ **Integración externa**: Recibe/envía datos de internet  

---

## 🎯 Generación 3: NIVEL ALTO (30 pts)
### **Auto-regulación + Inferencia + Estrategia Autónoma**

### Características Clave
- ✅ Todas las características de la Generación 2
- ✅ **Intensivo en conocimiento**: Hace inferencias más allá de los datos directos del sensor
- ✅ **Auto-regulación**: Crea sus propias estrategias operativas
- ✅ **Orientado a objetivos**: Define y persigue objetivos de optimización
- ✅ **Predictivo**: Anticipa estados futuros y actúa proactivamente

### Estrategia de Implementación

#### **Capacidades Avanzadas**

1. **Motor de Inferencia - Más Allá de la Detección Directa**
   - **Predicción de tráfico**: Inferir congestión futura antes de que ocurra
     - Patrón: "Lunes 8AM siempre congestionado" → Pre-ajustar a las 7:50 AM
     - Inferencia en cadena: "Pronóstico de lluvia intensa" → "Más autos" → "Menos peatones"
   - **Detección de anomalías**: 
     - Normal: 20 autos/hora → Actual: 5 autos/hora → Inferir: "Cierre de calle cercano"
     - Acción: Cambiar a modo "BAJA_DEMANDA" automáticamente
   - **Correlación multivariable**:
     - CO2 alto + Tráfico bajo → Inferir: "Vehículos al ralentí" → Ajustar ciclo

2. **Definición y Regulación Autónoma de Objetivos**
   - El sistema define sus propios objetivos de optimización:
     ```cpp
     Objetivos:
     1. Minimizar el tiempo total de espera de todos los vehículos
     2. Mantener el CO2 por debajo de 500 ppm
     3. Espera máxima de peatones < 60 segundos
     4. Equilibrar el rendimiento entre direcciones (justicia)
     ```
   - **Auto-regulación**: Monitorea el logro de objetivos
     - Si el Objetivo 2 falla → Aumentar la prioridad en la optimización del flujo
     - Si el Objetivo 3 falla → Fases peatonales más frecuentes
   - **Identificación de riesgos**: "Larga cola en Dir1 + Evento en 30 min = Alto riesgo de atasco"

3. **Modelado Predictivo de Tráfico**
   - Modelo simple de machine learning (o predicción basada en reglas):
     ```
     Trafico_Futuro(t+15min) = f(trafico_actual, hora_del_dia, dia_semana, eventos, clima)
     ```
   - **Acciones proactivas**:
     - Congestión predicha → Ajustar tiempos 10 minutos antes
     - Tráfico bajo predicho → Entrar en modo de ahorro de energía (luces más tenues)

4. **Toma de Decisiones Estratégica**
   - **Planificación multi-paso**:
     - Escenario: "Evento en el estadio termina en 20 minutos"
     - Estrategia: 
       1. Aumentar gradualmente el tiempo de verde para la dirección del estadio
       2. Coordinar con intersecciones adyacentes
       3. Prepararse para un período de alto flujo de 30 min
       4. Volver a la normalidad gradualmente (no bruscamente)
   - **Selección de estrategia adaptativa**:
     - Hora pico matutina: Estrategia "MÁXIMO_RENDIMIENTO"
     - Horario escolar: Estrategia "SEGURIDAD_PRIMERO" (fases peatonales más largas)
     - Noche: Estrategia "AHORRO_ENERGÍA"

5. **Coordinación Avanzada de Sistema de Sistemas**
   - **Comportamiento emergente**: Múltiples intersecciones se coordinan sin control central
   - **Optimización de ola verde**: Calcular desfases de tiempo óptimos
   - **Balanceo de carga**: Distribuir el tráfico por rutas alternativas
   - Enviar recomendaciones estratégicas: "Sugerir desvío por Calle Alternativa"

6. **Gestión Sofisticada de la Calidad del Aire**
   - **Inferencia**: CO2 alto → Calcular densidad de vehículos por minuto
   - **Estrategia**: Crear "períodos de respiración" (todo en rojo brevemente para limpiar el aire)
   - **Coordinación**: Compartir calidad del aire con vecinos → Reducción de emisiones a nivel ciudad

#### **Ejemplo de Base de Conocimiento**
```cpp
Reglas:
- SI (dia == LUNES && hora == 8 && historial_dir1 > 30) 
  ENTONCES predecir_alta_congestion(dir1) 
  Y aplicar_estrategia("PRIORIDAD_PREEMPTIVA_DIR1")

- SI (co2 > 600 && tiempo_espera > 45s) 
  ENTONCES inferir("patron_parar_y_arrancar") 
  Y cambiar_a("OPTIMIZACION_FLUJO")

- SI (anomalia_detectada() && intersecciones_vecinas_normales()) 
  ENTONCES inferir("incidente_local") 
  Y enviar_alerta("POSIBLE_ACCIDENTE_INT_A")
```

#### **Bucle de Control Auto-Regulador**
```
1. Monitorear: Recolectar todos los datos de sensores + datos externos
2. Analizar: Aplicar reglas de inferencia, predecir estados futuros
3. Planificar: Generar estrategia óptima basada en objetivos
4. Ejecutar: Implementar la estrategia de control de tráfico
5. Evaluar: Comprobar si se lograron los objetivos
6. Aprender: Actualizar la base de conocimiento y los modelos
7. Repetir
```

### Por Qué es Generación 3
✅ **Intensivo en conocimiento**: Motor de inferencia razona más allá de los datos del sensor  
✅ **Auto-regulación**: Define sus propios objetivos y estrategias  
✅ **Predictivo**: Anticipa y actúa proactivamente  
✅ **Toma de decisiones autónoma**: Sin reglas predefinidas para todos los escenarios  
✅ **Planificación estratégica**: Optimización multi-paso  

---

## 📊 Tabla Comparativa

| Característica | Gen 1 | Gen 2 | Gen 3 |
|---|---|---|---|
| **Control de lazo cerrado** | ✅ | ✅ | ✅ |
| **Auto-ajuste** | ✅ | ✅ | ✅ |
| **Conciencia del contexto** | ✅ | ✅ | ✅ |
| **Modos predefinidos** | ✅ | ✅ | ✅ |
| **Aprendizaje de experiencia** | ❌ | ✅ | ✅ |
| **Auto-adaptación** | ❌ | ✅ | ✅ |
| **Auto-conciencia (SdS)** | ❌ | ✅ | ✅ |
| **Com. Serial/Internet** | ❌ | ✅ | ✅ |
| **Cambios estructurales** | ❌ | ✅ | ✅ |
| **Motor de inferencia** | ❌ | ❌ | ✅ |
| **Auto-regulación** | ❌ | ❌ | ✅ |
| **Definición de objetivos** | ❌ | ❌ | ✅ |
| **Comportamiento predictivo** | ❌ | ❌ | ✅ |
| **Planificación estratégica** | ❌ | ❌ | ✅ |

---

## 🛠️ Hoja de Ruta de Implementación

### Fase 1: Generación 1 (Semana 1)
1. ✅ Configurar conexiones de hardware y probar todos los sensores
2. ✅ Implementar ciclo básico de semáforo
3. ✅ Añadir funciones de lectura de sensores (CNY, LDR, CO2, botones)
4. ✅ Implementar 4 modos de operación con cambio de contexto
5. ✅ Integración de la pantalla LCD
6. ✅ Probar y depurar transiciones de modo

### Fase 2: Generación 2 (Semana 2)
1. ✅ Añadir estructuras de almacenamiento de datos (historial de tráfico)
2. ✅ Implementar algoritmo de aprendizaje (detección de patrones)
3. ✅ Configurar protocolo de comunicación serial
4. ✅ Crear programa en PC para intercambio de datos
5. ✅ Implementar tiempos adaptativos basados en patrones aprendidos
6. ✅ Probar características de conciencia de sistema de sistemas
7. ✅ Optimización basada en CO2

### Fase 3: Generación 3 (Semana 3)
1. ✅ Diseñar arquitectura del motor de inferencia
2. ✅ Implementar modelos de predicción
3. ✅ Crear base de conocimiento y reglas
4. ✅ Desarrollar mecanismos de establecimiento y regulación de objetivos
5. ✅ Implementar algoritmos de planificación estratégica
6. ✅ Características de coordinación avanzadas
7. ✅ Pruebas y optimización exhaustivas

### Fase 4: Preparación de la Presentación
1. ✅ Crear demostraciones comparativas
2. ✅ Preparar diapositivas mostrando la evolución Gen1→Gen2→Gen3
3. ✅ Documentar mejoras en cada nivel
4. ✅ Preparar propuestas para futuras mejoras
5. ✅ Demo final del sistema

---

## 🎓 Estructura de la Presentación

### 1. Introducción (3 min)
- Problema de la ciudad inteligente: Congestión de tráfico, contaminación, seguridad
- Nuestra solución: Sistema de gestión de tráfico adaptativo

### 2. Demo Generación 1 (5 min)
- Mostrar 4 modos de operación en acción
- Demostrar comportamiento consciente del contexto
- Explicar limitaciones (sin aprendizaje, modos fijos)

### 3. Demo Generación 2 (5 min)
- Mostrar aprendizaje a lo largo del tiempo (gráfico de patrones de tráfico)
- Demostrar comunicación serial con PC
- Mostrar ajustes de tiempo adaptativos
- Explicar la auto-conciencia en la red

### 4. Demo Generación 3 (7 min)
- Demostrar comportamiento predictivo
- Mostrar motor de inferencia tomando decisiones
- Planificación estratégica para eventos
- Optimización orientada a objetivos

### 5. Comparación y Evolución (3 min)
- Tabla de comparación lado a lado
- Resaltar mejoras clave en cada nivel

### 6. Propuestas Futuras - Camino a la Generación 4 (2 min)
- **Propuesta 1**: Aprendizaje multi-agente (las intersecciones se enseñan entre sí)
- **Propuesta 2**: Auto-organización durante emergencias (desvío autónomo)
- **Propuesta 3**: Auto-reproducción (intersecciones virtuales se generan para simulación)
- **Propuesta 4**: Inteligencia de enjambre para optimización a nivel de ciudad

### 7. Preguntas y Respuestas (5 min)

---

## 💡 Propuestas para la Generación 4 (Nivel Alto)

### ¿Qué se Necesitaría?

1. **Aprendizaje por Refuerzo Multi-Agente**
   - Cada intersección es un agente autónomo
   - Aprende políticas óptimas a través de prueba y error
   - Sin controlador central, pura emergencia
   - **Requisitos**: Más potencia de cómputo, librerías de ML, entorno de simulación

2. **Capacidades de Auto-Organización**
   - Durante emergencias, las intersecciones se reorganizan autónomamente
   - Forman nuevos patrones de coordinación sin protocolos predefinidos
   - **Requisitos**: Algoritmos de consenso distribuido, detección de emergencias

3. **Integración con Gemelo Digital**
   - Réplica virtual en tiempo real de la intersección
   - Simular estrategias antes de aplicarlas
   - **Requisitos**: Computación en la nube, motor de simulación en tiempo real

4. **Auto-Reproducción para Escalabilidad**
   - El sistema puede generar copias virtuales para pruebas
   - Evolucionar múltiples estrategias en paralelo
   - La versión con mejor rendimiento se despliega automáticamente
   - **Requisitos**: Contenerización, plataforma de orquestación

5. **Inteligencia de Enjambre**
   - Miles de intersecciones se coordinan como una colonia de hormigas
   - Optimización emergente a nivel de toda la ciudad
   - **Requisitos**: Computación distribuida, red de baja latencia

---

## 📝 Estructura del Código

```
auto_adaptable/
├── sketch.ino                    # Código principal de Arduino
├── gen1_control_trafico.h        # Implementación Generación 1
├── gen2_sistema_adaptativo.h     # Implementación Generación 2
├── gen3_sistema_inteligente.h    # Implementación Generación 3
├── sensores.h                     # Funciones de lectura de sensores
├── actuadores.h                   # Control de LEDs y pantalla
├── comunicacion_pc.py             # Script de Python para el lado del PC (Gen 2+)
├── dashboard_web.html             # Interfaz web para monitoreo
├── diagram.json                   # Diagrama del circuito en Wokwi
├── wokwi.toml                     # Configuración de Wokwi
├── PLAN_es.md                     # Este archivo
├── PRESENTACION.pptx              # Diapositivas de la presentación final
└── README.md                      # Documentación del proyecto
```

---

## ✅ Criterios de Éxito

- [ ] Los 6 LEDs controlados correctamente
- [ ] Los 6 sensores CNY leyendo vehículos
- [ ] Ambos botones peatonales funcionando
- [ ] Sensores LDR detectando día/noche
- [ ] Sensor de CO2 monitoreando calidad del aire
- [ ] LCD mostrando información relevante
- [ ] Generación 1: 4 modos de operación funcionando sin problemas
- [ ] Generación 2: Aprendizaje y comunicación serial funcionales
- [ ] Generación 3: Inferencia y predicción demostrables
- [ ] Presentación clara y profesional
- [ ] La demo se ejecuta sin errores

---

**Próximos Pasos**: 
1. Revisar y aprobar este plan
2. Comenzar a implementar el código de la Generación 1
3. Probar las conexiones de hardware
4. Proceder sistemáticamente a través de cada generación

¿Estás listo para empezar a programar? 🚀
