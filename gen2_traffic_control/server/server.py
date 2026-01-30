"""
=================================================================
 Servidor Simple para Sistema de Tráfico - Generación 2
 Recibe datos del ESP32 via HTTP POST
=================================================================
 Ejecutar: python server.py
 El servidor escuchará en http://0.0.0.0:5000
"""

from flask import Flask, request, jsonify
from datetime import datetime
import json
import csv
import os

app = Flask(__name__)

# Almacena el último estado recibido
ultimo_estado = {}
historial = []

# Comando pendiente para enviar al ESP32
comando_pendiente = None

# =================================================================
#  SISTEMA ADAPTATIVO INTELIGENTE - "CEREBRO" DEL TRÁFICO
# =================================================================
modo_automatico = False  # Toggle para activar/desactivar
ultimo_comando_auto = None
razon_comando = ""
historial_decisiones = []

# Setpoints actuales del ESP32 (se actualizan con cada mensaje)
setpoints_actuales = {
    'sp_verde_normal': 10000,
    'sp_peatonal': 15000,
    'sp_verde_pesado_max': 15000,
    'sp_verde_pesado_min': 5000
}

# Historial para análisis de patrones
historial_peatonal = []  # Timestamps de activaciones peatonales
historial_desbalance = []  # Últimos ratios D1/D2

# Configuración del sistema adaptativo
CONFIG_ADAPTATIVO = {
    # Porcentajes de ajuste (escalado y desescalado)
    'porcentaje_ajuste': 0.15,            # 15% de incremento/decremento
    
    # Regla Peatonal
    'umbral_peatonal_alto': 3,            # ≥3 activaciones = aumentar
    'umbral_peatonal_bajo': 1,            # ≤1 activación = reducir
    'ventana_peatonal_minutos': 30,       # Ventana de tiempo
    'peatonal_base_ms': 15000,            # Valor base (para volver)
    'peatonal_max_ms': 25000,             # Máximo tiempo peatonal
    'peatonal_min_ms': 10000,             # Mínimo tiempo peatonal
    
    # Regla Desbalance
    'umbral_desbalance_alto': 0.70,       # >70% = desbalance fuerte
    'umbral_desbalance_bajo': 0.55,       # <55% = equilibrado (desescalar)
    'registros_desbalance': 10,           # Registros para evaluar (reducido para pruebas)
    'verde_base_ms': 15000,               # Valor base
    'verde_max_ms': 22000,                # Máximo tiempo verde
    'verde_min_ms': 10000,                # Mínimo tiempo verde
    
    # Control de frecuencia de ajustes
    'cooldown_ajuste_segundos': 60        # Mínimo 60s entre ajustes
}

# Último ajuste realizado (para cooldown)
ultimo_ajuste_timestamp = None

def actualizar_setpoints(data):
    """Actualiza los setpoints actuales desde los datos del ESP32"""
    global setpoints_actuales
    if 'sp_verde_normal' in data:
        setpoints_actuales['sp_verde_normal'] = data['sp_verde_normal']
    if 'sp_peatonal' in data:
        setpoints_actuales['sp_peatonal'] = data['sp_peatonal']
    if 'sp_verde_pesado_max' in data:
        setpoints_actuales['sp_verde_pesado_max'] = data['sp_verde_pesado_max']
    if 'sp_verde_pesado_min' in data:
        setpoints_actuales['sp_verde_pesado_min'] = data['sp_verde_pesado_min']

def registrar_activacion_peatonal(data):
    """Registra cuando se activa el modo peatonal"""
    global historial_peatonal
    contador = data.get('contador_peatonal', 0)
    
    # Si el contador aumentó, registrar la activación
    if historial_peatonal:
        ultimo = historial_peatonal[-1]
        if contador > ultimo.get('contador', 0):
            historial_peatonal.append({
                'timestamp': datetime.now(),
                'contador': contador
            })
    elif contador > 0:
        historial_peatonal.append({
            'timestamp': datetime.now(),
            'contador': contador
        })
    
    # Mantener solo últimas 50 activaciones
    if len(historial_peatonal) > 50:
        historial_peatonal.pop(0)

def registrar_desbalance(data):
    """Registra el ratio de tráfico para análisis de desbalance"""
    global historial_desbalance
    d1 = data.get('vehiculos_dir1', 0)
    d2 = data.get('vehiculos_dir2', 0)
    total = d1 + d2
    
    if total > 0:
        ratio_d1 = d1 / total
        historial_desbalance.append({
            'timestamp': datetime.now(),
            'ratio_d1': ratio_d1,
            'd1': d1,
            'd2': d2
        })
        print(f"📊 Registrado: D1={d1}, D2={d2}, Ratio D1={ratio_d1*100:.1f}% | Historial: {len(historial_desbalance)} registros")
    else:
        print(f"⚠️ Sin vehículos: D1={d1}, D2={d2}")
    
    # Mantener solo últimos N registros
    max_registros = CONFIG_ADAPTATIVO['registros_desbalance']
    if len(historial_desbalance) > max_registros:
        historial_desbalance.pop(0)

