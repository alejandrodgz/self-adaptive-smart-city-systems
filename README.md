# Sistema de Tráfico Auto-Adaptativo para Ciudad Inteligente

Este repositorio contiene el código fuente y documentación para un sistema de control de tráfico auto-adaptativo para ciudades inteligentes. El proyecto simula una intersección urbana inteligente que adapta dinámicamente sus ciclos de semáforos basándose en datos en tiempo real de varios sensores.

## 🚀 Descripción General

El objetivo principal de este proyecto es crear un sistema de control de tráfico consciente del contexto que pueda ajustar su comportamiento para optimizar el flujo vehicular, priorizar la seguridad peatonal y responder a condiciones ambientales. Esta simulación está construida usando un microcontrolador ESP32-S3 y está diseñada como un paso fundamental hacia infraestructura urbana autónoma más compleja.

Esta primera generación del sistema es un **sistema auto-adaptativo de "bajo nivel"**, lo que significa que opera con un mecanismo de control de lazo cerrado y ajusta sus parámetros basándose en las lecturas de sensores. Aún no tiene la capacidad de aprender de experiencias pasadas ni hacer cambios estructurales a su lógica.

## ✨ Características (Generación 1)

- **Control Consciente del Contexto**: El sistema usa sensores para entender su entorno y tomar decisiones.
- **Múltiples Modos de Operación**: Puede cambiar entre diferentes modos para manejar varios escenarios como horario nocturno, tráfico pesado, emisiones altas de CO2 y cruces peatonales.
- **Ajuste Dinámico de Parámetros**: Los tiempos de los semáforos no son fijos; se ajustan automáticamente según el modo de operación actual.
- **Monitoreo en Tiempo Real**: Una pantalla LCD de 20x4 proporciona retroalimentación en tiempo real sobre el estado del sistema, incluyendo modo actual, estado de semáforos, conteo de vehículos y calidad del aire.
- **Arquitectura No Bloqueante**: Implementa una máquina de estados que permite lectura constante de sensores y respuesta inmediata a cambios de condiciones.

## 🛠️ Componentes del Sistema

### Hardware
- **Microcontrolador**: ESP32-S3 DevKit
- **Semáforos**: 2 conjuntos de LEDs Rojo, Amarillo y Verde para simular una intersección
- **Sensores de Vehículos**: 6 sensores infrarrojos CNY70 para detectar y contar vehículos en dos direcciones (3 por dirección)
- **Botones Peatonales**: 2 pulsadores para que los peatones soliciten señal de cruce
- **Sensores Ambientales**:
  - 2 Fotorresistencias (LDR) para detectar niveles de luz ambiente
  - 1 Sensor de CO2 para monitorear calidad del aire
- **Pantalla**: 1 LCD I2C de 20x4 caracteres

## ⚙️ Modos de Operación

El sistema soporta **cinco modos distintos de operación** con prioridades jerárquicas:

### 1. **MODO NORMAL** (Por Defecto)
- Ciclo de tráfico estándar y balanceado para ambas direcciones
- **Luz Verde**: 10 segundos
- **Luz Amarilla**: 3 segundos
- **Transición Todo-Rojo**: 1 segundo (seguridad)

### 2. **MODO NOCTURNO** (Prioridad 2)
- **Activación**: Cuando ambos sensores LDR detectan poca luz ambiente (< 300)
- Ciclos más rápidos para mejorar eficiencia durante horas de bajo tráfico
- **Luz Verde**: 6 segundos
- **Luz Amarilla**: 2 segundos

### 3. **MODO TRÁFICO PESADO** (Prioridad 3)
- **Activación**: Cuando la diferencia de vehículos entre direcciones es ≥ 3
- El sistema extiende el tiempo de luz verde para la dirección más congestionada
- **Dirección Congestionada**: Verde 15s, Amarillo 3s
- **Dirección Menos Congestionada**: Verde 5s, Amarillo 3s
- Los contadores se resetean cada 60 segundos para reajuste dinámico

### 4. **MODO PEATONAL** (Activado por Solicitud)
- **Activación**: Cuando un peatón presiona cualquier botón de cruce
- **Bloqueo**: No se activa si hay alerta de CO2 alto (prioridad ambiental)
- Todos los semáforos vehiculares se ponen en rojo durante **15 segundos**
- Cuenta regresiva visible en pantalla LCD
- Parpadeo amarillo al finalizar (2 segundos) antes de retornar al ciclo normal
- Respuesta inmediata mediante sistema de interrupciones

### 5. **MODO EMISIÓN CO2** (Prioridad 1 - Máxima)
- **Activación**: Cuando el sensor de CO2 detecta niveles altos (> 600)
- **Objetivo**: Minimizar emisiones vehiculares reduciendo paradas (menos ralentí = menos CO2)
- **Luz Verde**: 20 segundos (muy largo para flujo continuo)
- **Luz Amarilla**: 2 segundos (mínimo)
- Este modo tiene **prioridad absoluta** y no puede ser interrumpido ni por solicitudes peatonales

## 🏗️ Arquitectura del Sistema

### Máquina de Estados del Semáforo
El sistema implementa una máquina de estados no bloqueante con 6 fases:
1. **FASE_TL1_VERDE**: Semáforo 1 en verde, Semáforo 2 en rojo
2. **FASE_TL1_AMARILLO**: Semáforo 1 en amarillo, Semáforo 2 en rojo
3. **FASE_TL1_ROJO**: Transición todo rojo (1 segundo)
4. **FASE_TL2_VERDE**: Semáforo 2 en verde, Semáforo 1 en rojo
5. **FASE_TL2_AMARILLO**: Semáforo 2 en amarillo, Semáforo 1 en rojo
6. **FASE_TL2_ROJO**: Transición todo rojo (1 segundo)

### Ciclo Principal (Loop)
1. **Lectura de Sensores** (cada 50ms): Monitoreo constante de todos los sensores
2. **Procesamiento de Solicitudes Peatonales**: Sistema de interrupciones para respuesta inmediata
3. **Determinación de Modo**: Evaluación de prioridades y cambio de modo si es necesario
4. **Ejecución de Máquina de Estados**: Control de semáforos según modo activo
5. **Actualización de Pantalla** (cada 2 segundos): Información visual del sistema
6. **Reset de Contadores** (cada 60 segundos): Reajuste para adaptación continua

## 📊 Información en Pantalla LCD

La pantalla muestra información en 4 líneas:
- **Línea 1**: Modo de operación actual
- **Línea 2**: Conteo de vehículos por dirección (D1 y D2)
- **Línea 3**: Nivel de luz ambiente y CO2
- **Línea 4**: Estado actual de los semáforos (TL1 y TL2)

## 💻 Cómo Usar

### Versión Original (Bloqueante)
El archivo `gen1_traffic_control.ino` contiene la implementación original con delays bloqueantes.

### Versión Refactorizada (Recomendada)
El archivo `gen1_traffic_control_refactored.ino` contiene la versión optimizada con:
- Arquitectura no bloqueante
- Máquina de estados explícita
- Mejor respuesta a cambios de contexto
- Lectura continua de sensores

### Compilación y Carga
El código puede ser compilado y cargado a una placa ESP32-S3 conectada a los componentes de hardware según el diseño de pines definido en el código.

### Simulación en Wokwi
La simulación también puede ejecutarse en el entorno virtual Wokwi, para lo cual se proporcionan los archivos `wokwi.toml` y `diagram.json`.
