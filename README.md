# ATX Watchdog

Sistema de watchdog y control remoto para equipos ATX usando ESP8266 NodeMCU.

## Caracteristicas

- **WiFi**: Configuracion inicial mediante WiFiManager (portal captivo)
- **WebServer**: Interfaz web para configuracion y control manual
- **MQTT**: Control remoto y keepalive via MQTT
- **Watchdog TCP**: Monitoreo via conexion TCP a un puerto del servidor
- **GPIOs**: Control de botones Power y Reset
- **Botones Fisicos**: Control local mediante botones en el watchdog

## Hardware

- **NodeMCU ESP8266**
- **Pines de Salida (conectar a motherboard):**
  - D1 (GPIO5) -> Boton POWER de la motherboard
  - D2 (GPIO4) -> Boton RESET de la motherboard
- **Pines de Entrada (botones fisicos opcionales):**
  - D5 (GPIO14) -> Boton POWER fisico del watchdog
  - D6 (GPIO12) -> Boton RESET fisico del watchdog
- **Alimentacion:** Vin (5V desde fuente ATX standby +5VSB)

## Configuracion Inicial

### 1. Primera vez - WiFi Setup

Al encender por primera vez, el ESP8266 creara un punto de acceso:
- **SSID:** `ATX-Watchdog-Setup`
- **Contraseña:** Ninguna (red abierta)

**Pasos:**
1. Conectarse al WiFi "ATX-Watchdog-Setup" desde celular/PC
2. **IMPORTANTE:** Si no se abre automáticamente el portal captivo, abrir navegador y ir a:
   - `http://192.168.4.1`
3. Seleccionar tu red WiFi de la lista
4. Ingresar contraseña
5. Guardar - El ESP8266 se reiniciará y conectará a tu red

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
- **Resetear WiFi** (borra credenciales y reinicia en modo AP)

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

### Watchdog TCP

El ESP8266 puede monitorear automaticamente si un servidor esta respondiendo mediante conexiones TCP.

**Configuracion desde la web:**

1. **Habilitar Watchdog:** Marcar checkbox
2. **Host a monitorear:** IP o hostname del servidor (ej: `192.168.1.100`)
3. **Puerto:** Puerto TCP a verificar (ej: `22` para SSH, `80` para HTTP, `3389` para RDP)
4. **Intervalo de chequeo:** Cada cuanto tiempo verificar (default: 30000ms = 30 segundos)
5. **Timeout sin respuesta:** Tiempo total sin respuesta antes de reiniciar (default: 120000ms = 2 minutos)

**Comportamiento:**
- El ESP8266 intenta conectarse al host:puerto cada X segundos
- Si la conexion TCP es exitosa, el host esta vivo
- Si falla repetidamente por mas tiempo que el timeout configurado
- Ejecuta automaticamente un POWER CLICK para reiniciar el equipo

**Ejemplos de puertos comunes:**
- `22` - SSH (Linux/Unix)
- `3389` - RDP (Windows Remote Desktop)
- `80` o `443` - Servidor web
- `5900` - VNC
- Cualquier puerto TCP que tu servidor tenga abierto

**Ventajas sobre watchdog serial:**
- No requiere software adicional en el host
- Funciona con cualquier servicio TCP
- Monitoreo verdadero del estado del servidor (no solo del OS)
- Funciona tanto con servidores Linux como Windows

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

### Botones Fisicos (Opcional)

Puedes agregar botones fisicos al watchdog para control local:

```
Boton POWER:
  - Un terminal del boton -> D5 (GPIO14)
  - Otro terminal del boton -> GND

Boton RESET:
  - Un terminal del boton -> D6 (GPIO12)
  - Otro terminal del boton -> GND
```

**Caracteristicas:**
- Los botones usan pull-up interno, no necesitan resistencias externas
- Debounce automatico de 300ms
- Al presionar ejecuta el mismo click configurado (duracion configurable desde web)
- Util para pruebas o control local sin necesidad de web/MQTT

**Tipo de botones recomendados:**
- Pulsadores momentaneos (normalmente abiertos)
- Cualquier interruptor de dos terminales

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

## Resetear Configuracion WiFi

Si necesitas reconfigurar el WiFi o ver el portal captivo nuevamente, tienes 3 opciones:

### Opcion 1: Desde la Interfaz Web (Mas Facil)
1. Acceder a la IP del ESP8266 desde el navegador
2. Ir a la seccion "Zona Peligrosa" al final de la pagina
3. Hacer clic en "RESETEAR WiFi"
4. El ESP8266 se reiniciara en modo AP
5. Conectarse a "ATX-Watchdog-Setup" y reconfigurar

### Opcion 2: Borrar Flash Completo (PlatformIO)
```bash
# Borrar toda la flash del ESP8266
pio run --target erase

# Volver a subir el codigo
pio run --target upload
```

### Opcion 3: Usar esptool (Si lo tienes instalado)
```bash
esptool.py --port COM4 erase_flash
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