def analizar_y_decidir(data):
    """
    🧠 CEREBRO ADAPTATIVO: Analiza patrones históricos y ajusta setpoints.
    Usa porcentajes para escalar y desescalar según las condiciones.
    """
    global ultimo_comando_auto, razon_comando, historial_decisiones, ultimo_ajuste_timestamp
    
    if not modo_automatico:
        return None, None
    
    # Actualizar datos
    actualizar_setpoints(data)
    registrar_activacion_peatonal(data)
    registrar_desbalance(data)
    
    # Verificar cooldown entre ajustes
    ahora = datetime.now()
    if ultimo_ajuste_timestamp:
        segundos_desde_ajuste = (ahora - ultimo_ajuste_timestamp).total_seconds()
        if segundos_desde_ajuste < CONFIG_ADAPTATIVO['cooldown_ajuste_segundos']:
            return None, None
    
    comando = None
    razon = ""
    porcentaje = CONFIG_ADAPTATIVO['porcentaje_ajuste']
    
    # =========================================================
    # REGLA 1: Ajuste de tiempo PEATONAL (escalar/desescalar)
    # =========================================================
    ventana_minutos = CONFIG_ADAPTATIVO['ventana_peatonal_minutos']
    
    activaciones_recientes = [
        h for h in historial_peatonal 
        if (ahora - h['timestamp']).total_seconds() < ventana_minutos * 60
    ]
    num_activaciones = len(activaciones_recientes)
    
    sp_peatonal = setpoints_actuales['sp_peatonal']
    
    # ESCALAR: Muchas activaciones → aumentar tiempo
    if num_activaciones >= CONFIG_ADAPTATIVO['umbral_peatonal_alto']:
        if sp_peatonal < CONFIG_ADAPTATIVO['peatonal_max_ms']:
            incremento = int(sp_peatonal * porcentaje)
            nuevo_valor = min(sp_peatonal + incremento, CONFIG_ADAPTATIVO['peatonal_max_ms'])
            comando = f"AJUSTAR:SP_PEATONAL:{nuevo_valor}"
            razon = f"📈 Peatonal frecuente ({num_activaciones} en {ventana_minutos}min) → +{porcentaje*100:.0f}%: {sp_peatonal/1000:.1f}s → {nuevo_valor/1000:.1f}s"
    
    # DESESCALAR: Pocas activaciones → reducir tiempo (volver al base)
    elif num_activaciones <= CONFIG_ADAPTATIVO['umbral_peatonal_bajo']:
        if sp_peatonal > CONFIG_ADAPTATIVO['peatonal_min_ms']:
            decremento = int(sp_peatonal * porcentaje)
            nuevo_valor = max(sp_peatonal - decremento, CONFIG_ADAPTATIVO['peatonal_min_ms'])
            if nuevo_valor < sp_peatonal:  # Solo si realmente cambia
                comando = f"AJUSTAR:SP_PEATONAL:{nuevo_valor}"
                razon = f"📉 Peatonal bajo ({num_activaciones} en {ventana_minutos}min) → -{porcentaje*100:.0f}%: {sp_peatonal/1000:.1f}s → {nuevo_valor/1000:.1f}s"
    
    # =========================================================
    # REGLA 2: Ajuste de tiempo VERDE (escalar/desescalar)
    # =========================================================
    registros_necesarios = CONFIG_ADAPTATIVO['registros_desbalance']
    registros_actuales = len(historial_desbalance)
    
    print(f"🔍 Desbalance: {registros_actuales}/{registros_necesarios} registros")
    
    if not comando and registros_actuales >= registros_necesarios:
        promedio_ratio_d1 = sum(h['ratio_d1'] for h in historial_desbalance) / len(historial_desbalance)
        
        sp_verde = setpoints_actuales['sp_verde_pesado_max']
        umbral_alto = CONFIG_ADAPTATIVO['umbral_desbalance_alto']
        umbral_bajo = CONFIG_ADAPTATIVO['umbral_desbalance_bajo']
        
        print(f"🔍 Ratio D1: {promedio_ratio_d1*100:.1f}% | Umbral alto: >{umbral_alto*100:.0f}% | SP Verde: {sp_verde}ms")
        
        # ESCALAR: Desbalance fuerte → aumentar tiempo verde
        # D1 domina (>70%) o D2 domina (D1 < 30%)
        if promedio_ratio_d1 > umbral_alto or promedio_ratio_d1 < (1 - umbral_alto):
            direccion = "D1" if promedio_ratio_d1 > 0.5 else "D2"
            pct_dominante = max(promedio_ratio_d1, 1 - promedio_ratio_d1) * 100
            
            print(f"⚠️ DESBALANCE DETECTADO: {direccion} domina con {pct_dominante:.0f}%")
            
            if sp_verde < CONFIG_ADAPTATIVO['verde_max_ms']:
                incremento = int(sp_verde * porcentaje)
                nuevo_valor = min(sp_verde + incremento, CONFIG_ADAPTATIVO['verde_max_ms'])
                comando = f"AJUSTAR:SP_VERDE_PESADO_MAX:{nuevo_valor}"
                razon = f"📈 {direccion} domina ({pct_dominante:.0f}%) → +{porcentaje*100:.0f}%: {sp_verde/1000:.1f}s → {nuevo_valor/1000:.1f}s"
                print(f"✅ COMANDO GENERADO: {comando}")
            else:
                print(f"⚠️ No se ajusta: ya en máximo ({sp_verde}ms >= {CONFIG_ADAPTATIVO['verde_max_ms']}ms)")
        
        # DESESCALAR: Tráfico equilibrado (entre 45% y 55%) → reducir tiempo verde
        elif (1 - umbral_bajo) <= promedio_ratio_d1 <= umbral_bajo:
            print(f"📊 Tráfico equilibrado: {promedio_ratio_d1*100:.1f}%")
            if sp_verde > CONFIG_ADAPTATIVO['verde_min_ms']:
                decremento = int(sp_verde * porcentaje)
                nuevo_valor = max(sp_verde - decremento, CONFIG_ADAPTATIVO['verde_min_ms'])
                if nuevo_valor < sp_verde:
                    comando = f"AJUSTAR:SP_VERDE_PESADO_MAX:{nuevo_valor}"
                    razon = f"📉 Tráfico equilibrado ({promedio_ratio_d1*100:.0f}%/{(1-promedio_ratio_d1)*100:.0f}%) → -{porcentaje*100:.0f}%: {sp_verde/1000:.1f}s → {nuevo_valor/1000:.1f}s"
                    print(f"✅ COMANDO GENERADO: {comando}")
        else:
            print(f"📊 Sin acción: ratio {promedio_ratio_d1*100:.1f}% no cumple umbrales")
    
    # Evitar enviar el mismo comando repetidamente
    if comando and comando == ultimo_comando_auto:
        return None, None
    
    if comando:
        ultimo_comando_auto = comando
        razon_comando = razon
        ultimo_ajuste_timestamp = ahora  # Registrar para cooldown
        
        # Guardar en historial de decisiones
        historial_decisiones.append({
            'timestamp': ahora.strftime('%H:%M:%S'),
            'comando': comando.split(':')[0] + ':' + comando.split(':')[1],
            'razon': razon
        })
        if len(historial_decisiones) > 20:
            historial_decisiones.pop(0)
    
    return comando, razon

