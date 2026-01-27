# Comparación de Niveles: Gen1, Gen2 y Gen3

Este documento describe las diferencias y mejoras entre los tres niveles de implementación del sistema de tráfico autoadaptativo.

## 📊 Resumen de Niveles

### Gen1 - Nivel Bajo (Implementado)
- **Características**: Auto-ajuste y conciencia del contexto
- **Capacidades**: Modifica setpoints según condiciones del entorno
- **Limitaciones**: No aprende de experiencias pasadas, no tiene comunicación externa

### Gen2 - Nivel Medio (Nuevo)
- **Características**: Autoconciencia y autoadaptación mejorada
- **Capacidades**: Comunicación serial con computador para recibir/enviar datos de internet
- **Mejoras**: Integración con datos externos, modos adicionales basados en información de internet

### Gen3 - Nivel Alto (Nuevo)
- **Características**: Componentes intensivos en conocimiento, inferencias y estrategia operacional propia
- **Capacidades**: Aprendizaje de patrones, optimización automática, predicción de tráfico
- **Mejoras**: Auto-regulación avanzada, auto-conciencia mejorada, toma de decisiones inteligente

---

## 🔌 Entradas y Salidas Utilizadas

Todos los niveles utilizan **TODAS** las entradas y salidas de la maqueta:

### Entradas (Inputs):
- ✅ **6 Sensores CNY70** (CNY1-6): Detección de vehículos en dos direcciones
- ✅ **2 Botones Peatonales** (P1, P2): Solicitudes de cruce peatonal
- ✅ **2 Sensores LDR** (LDR1, LDR2): Detección de niveles de luz ambiente
- ✅ **1 Sensor CO2** (CO2_PIN): Monitoreo de calidad del aire

### Salidas (Outputs):
- ✅ **6 LEDs de Semáforos** (LR1, LY1, LG1, LR2, LY2, LG2): Control de semáforos
- ✅ **1 Pantalla LCD I2C** (20x4): Visualización de información del sistema

**NO se agregaron componentes adicionales** - solo se utilizan los existentes.

---

## 🔄 Diferencias Detalladas

### Gen1 (Nivel Bajo)

#### Modos de Operación:
1. **NORMAL**: Ciclo estándar (10s verde, 3s amarillo)
2. **NOCTURNO**: Activado por poca luz (< 300)
3. **TRAFICO_PESADO**: Activado por diferencia ≥ 3 vehículos
4. **PEATONAL**: Activado por botones peatonales
5. **EMISION**: Activado por CO2 alto (> 600)

#### Características:
- Máquina de estados no bloqueante
- Lectura continua de sensores locales
- Ajuste dinámico de tiempos según modo
- Sin comunicación externa
- Sin aprendizaje

#### Limitaciones:
- Umbrales fijos (no se ajustan)
- No aprende de patrones históricos
- No recibe información externa
- Decisiones basadas solo en sensores locales

---

### Gen2 (Nivel Medio)

#### Modos de Operación (Extendidos):
1. **NORMAL**: Ciclo estándar
2. **NOCTURNO**: Activado por poca luz
3. **TRAFICO_PESADO**: Activado por diferencia de vehículos
4. **PEATONAL**: Activado por botones
5. **EMISION**: Activado por CO2 alto
6. **EVENTO_ESPECIAL**: ✨ Nuevo - Activado desde internet
7. **PREDICCION_TRAFICO**: ✨ Nuevo - Basado en predicciones de internet

#### Características Adicionales:
- ✅ **Comunicación Serial Bidireccional**:
  - Recibe comandos desde computador (simulando datos de internet)
  - Envía telemetría del sistema cada 5 segundos
- ✅ **Ajuste Dinámico de Umbrales**:
  - Umbral CO2 ajustable desde internet
  - Umbral de tráfico ajustable desde internet
  - Tiempo verde ajustable desde internet
- ✅ **Modos Basados en Internet**:
  - Modo evento especial (eventos en la ciudad)
  - Modo predicción de tráfico (datos de predicción)
  - Modo emergencia (activado remotamente)
- ✅ **Telemetría en Tiempo Real**:
  - Envía datos en formato JSON simplificado
  - Incluye: vehículos, CO2, luz, modo actual, fase

#### Comandos Serial Soportados:
```
EVENTO:1              -> Activar evento especial
EVENTO:0              -> Desactivar evento especial
PREDICCION:1          -> Predicción de tráfico alto
PREDICCION:0          -> Sin predicción especial
CO2_THRESHOLD:700     -> Ajustar umbral CO2
TRAFICO_THRESHOLD:5   -> Ajustar umbral tráfico
EMERGENCIA:1          -> Activar modo emergencia
VERDE_TIME:12000      -> Ajustar tiempo verde sugerido
```

#### Mejoras sobre Gen1:
- Autoadaptabilidad amplificada con datos externos
- Capacidad de recibir información contextual de internet
- Modos adicionales basados en eventos externos
- Telemetría para monitoreo remoto
- Ajuste remoto de parámetros

---

### Gen3 (Nivel Alto)

#### Modos de Operación (Extendidos):
1. **NORMAL**: Ciclo estándar
2. **NOCTURNO**: Activado por poca luz
3. **TRAFICO_PESADO**: Activado por diferencia de vehículos
4. **PEATONAL**: Activado por botones
5. **EMISION**: Activado por CO2 alto
6. **EVENTO_ESPECIAL**: Desde internet
7. **PREDICCION_TRAFICO**: Basado en predicciones
8. **OPTIMIZADO**: ✨ Nuevo - Modo basado en aprendizaje

