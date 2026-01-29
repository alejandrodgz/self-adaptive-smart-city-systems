# 🚦 Sistema de Tráfico Gen2 - Conectividad WiFi

## Descripción
Esta es la **Generación 2** del sistema de tráfico inteligente. Añade conectividad WiFi para enviar datos de sensores a un servidor central.

## Arquitectura

```
┌─────────────────┐         HTTP POST         ┌─────────────────┐
│     ESP32       │ ────────────────────────► │  Servidor       │
│  (Semáforos)    │    JSON cada 5 seg        │  Python/Flask   │
│                 │ ◄──────────────────────── │                 │
└─────────────────┘      Respuesta OK         └─────────────────┘
                                                      │
                                                      ▼
                                              ┌─────────────────┐
                                              │  Interfaz Web   │
                                              │  Dashboard      │
                                              └─────────────────┘
```

## Configuración Rápida

### 1. Servidor (PC)

```bash
cd server
pip install -r requirements.txt
python server.py
```

El servidor mostrará algo como:
```
🚦 Servidor de Tráfico Inteligente - Generación 2
📡 Iniciando servidor en http://0.0.0.0:5000
```

### 2. Obtener IP de tu PC

En Windows PowerShell:
```powershell
ipconfig
```

Busca la dirección IPv4 (ej: `192.168.1.100`)

### 3. Configurar ESP32

Edita `gen2_traffic_control.ino` y modifica estas líneas:

```cpp
const char* WIFI_SSID = "TU_RED_WIFI";        // Tu red WiFi
const char* WIFI_PASSWORD = "TU_PASSWORD";     // Tu contraseña
const char* SERVER_URL = "http://192.168.1.100:5000/api/traffic";  // IP de tu PC
```

### 4. Subir código al ESP32

Usa Arduino IDE o PlatformIO para compilar y subir el código.

### 5. Verificar comunicación

1. Abre `http://localhost:5000` en tu navegador
2. El ESP32 debería empezar a enviar datos cada 5 segundos
3. Verás los datos actualizarse en el dashboard

## Datos Enviados (JSON)

```json
{
  "estado": "NORMAL",
  "fase": "TL1_VERDE",
  "vehiculos_dir1": 5,
  "vehiculos_dir2": 3,
  "ldr1": 450,
  "ldr2": 460,
  "co2": 320,
  "wifi_rssi": -45
}
```

## Endpoints API

| Método | Endpoint | Descripción |
|--------|----------|-------------|
| POST | `/api/traffic` | Recibe datos del ESP32 |
| GET | `/api/status` | Obtiene último estado |
| GET | `/api/history` | Obtiene historial de eventos |
| GET | `/` | Dashboard web |

## Troubleshooting

### ESP32 no conecta a WiFi
- Verifica SSID y contraseña
- Asegúrate de que la red es 2.4GHz (ESP32 no soporta 5GHz)

### Servidor no recibe datos
- Verifica que el firewall permite conexiones en puerto 5000
- Confirma que la IP del servidor es correcta
- Ambos dispositivos deben estar en la misma red

### Ver logs del ESP32
- Abre el Monitor Serial en Arduino IDE (115200 baud)