# Archivo CSV para historial
CSV_FILE = "traffic_data.csv"
CSV_COLUMNS = ["timestamp", "estado", "fase", "vehiculos_dir1", "vehiculos_dir2", "ldr1", "ldr2", "co2", "wifi_rssi"]

def inicializar_csv():
    """Crea el archivo CSV con encabezados si no existe"""
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(CSV_COLUMNS)
        print(f"📁 Archivo {CSV_FILE} creado")
    else:
        print(f"📁 Archivo {CSV_FILE} encontrado")

def guardar_en_csv(data):
    """Guarda un registro en el archivo CSV"""
    try:
        with open(CSV_FILE, 'a', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            row = [
                data.get('timestamp', ''),
                data.get('estado', ''),
                data.get('fase', ''),
                data.get('vehiculos_dir1', 0),
                data.get('vehiculos_dir2', 0),
                data.get('ldr1', 0),
                data.get('ldr2', 0),
                data.get('co2', 0),
                data.get('wifi_rssi', 0)
            ]
            writer.writerow(row)
    except Exception as e:
        print(f"❌ Error guardando CSV: {e}")

# =================================================================
#  ANÁLISIS DE DATOS EN TIEMPO REAL
# =================================================================
# Almacena datos para estadísticas (últimos 100 registros)
datos_analisis = []

def agregar_dato_analisis(data):
    """Agrega dato para análisis y mantiene solo los últimos 100"""
    datos_analisis.append({
        'timestamp': data.get('timestamp'),
        'estado': data.get('estado'),
        'vehiculos_dir1': data.get('vehiculos_dir1', 0),
        'vehiculos_dir2': data.get('vehiculos_dir2', 0),
        'co2': data.get('co2', 0),
        'ldr1': data.get('ldr1', 0),
        'ldr2': data.get('ldr2', 0)
    })
    if len(datos_analisis) > 100:
        datos_analisis.pop(0)

def calcular_estadisticas():
    """Calcula estadísticas de los datos recolectados"""
    if not datos_analisis:
        return None
    
    total_dir1 = sum(d['vehiculos_dir1'] for d in datos_analisis)
    total_dir2 = sum(d['vehiculos_dir2'] for d in datos_analisis)
    total_vehiculos = total_dir1 + total_dir2
    
    # Promedios
    n = len(datos_analisis)
    avg_co2 = sum(d['co2'] for d in datos_analisis) / n
    avg_luz = sum((d['ldr1'] + d['ldr2']) / 2 for d in datos_analisis) / n
    
    # Conteo de estados
    estados_count = {}
    for d in datos_analisis:
        estado = d['estado']
        estados_count[estado] = estados_count.get(estado, 0) + 1
    
    # Estado más frecuente
    estado_frecuente = max(estados_count, key=estados_count.get) if estados_count else "N/A"
    
    # Porcentaje de tráfico por dirección
    pct_dir1 = (total_dir1 / total_vehiculos * 100) if total_vehiculos > 0 else 50
    pct_dir2 = (total_dir2 / total_vehiculos * 100) if total_vehiculos > 0 else 50
    
    # Dirección dominante
    if total_dir1 > total_dir2 * 1.2:
        direccion_dominante = "Dirección 1 (+20%)"
    elif total_dir2 > total_dir1 * 1.2:
        direccion_dominante = "Dirección 2 (+20%)"
    else:
        direccion_dominante = "Equilibrado"
    
    # Recomendación
    if pct_dir1 > 60:
        recomendacion = "⚡ Aumentar tiempo verde Dir1"
    elif pct_dir2 > 60:
        recomendacion = "⚡ Aumentar tiempo verde Dir2"
    elif avg_co2 > 400:
        recomendacion = "🌿 CO2 alto - ciclos largos activos"
    else:
        recomendacion = "✅ Sistema balanceado"
    
    return {
        'total_registros': n,
        'total_vehiculos': total_vehiculos,
        'total_dir1': total_dir1,
        'total_dir2': total_dir2,
        'pct_dir1': round(pct_dir1, 1),
        'pct_dir2': round(pct_dir2, 1),
        'avg_co2': round(avg_co2, 1),
        'avg_luz': round(avg_luz, 1),
        'estados_count': estados_count,
        'estado_frecuente': estado_frecuente,
        'direccion_dominante': direccion_dominante,
        'recomendacion': recomendacion
    }

@app.route('/')
def index():
    """Página principal con estado actual"""
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Sistema de Tráfico Gen2</title>
        <meta http-equiv="refresh" content="5">
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; background: #1a1a2e; color: #eee; }
            .container { max-width: 800px; margin: 0 auto; }
            h1 { color: #00d4ff; }
            .card { background: #16213e; padding: 20px; border-radius: 10px; margin: 20px 0; }
            .status { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; }
            .stat { background: #0f3460; padding: 15px; border-radius: 8px; }
            .stat-label { color: #888; font-size: 12px; }
            .stat-value { font-size: 24px; font-weight: bold; color: #00d4ff; }
            .estado-NORMAL { color: #00ff88; }
            .estado-NOCTURNO { color: #ffaa00; }
            .estado-TRAFICO_PESADO { color: #ff6600; }
            .estado-PEATONAL { color: #00aaff; }
            .estado-EMISION { color: #ff4444; }
            .historial { font-size: 12px; max-height: 300px; overflow-y: auto; }
            .historial-item { padding: 5px; border-bottom: 1px solid #333; }
            .controls { display: flex; gap: 10px; flex-wrap: wrap; }
            .btn { padding: 15px 25px; border: none; border-radius: 8px; cursor: pointer; font-size: 16px; font-weight: bold; }
            .btn-blue { background: #0066cc; color: white; }
            .btn-green { background: #00aa44; color: white; }
            .btn-yellow { background: #cc8800; color: white; }
            .btn-red { background: #cc3333; color: white; }
            .btn-purple { background: #8844cc; color: white; }
            .btn:hover { opacity: 0.8; }
            .auto-panel { background: linear-gradient(135deg, #1a1a4e, #2a2a6e); border: 2px solid #6644aa; }
            .auto-active { border-color: #00ff88; box-shadow: 0 0 20px rgba(0,255,136,0.3); }
            .decision-item { padding: 8px; background: #0a1628; margin: 5px 0; border-radius: 5px; font-size: 13px; border-left: 3px solid #8844cc; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🚦 Sistema de Tráfico Inteligente - Gen2</h1>
    """
    
    if ultimo_estado:
        estado = ultimo_estado.get('estado', 'DESCONOCIDO')
        html += f"""
            <div class="card">
                <h2>Estado Actual</h2>
                <div class="status">
                    <div class="stat">
                        <div class="stat-label">MODO</div>
                        <div class="stat-value estado-{estado}">{estado}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">FASE</div>
                        <div class="stat-value">{ultimo_estado.get('fase', '-')}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">VEHÍCULOS DIR 1</div>
                        <div class="stat-value">{ultimo_estado.get('vehiculos_dir1', 0)}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">VEHÍCULOS DIR 2</div>
                        <div class="stat-value">{ultimo_estado.get('vehiculos_dir2', 0)}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">LUZ (LDR1/LDR2)</div>
                        <div class="stat-value">{ultimo_estado.get('ldr1', 0)} / {ultimo_estado.get('ldr2', 0)}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">CO2</div>
                        <div class="stat-value">{ultimo_estado.get('co2', 0)}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">WiFi RSSI</div>
                        <div class="stat-value">{ultimo_estado.get('wifi_rssi', 0)} dBm</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">ÚLTIMA ACTUALIZACIÓN</div>
                        <div class="stat-value" style="font-size: 14px;">{ultimo_estado.get('timestamp', '-')}</div>
                    </div>
                </div>
            </div>
        """
    else:
        html += """
            <div class="card">
                <h2>⏳ Esperando conexión del ESP32...</h2>
                <p>Asegúrate de que el ESP32 esté conectado a la misma red WiFi</p>
            </div>
        """
    
    # Panel de Control
    html += """
        <div class="card">
            <h2>🎮 Panel de Control</h2>
            <div class="controls">
                <form action="/api/command" method="POST" style="display: inline;">
                    <input type="hidden" name="cmd" value="PEATONAL">
                    <button type="submit" class="btn btn-blue">🚶 Activar Peatonal</button>
                </form>
                <form action="/api/command" method="POST" style="display: inline;">
                    <input type="hidden" name="cmd" value="NORMAL">
                    <button type="submit" class="btn btn-green">🚗 Modo Normal</button>
                </form>
                <form action="/api/command" method="POST" style="display: inline;">
                    <input type="hidden" name="cmd" value="NOCTURNO">
                    <button type="submit" class="btn btn-yellow">🌙 Modo Nocturno</button>
                </form>
            </div>
    """
    if comando_pendiente:
        html += f'<p style="color: #ffaa00;">⏳ Comando pendiente: {comando_pendiente}</p>'
    html += "</div>"
    
    # Panel del Sistema Adaptativo Inteligente
    auto_class = "auto-panel auto-active" if modo_automatico else "auto-panel"
    auto_estado = "✅ ACTIVO" if modo_automatico else "❌ INACTIVO"
    auto_color = "#00ff88" if modo_automatico else "#ff4444"
    btn_texto = "🚫 Desactivar" if modo_automatico else "🧠 Activar"
    btn_class = "btn btn-red" if modo_automatico else "btn btn-purple"
    
    # Información de setpoints y patrones
    activaciones_recientes = len([
        h for h in historial_peatonal 
        if (datetime.now() - h['timestamp']).total_seconds() < CONFIG_ADAPTATIVO['ventana_peatonal_minutos'] * 60
    ]) if historial_peatonal else 0
    
    promedio_ratio = 0.5
    if historial_desbalance:
        promedio_ratio = sum(h['ratio_d1'] for h in historial_desbalance) / len(historial_desbalance)
    
    html += f"""
        <div class="card {auto_class}">
            <h2>🧠 Sistema Adaptativo Inteligente</h2>
            <div style="display: flex; align-items: center; gap: 20px; margin-bottom: 15px;">
                <div>
                    <span style="font-size: 24px; font-weight: bold; color: {auto_color};">{auto_estado}</span>
                </div>
                <form action="/api/auto" method="POST">
                    <button type="submit" class="{btn_class}">{btn_texto}</button>
                </form>
            </div>
            
            <!-- Setpoints Actuales -->
            <div style="background: #0a1628; padding: 15px; border-radius: 8px; margin-bottom: 15px;">
                <strong>⚙️ Setpoints Actuales del ESP32:</strong>
                <div style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin-top: 10px;">
                    <div style="background: #0f3460; padding: 10px; border-radius: 5px;">
                        <span style="color: #888;">Verde Normal:</span>
                        <span style="color: #00d4ff; font-weight: bold;"> {setpoints_actuales['sp_verde_normal']/1000:.1f}s</span>
                    </div>
                    <div style="background: #0f3460; padding: 10px; border-radius: 5px;">
                        <span style="color: #888;">Tiempo Peatonal:</span>
                        <span style="color: #00d4ff; font-weight: bold;"> {setpoints_actuales['sp_peatonal']/1000:.1f}s</span>
                    </div>
                    <div style="background: #0f3460; padding: 10px; border-radius: 5px;">
                        <span style="color: #888;">Verde Máx (Pesado):</span>
                        <span style="color: #00d4ff; font-weight: bold;"> {setpoints_actuales['sp_verde_pesado_max']/1000:.1f}s</span>
                    </div>
                    <div style="background: #0f3460; padding: 10px; border-radius: 5px;">
                        <span style="color: #888;">Verde Mín (Pesado):</span>
                        <span style="color: #00d4ff; font-weight: bold;"> {setpoints_actuales['sp_verde_pesado_min']/1000:.1f}s</span>
                    </div>
                </div>
            </div>
            
            <!-- Reglas Adaptativas -->
            <div style="background: #0a1628; padding: 15px; border-radius: 8px; margin-bottom: 15px;">
                <strong>📖 Reglas de Adaptación (±{CONFIG_ADAPTATIVO['porcentaje_ajuste']*100:.0f}%):</strong>
                <ul style="margin: 10px 0; padding-left: 20px; font-size: 14px;">
                    <li>📈 <strong>Peatonal frecuente</strong> (≥{CONFIG_ADAPTATIVO['umbral_peatonal_alto']} en {CONFIG_ADAPTATIVO['ventana_peatonal_minutos']}min) → <span style="color:#00ff88;">+{CONFIG_ADAPTATIVO['porcentaje_ajuste']*100:.0f}%</span></li>
                    <li>📉 <strong>Peatonal bajo</strong> (≤{CONFIG_ADAPTATIVO['umbral_peatonal_bajo']} en {CONFIG_ADAPTATIVO['ventana_peatonal_minutos']}min) → <span style="color:#ff6666;">-{CONFIG_ADAPTATIVO['porcentaje_ajuste']*100:.0f}%</span></li>
                    <li>📈 <strong>Desbalance</strong> (&gt;{CONFIG_ADAPTATIVO['umbral_desbalance_alto']*100:.0f}% una dir) → <span style="color:#00ff88;">+{CONFIG_ADAPTATIVO['porcentaje_ajuste']*100:.0f}%</span> verde</li>
                    <li>📉 <strong>Equilibrado</strong> ({CONFIG_ADAPTATIVO['umbral_desbalance_bajo']*100:.0f}%-{(1-CONFIG_ADAPTATIVO['umbral_desbalance_bajo'])*100:.0f}%) → <span style="color:#ff6666;">-{CONFIG_ADAPTATIVO['porcentaje_ajuste']*100:.0f}%</span> verde</li>
                </ul>
                <div style="font-size: 12px; color: #888; margin-top: 5px;">
                    Activaciones recientes: <strong>{activaciones_recientes}</strong> | 
                    Balance D1/D2: <strong>{promedio_ratio*100:.0f}%/{(1-promedio_ratio)*100:.0f}%</strong>
                </div>
            </div>
    """
    
    # Mostrar última decisión y historial
    if modo_automatico and razon_comando:
        html += f"""
            <div style="background: #0f3460; padding: 10px; border-radius: 5px; margin-bottom: 10px; border-left: 4px solid #00ff88;">
                <strong>🎯 Última adaptación:</strong><br>
                <span style="font-size: 13px;">{razon_comando}</span>
            </div>
        """
    
    if historial_decisiones:
        html += "<div><strong>📜 Historial de Adaptaciones:</strong></div>"
        for decision in reversed(historial_decisiones[-5:]):
            html += f'<div class="decision-item">[{decision["timestamp"]}] <strong>{decision["comando"]}</strong>: {decision["razon"]}</div>'
    
    html += "</div>"
    
    # Panel de Estadísticas
    stats = calcular_estadisticas()
    if stats:
        html += f"""
            <div class="card">
                <h2>📊 Análisis en Tiempo Real</h2>
                <div class="status">
                    <div class="stat">
                        <div class="stat-label">TOTAL VEHÍCULOS</div>
                        <div class="stat-value">{stats['total_vehiculos']}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">REGISTROS ANALIZADOS</div>
                        <div class="stat-value">{stats['total_registros']}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">DIRECCIÓN 1</div>
                        <div class="stat-value">{stats['total_dir1']} <span style="font-size:14px;">({stats['pct_dir1']}%)</span></div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">DIRECCIÓN 2</div>
                        <div class="stat-value">{stats['total_dir2']} <span style="font-size:14px;">({stats['pct_dir2']}%)</span></div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">PROMEDIO CO2</div>
                        <div class="stat-value">{stats['avg_co2']}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">PROMEDIO LUZ</div>
                        <div class="stat-value">{stats['avg_luz']}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">BALANCE TRÁFICO</div>
                        <div class="stat-value" style="font-size: 16px;">{stats['direccion_dominante']}</div>
                    </div>
                    <div class="stat">
                        <div class="stat-label">MODO MÁS FRECUENTE</div>
                        <div class="stat-value" style="font-size: 16px;">{stats['estado_frecuente']}</div>
                    </div>
                </div>
                <div style="margin-top: 15px; padding: 15px; background: #0a1628; border-radius: 8px; border-left: 4px solid #00d4ff;">
                    <strong>💡 Recomendación del Sistema:</strong><br>
                    <span style="font-size: 18px;">{stats['recomendacion']}</span>
                </div>
                
                <!-- Gráficas -->
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px;">
                    <div style="background: #0f3460; padding: 15px; border-radius: 8px;">
                        <h3 style="margin: 0 0 10px 0; color: #00d4ff;">📊 Comparación D1 vs D2</h3>
                        <canvas id="chartBarras" height="200"></canvas>
                    </div>
                    <div style="background: #0f3460; padding: 15px; border-radius: 8px;">
                        <h3 style="margin: 0 0 10px 0; color: #00d4ff;">📈 Flujo Temporal</h3>
                        <canvas id="chartLineas" height="200"></canvas>
                    </div>
                </div>
            </div>
        """
    
    # Mostrar historial
    if historial:
        html += """
            <div class="card">
                <h2>📋 Historial Reciente</h2>
                <div class="historial">
        """
        for item in reversed(historial[-20:]):
            html += f'<div class="historial-item">{item}</div>'
        html += "</div></div>"
    
    # Agregar JavaScript para las gráficas
    stats = calcular_estadisticas()
    if stats:
        # Preparar datos para gráfica temporal
        datos_temporales = list(datos_analisis)[-20:]  # Últimos 20 registros
        labels_tiempo = [d.get('timestamp', '')[-8:-3] for d in datos_temporales]  # HH:MM
        valores_d1 = [d.get('vehiculos_dir1', 0) for d in datos_temporales]
        valores_d2 = [d.get('vehiculos_dir2', 0) for d in datos_temporales]
        
        html += f"""
        <script>
            // Gráfica de Barras D1 vs D2
            const ctxBar = document.getElementById('chartBarras');
            if (ctxBar) {{
                new Chart(ctxBar, {{
                    type: 'bar',
                    data: {{
                        labels: ['Dirección 1', 'Dirección 2'],
                        datasets: [{{
                            label: 'Total Vehículos',
                            data: [{stats['total_dir1']}, {stats['total_dir2']}],
                            backgroundColor: ['rgba(0, 212, 255, 0.7)', 'rgba(255, 170, 0, 0.7)'],
                            borderColor: ['#00d4ff', '#ffaa00'],
                            borderWidth: 2
                        }}]
                    }},
                    options: {{
                        responsive: true,
                        plugins: {{
                            legend: {{ display: false }},
                            title: {{ display: false }}
                        }},
                        scales: {{
                            y: {{
                                beginAtZero: true,
                                grid: {{ color: 'rgba(255,255,255,0.1)' }},
                                ticks: {{ color: '#888' }}
                            }},
                            x: {{
                                grid: {{ display: false }},
                                ticks: {{ color: '#888' }}
                            }}
                        }}
                    }}
                }});
            }}
            
            // Gráfica de Líneas Temporal
            const ctxLine = document.getElementById('chartLineas');
            if (ctxLine) {{
                new Chart(ctxLine, {{
                    type: 'line',
                    data: {{
                        labels: {labels_tiempo},
                        datasets: [
                            {{
                                label: 'D1',
                                data: {valores_d1},
                                borderColor: '#00d4ff',
                                backgroundColor: 'rgba(0, 212, 255, 0.1)',
                                fill: true,
                                tension: 0.3
                            }},
                            {{
                                label: 'D2',
                                data: {valores_d2},
                                borderColor: '#ffaa00',
                                backgroundColor: 'rgba(255, 170, 0, 0.1)',
                                fill: true,
                                tension: 0.3
                            }}
                        ]
                    }},
                    options: {{
                        responsive: true,
                        plugins: {{
                            legend: {{
                                labels: {{ color: '#888' }}
                            }}
                        }},
                        scales: {{
                            y: {{
                                beginAtZero: true,
                                grid: {{ color: 'rgba(255,255,255,0.1)' }},
                                ticks: {{ color: '#888' }}
                            }},
                            x: {{
                                grid: {{ display: false }},
                                ticks: {{ color: '#888' }}
                            }}
                        }}
                    }}
                }});
            }}
        </script>
        """
    
    html += """
        </div>
    </body>
    </html>
    """
    return html


@app.route('/api/traffic', methods=['POST'])
def recibir_datos():
    """Endpoint que recibe datos del ESP32"""
    global ultimo_estado, comando_pendiente
    
    try:
        data = request.get_json()
        
        if data:
            # Agregar timestamp
            data['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            ultimo_estado = data
            
            # Guardar en CSV (persistente)
            guardar_en_csv(data)
            
            # Agregar a datos de análisis (tiempo real)
            agregar_dato_analisis(data)
            
            # === MODO AUTOMÁTICO: Analizar y decidir ===
            comando_auto, razon = analizar_y_decidir(data)
            if comando_auto:
                print(f"🤖 Decisión automática: {comando_auto} - {razon}")
                historial.append(f"[{data['timestamp']}] 🤖 AUTO: {comando_auto} ({razon})")
            
            # Guardar en historial (memoria)
            log_entry = f"[{data['timestamp']}] Estado: {data.get('estado')} | D1: {data.get('vehiculos_dir1')} | D2: {data.get('vehiculos_dir2')} | CO2: {data.get('co2')}"
            historial.append(log_entry)
            
            # Limitar historial a 100 entradas
            if len(historial) > 100:
                historial.pop(0)
            
            print(f"📨 Recibido: {json.dumps(data, indent=2)}")
            
            # Respuesta al ESP32 con comando si hay uno pendiente
            response = {
                "status": "ok",
                "timestamp": data['timestamp']
            }
            
            # Si hay comando pendiente, enviarlo
            if comando_pendiente:
                response["command"] = comando_pendiente
                print(f"📤 Enviando comando manual: {comando_pendiente}")
                historial.append(f"[{data['timestamp']}] 📤 COMANDO ENVIADO: {comando_pendiente}")
                comando_pendiente = None  # Limpiar después de enviar
            # Si hay comando automático, enviarlo
            elif comando_auto:
                response["command"] = comando_auto
                print(f"🤖 Enviando comando automático: {comando_auto}")
            
            return jsonify(response), 200
        else:
            return jsonify({"status": "error", "message": "No data received"}), 400
            
    except Exception as e:
        print(f"❌ Error: {str(e)}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/status', methods=['GET'])
def obtener_estado():
    """Endpoint para obtener el estado actual (para otras aplicaciones)"""
    return jsonify(ultimo_estado), 200


@app.route('/api/history', methods=['GET'])
def obtener_historial():
    """Endpoint para obtener el historial"""
    return jsonify({"historial": historial[-50:]}), 200


@app.route('/api/command', methods=['POST'])
def enviar_comando():
    """Endpoint para enviar comandos al ESP32 desde el dashboard"""
    global comando_pendiente
    
    cmd = request.form.get('cmd')
    if cmd:
        comando_pendiente = cmd
        print(f"🎮 Comando programado: {cmd}")
        historial.append(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🎮 COMANDO PROGRAMADO: {cmd}")
    
    # Redirigir de vuelta al dashboard
    from flask import redirect
    return redirect('/')


@app.route('/api/auto', methods=['POST'])
def toggle_automatico():
    """Endpoint para activar/desactivar el modo automático"""
    global modo_automatico, ultimo_comando_auto
    
    modo_automatico = not modo_automatico
    ultimo_comando_auto = None  # Resetear último comando
    
    estado = "ACTIVADO" if modo_automatico else "DESACTIVADO"
    print(f"🤖 Modo automático: {estado}")
    historial.append(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 🤖 MODO AUTOMÁTICO: {estado}")
    
    from flask import redirect
    return redirect('/')


if __name__ == '__main__':
    print("=" * 60)
    print("🚦 Servidor de Tráfico Inteligente - Generación 2")
    print("=" * 60)
    
    # Inicializar archivo CSV
    inicializar_csv()
    
    print("📡 Iniciando servidor en http://0.0.0.0:5000")
    print("📱 Interfaz web: http://localhost:5000")
    print("📨 API endpoint: http://localhost:5000/api/traffic")
    print(f"💾 Datos guardados en: {CSV_FILE}")
    print("=" * 60)
    print("⚠️  Asegúrate de configurar la IP de este servidor en el ESP32")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