#### Características Avanzadas:
- ✅ **Sistema de Aprendizaje**:
  - Historial de patrones (últimos 20 registros)
  - Cálculo de eficiencia en tiempo real
  - Análisis de patrones exitosos
- ✅ **Auto-Optimización**:
  - Ajuste automático de umbrales basado en rendimiento
  - Optimización de tiempos de semáforo
  - Aprendizaje de parámetros óptimos
- ✅ **Predicción Inteligente**:
  - Predicción de tráfico basada en patrones históricos
  - Nivel de confianza en predicciones
  - Anticipación de eventos
- ✅ **Estrategia Operacional Propia**:
  - Toma de decisiones basada en conocimiento acumulado
  - Modo OPTIMIZADO cuando el sistema detecta condiciones ideales
  - Parámetros aprendidos que se ajustan automáticamente
- ✅ **Métricas de Rendimiento**:
  - Eficiencia promedio calculada
  - Análisis de CO2 acumulado
  - Seguimiento de cambios de modo

#### Componentes de Conocimiento:
1. **Registro Historial**: Almacena patrones de operación
2. **Cálculo de Eficiencia**: Evalúa rendimiento en tiempo real
3. **Análisis de Patrones**: Identifica tendencias
4. **Optimización Automática**: Ajusta parámetros para mejorar
5. **Predicción**: Anticipa condiciones futuras

#### Parámetros Aprendidos:
- `umbralTraficoOptimo`: Ajustado automáticamente
- `umbralCO2Optimo`: Optimizado según rendimiento
- `tiempoVerdeOptimo`: Aprendido de patrones exitosos
- `tiempoAmarilloOptimo`: Optimizado
- `factorAjusteNocturno`: Ajustado según eficiencia

#### Mejoras sobre Gen2:
- Aprendizaje de experiencias pasadas
- Optimización automática de parámetros
- Predicción de eventos futuros
- Estrategia operacional propia
- Auto-regulación avanzada
- Auto-conciencia mejorada (sabe qué tan bien está funcionando)

---

## 📈 Progresión de Capacidades

| Característica | Gen1 (Bajo) | Gen2 (Medio) | Gen3 (Alto) |
|----------------|-------------|--------------|-------------|
| Sensores Locales | ✅ | ✅ | ✅ |
| Modos Básicos | ✅ | ✅ | ✅ |
| Comunicación Serial | ❌ | ✅ | ✅ |
| Datos de Internet | ❌ | ✅ | ✅ |
| Ajuste Remoto | ❌ | ✅ | ✅ |
| Historial de Patrones | ❌ | ❌ | ✅ |
| Aprendizaje | ❌ | ❌ | ✅ |
| Auto-Optimización | ❌ | ❌ | ✅ |
| Predicción | ❌ | ❌ | ✅ |
| Estrategia Propia | ❌ | ❌ | ✅ |
| Métricas de Rendimiento | ❌ | ❌ | ✅ |

---

## 🎯 Casos de Uso

### Gen1 - Cuándo Usar:
- Sistema básico sin conectividad
- Entornos donde no hay datos externos disponibles
- Implementación inicial simple

### Gen2 - Cuándo Usar:
- Sistema integrado con infraestructura de ciudad inteligente
- Necesidad de recibir alertas y eventos externos
- Monitoreo remoto requerido
- Ajuste de parámetros desde centro de control

### Gen3 - Cuándo Usar:
- Sistema que necesita mejorar continuamente
- Entornos complejos con patrones variables
- Optimización de largo plazo requerida
- Sistema que debe adaptarse a cambios estacionales o de comportamiento

---

## 🔧 Instalación y Uso

### Gen1:
1. Cargar `gen1_traffic_control/gen1_traffic_control_refactored.ino`
2. No requiere configuración adicional
3. Funciona completamente autónomo

### Gen2:
1. Cargar `gen2_traffic_control/gen2_traffic_control.ino`
2. Conectar ESP32 a computador vía USB
3. Abrir monitor serial (115200 baud)
4. Enviar comandos según formato especificado
5. Recibir telemetría automáticamente

### Gen3:
1. Cargar `gen3_traffic_control/gen3_traffic_control.ino`
2. Conectar ESP32 a computador vía USB
3. Sistema aprende automáticamente después de varios ciclos
4. Optimización automática cada 30 segundos
5. Predicciones activas después de 5 registros históricos

---

## 📝 Notas Técnicas

- Todos los niveles usan los mismos pines y hardware
- Gen2 y Gen3 son compatibles hacia atrás con Gen1
- La comunicación serial en Gen2/Gen3 no interfiere con el funcionamiento local
- Gen3 requiere tiempo para "aprender" (mínimo 5 ciclos para predicciones)
- Los parámetros aprendidos en Gen3 se mantienen durante la ejecución pero se reinician al reiniciar el dispositivo

---

## 🚀 Propuestas para Nivel Alto (Gen3+)

Para llevar el sistema aún más lejos, se podrían implementar:

1. **Persistencia de Aprendizaje**: Guardar parámetros aprendidos en EEPROM/Flash
2. **Red Neuronal Simple**: Para predicciones más avanzadas
3. **Comunicación entre Semáforos**: Coordinación entre múltiples intersecciones
4. **Análisis de Tendencias Temporales**: Patrones por hora del día, día de la semana
5. **Optimización Multi-Objetivo**: Balance entre tráfico, CO2, y tiempo de espera
6. **Aprendizaje por Refuerzo**: Sistema que aprende qué acciones dan mejores resultados
7. **Integración con APIs de Tráfico**: Datos reales de Google Maps, Waze, etc.

