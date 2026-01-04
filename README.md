# ATX Watchdog

Sistema de watchdog y control remoto para equipos ATX usando ESP8266 NodeMCU.

## Caracteristicas

- **WiFi**: Configuracion inicial mediante WiFiManager (portal captivo)
- **WebServer**: Interfaz web para configuracion y control manual
- **MQTT**: Control remoto y keepalive via MQTT
- **Watchdog Serial**: Monitoreo via USB del equipo host
- **GPIOs**: Control de botones Power y Reset

## Hardware

- **NodeMCU ESP8266**
- **Pines utilizados:**
  - D1 (GPIO5) -> Boton POWER
  - D2 (GPIO4) -> Boton RESET
- **Alimentacion:** Vin (5V desde fuente ATX)
- **USB (Opcional):** Para watchdog serial conectado al host

## Configuracion Inicial

### 1. Primera vez - WiFi Setup

Al encender por primera vez, el ESP8266 creara un punto de acceso:
- **SSID:** `ATX-Watchdog-Setup`
- Conectarse desde el celular/PC
- Se abrira portal captivo automaticamente
- Configurar red WiFi de tu hogar

### 2. Configuracion Web

Una vez conectado a tu WiFi:
- El ESP8266 mostrara su IP en el monitor serial
- Abrir navegador en: `http://[IP_DEL_ESP8266]`
- Configurar:
  - **Hostname:** Nombre del dispositivo
  - **MQTT Server:** IP o hostname del broker MQTT
  - **MQTT Port:** Puerto (default: 1883)
  - **MQTT User/Pass:** Credenciales (opcional)
  - **Client ID:** Identificador unico del watchdog
  - **Power Click (ms):** Duracion del pulso para Power
  - **Reset Click (ms):** Duracion del pulso para Reset
  - **Watchdog Timeout (ms):** Tiempo sin keepalive antes de actuar

## Uso

### Control via Web

Desde la interfaz web puedes:
- Ver estado de conexion MQTT
- Ver topics MQTT activos
- Ejecutar manualmente POWER CLICK
- Ejecutar manualmente RESET CLICK
- Modificar configuracion

### Control via MQTT

#### Topic de comandos
`/watchdog/{clientID}/cmd`

**Formato del mensaje (JSON):**
```json
{
  "cmd": "power",
  "duration": 200
}
```

**Comandos disponibles:**
- `power` - Simula click en boton Power
- `reset` - Simula click en boton Reset

El campo `duration` es opcional. Si no se especifica, usa los valores configurados.

**Ejemplo con mosquitto:**
```bash
# Power click (usa duracion configurada)
mosquitto_pub -h [BROKER_IP] -t "/watchdog/watchdog-001/cmd" -m '{"cmd":"power"}'

# Power click con duracion personalizada (5 segundos para apagado forzado)
mosquitto_pub -h [BROKER_IP] -t "/watchdog/watchdog-001/cmd" -m '{"cmd":"power","duration":5000}'

# Reset click
mosquitto_pub -h [BROKER_IP] -t "/watchdog/watchdog-001/cmd" -m '{"cmd":"reset"}'
```

#### Topic de estado
`/watchdog/{clientID}/status`

El ESP8266 publica cada 60 segundos:
```json
{
  "status": "online",
  "uptime": 12345,
  "ip": "192.168.1.100",
  "rssi": -45
}
```

### Watchdog Serial USB

Si conectas el NodeMCU via USB al equipo a monitorear:

**Desde el host (Linux/Windows):**

Enviar por serial (115200 baud):
```
KEEPALIVE\n
```
o
```
PING\n
```

El ESP8266 respondera:
```
ACK
```

**Comportamiento:**
- El ESP8266 espera recibir KEEPALIVE periodicamente
- Si pasa mas tiempo que `watchdog_timeout_ms` sin recibir keepalive
- Ejecutara automaticamente un POWER CLICK para reiniciar el equipo

**Script ejemplo (Linux/Windows Python):**
```python
import serial
import time

ser = serial.Serial('COM4', 115200)  # Ajustar puerto

while True:
    ser.write(b'KEEPALIVE\n')
    response = ser.readline().decode().strip()
    print(f"Respuesta: {response}")
    time.sleep(30)  # Enviar cada 30 segundos
```

## Conexion Hardware

### Conexion a placa madre ATX

1. Identificar pines del panel frontal en la motherboard:
   - **PWR_SW** (Power Switch)
   - **RESET_SW** (Reset Switch)
   - **GND** (Ground)

2. Conectar:
   - D1 (GPIO5) del NodeMCU -> Un pin de PWR_SW
   - GND del NodeMCU -> GND de la motherboard
   - D2 (GPIO4) del NodeMCU -> Un pin de RESET_SW

**IMPORTANTE:** Los pines funcionan haciendo un "corto" momentaneo a GND, por eso el codigo pone el pin en LOW durante el tiempo configurado.

### Alimentacion

- **Opcion 1:** Alimentar Vin del NodeMCU desde +5VSB de la fuente ATX (siempre activo)
- **Opcion 2:** Alimentar desde USB conectado al host (requiere que el USB este alimentado incluso con PC apagado)

## Monitor Serial

Conectar a 115200 baudios para ver mensajes de debug:
```
=================================
ATX Watchdog - Iniciando...
=================================
GPIOs configurados (D1=Power, D2=Reset)
LittleFS montado correctamente
Config no encontrado, usando valores por defecto
Conectando a WiFi...
WiFi conectado!
IP: 192.168.1.100
Servidor web iniciado en puerto 80
=================================
Sistema listo!
=================================
```

## Compilar y Subir

Desde VSCode con PlatformIO:
1. Abrir carpeta del proyecto
2. Build (icono ✓)
3. Upload (icono →)

O desde linea de comandos:
```bash
pio run --target upload
pio device monitor
```

## Librerias Utilizadas

- WiFiManager (tzapu) v2.0.17
- PubSubClient (knolleary) v2.8
- ArduinoJson (bblanchon) v7.2.1
- LittleFS (filesystem para ESP8266)

## Notas

- La configuracion se guarda en LittleFS y persiste entre reinicios
- Al cambiar configuracion desde la web, el ESP8266 se reinicia automaticamente
- El watchdog serial solo se activa despues de recibir el primer KEEPALIVE
- Los clicks simulan presionar el boton (pin a LOW) por la duracion configurada
