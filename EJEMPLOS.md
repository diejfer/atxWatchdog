# Ejemplos de Uso - ATX Watchdog

## Escenarios de Uso Comunes

### 1. Servidor Linux con SSH

**Configuracion:**
- Host: `192.168.1.100`
- Puerto: `22` (SSH)
- Intervalo de chequeo: `30000` ms (30 segundos)
- Timeout: `120000` ms (2 minutos)

**Como funciona:**
El watchdog verifica cada 30 segundos si el puerto SSH esta abierto. Si durante 2 minutos consecutivos no puede conectarse, reinicia el servidor.

**Comandos MQTT utiles:**
```bash
# Reiniciar servidor remotamente
mosquitto_pub -h broker.local -t "/watchdog/servidor-linux/cmd" -m '{"cmd":"power"}'

# Apagado forzado (5 segundos)
mosquitto_pub -h broker.local -t "/watchdog/servidor-linux/cmd" -m '{"cmd":"power","duration":5000}'
```

### 2. Servidor Windows con RDP

**Configuracion:**
- Host: `192.168.1.50`
- Puerto: `3389` (RDP)
- Intervalo de chequeo: `60000` ms (1 minuto)
- Timeout: `300000` ms (5 minutos)

**Como funciona:**
Verifica cada minuto si el Remote Desktop esta respondiendo. Si falla por 5 minutos, ejecuta power click.

### 3. Servidor Web

**Configuracion:**
- Host: `192.168.1.200`
- Puerto: `80` o `443`
- Intervalo de chequeo: `20000` ms (20 segundos)
- Timeout: `180000` ms (3 minutos)

**Como funciona:**
Monitorea si el servidor web esta escuchando. Util para servidores de produccion que deben estar siempre disponibles.

### 4. Servidor de Base de Datos

**Configuracion PostgreSQL:**
- Host: `192.168.1.150`
- Puerto: `5432`

**Configuracion MySQL:**
- Host: `192.168.1.151`
- Puerto: `3306`

**Configuracion MongoDB:**
- Host: `192.168.1.152`
- Puerto: `27017`

### 5. Servidor de Juegos

**Minecraft:**
- Puerto: `25565`

**Counter-Strike:**
- Puerto: `27015`

## Control via MQTT - Ejemplos Practicos

### Usando mosquitto_pub (Linux/Mac/Windows)

```bash
# Variables
BROKER="192.168.1.10"
CLIENT_ID="watchdog-001"

# Power click normal
mosquitto_pub -h $BROKER -t "/watchdog/$CLIENT_ID/cmd" -m '{"cmd":"power"}'

# Reset click
mosquitto_pub -h $BROKER -t "/watchdog/$CLIENT_ID/cmd" -m '{"cmd":"reset"}'

# Power click largo (apagado forzado 10 segundos)
mosquitto_pub -h $BROKER -t "/watchdog/$CLIENT_ID/cmd" -m '{"cmd":"power","duration":10000}'

# Escuchar status
mosquitto_sub -h $BROKER -t "/watchdog/$CLIENT_ID/status" -v
```

### Usando Node-RED

**Flow de ejemplo para monitoreo:**

```json
[
  {
    "type": "mqtt in",
    "topic": "/watchdog/+/status",
    "broker": "local-broker",
    "name": "Watchdog Status"
  },
  {
    "type": "function",
    "func": "msg.payload = JSON.parse(msg.payload);\nreturn msg;",
    "name": "Parse JSON"
  },
  {
    "type": "debug",
    "name": "Debug"
  }
]
```

**Flow para control:**

```json
[
  {
    "type": "inject",
    "name": "Power ON/OFF",
    "topic": "/watchdog/servidor-principal/cmd",
    "payload": "{\"cmd\":\"power\"}",
    "payloadType": "json"
  },
  {
    "type": "mqtt out",
    "broker": "local-broker"
  }
]
```

### Usando Python

```python
import paho.mqtt.client as mqtt
import json

broker = "192.168.1.10"
client_id = "watchdog-001"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(broker, 1883)

# Enviar comando power
topic = f"/watchdog/{client_id}/cmd"
payload = json.dumps({"cmd": "power"})
client.publish(topic, payload)

client.disconnect()
```

### Usando Home Assistant

**configuration.yaml:**

```yaml
mqtt:
  sensor:
    - name: "Servidor Estado"
      state_topic: "/watchdog/servidor-01/status"
      value_template: "{{ value_json.status }}"
      json_attributes_topic: "/watchdog/servidor-01/status"

  button:
    - name: "Servidor Power"
      command_topic: "/watchdog/servidor-01/cmd"
      payload_press: '{"cmd":"power"}'

    - name: "Servidor Reset"
      command_topic: "/watchdog/servidor-01/cmd"
      payload_press: '{"cmd":"reset"}'
```

## Integracion con Monitoreo

### Script de monitoreo bash

```bash
#!/bin/bash
# monitor_watchdog.sh

BROKER="192.168.1.10"
CLIENT_ID="watchdog-001"

mosquitto_sub -h $BROKER -t "/watchdog/$CLIENT_ID/status" | while read msg
do
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] $msg" >> /var/log/watchdog.log

    # Parsear JSON y verificar
    uptime=$(echo $msg | jq -r '.uptime')
    echo "Servidor activo por: $uptime segundos"
done
```

### Alertas con Telegram

```python
import paho.mqtt.client as mqtt
import json
import requests

TELEGRAM_TOKEN = "tu_bot_token"
CHAT_ID = "tu_chat_id"

def send_telegram(message):
    url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendMessage"
    data = {"chat_id": CHAT_ID, "text": message}
    requests.post(url, data=data)

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    uptime = data.get('uptime', 0)

    # Alerta si el servidor se reinicio
    if uptime < 300:  # Menos de 5 minutos
        send_telegram(f"⚠️ Servidor reiniciado! Uptime: {uptime}s")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("192.168.1.10", 1883)
client.subscribe("/watchdog/+/status")
client.loop_forever()
```

## Tips y Mejores Practicas

1. **Intervalo de chequeo:** No hacer chequeos muy frecuentes (minimo 15-20 segundos) para evitar sobrecarga de red

2. **Timeout razonable:** Configurar timeout al menos 2-3 veces el intervalo de chequeo para evitar reinicios falsos

3. **Puertos a monitorear:**
   - Elegir servicios criticos que siempre deben estar activos
   - SSH/RDP son buenas opciones para servidores de proposito general
   - Para servidores especializados, monitorear el servicio principal

4. **Multiples watchdogs:** Puedes tener varios ESP8266 monitoreando diferentes servidores, cada uno con su client_id unico

5. **Alimentacion redundante:** Alimentar el ESP8266 desde fuente ATX standby (+5VSB) para que funcione incluso con PC apagado
