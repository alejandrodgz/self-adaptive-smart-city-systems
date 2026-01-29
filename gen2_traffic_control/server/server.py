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

app = Flask(__name__)

# Almacena el último estado recibido
ultimo_estado = {}
historial = []

# Comando pendiente para enviar al ESP32
comando_pendiente = None

@app.route('/')
def index():
    """Página principal con estado actual"""
    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Sistema de Tráfico Gen2</title>
        <meta http-equiv="refresh" content="5">
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
            .btn:hover { opacity: 0.8; }
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
            
            # Guardar en historial
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
                print(f"📤 Enviando comando: {comando_pendiente}")
                historial.append(f"[{data['timestamp']}] 📤 COMANDO ENVIADO: {comando_pendiente}")
                comando_pendiente = None  # Limpiar después de enviar
            
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


if __name__ == '__main__':
    print("=" * 60)
    print("🚦 Servidor de Tráfico Inteligente - Generación 2")
    print("=" * 60)
    print("📡 Iniciando servidor en http://0.0.0.0:5000")
    print("📱 Interfaz web: http://localhost:5000")
    print("📨 API endpoint: http://localhost:5000/api/traffic")
    print("=" * 60)
    print("⚠️  Asegúrate de configurar la IP de este servidor en el ESP32")
    print("=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
