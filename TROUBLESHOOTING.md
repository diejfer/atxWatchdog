# Troubleshooting - ATX Watchdog

## Problemas con Portal Captivo WiFi

### ❌ Problema: El portal captivo no se abre automáticamente

**Causa:** Algunos dispositivos (especialmente Windows y ciertos Android) no detectan automáticamente el portal captivo.

**Solución:**

1. **Conectarse al WiFi "ATX-Watchdog-Setup"**
2. **Abrir navegador web** (NO usar apps)
3. **Ir manualmente a:** `http://192.168.4.1`

**URLs alternativas que puedes probar:**
- `http://192.168.4.1`
- `http://example.com` (cualquier HTTP, NO HTTPS)
- `http://google.com`
- `http://neverssl.com`

**Desde el Monitor Serial:**
Cuando el ESP8266 entra en modo AP, verás:
```
========================================
MODO PORTAL CAPTIVO ACTIVADO
========================================
SSID: ATX-Watchdog-Setup
IP: 192.168.4.1

Conectate al WiFi y abre:
  http://192.168.4.1
========================================
```

### ❌ Problema: No puedo conectarme a "ATX-Watchdog-Setup"

**Posibles causas:**

1. **El ESP8266 no está en modo AP**
   - Verifica que la flash esté borrada: `pio run --target erase`
   - Sube el código nuevamente: `pio run --target upload`

2. **Ya tiene credenciales WiFi guardadas**
   - Usa el botón "RESETEAR WiFi" desde la interfaz web
   - O borra la flash: `pio run --target erase`

3. **El ESP8266 se reinició rápido**
   - WiFiManager tiene timeout de 180 segundos (3 minutos)
   - Mira el monitor serial para confirmar que está en modo AP

### ❌ Problema: El portal se cierra solo después de 3 minutos

**Causa:** El timeout del portal captivo está configurado en 180 segundos.

**Solución:**

**Opción 1: Aumentar el timeout** (modificar código)
```cpp
wifiManager.setConfigPortalTimeout(300); // 5 minutos
```

**Opción 2: Deshabilitar el timeout** (para pruebas)
```cpp
wifiManager.setConfigPortalTimeout(0); // Sin timeout
```

**Opción 3: Conectarse más rápido**
- Ten la contraseña WiFi lista antes de iniciar

## Problemas de Conexión WiFi

### ❌ Problema: No se conecta a mi red WiFi

**Diagnóstico por Monitor Serial:**

```
Conectando a WiFi...
Fallo al conectar WiFi. Reiniciando...
```

**Posibles causas:**

1. **Contraseña incorrecta**
   - Borra credenciales y vuelve a intentar
   - Verifica mayúsculas/minúsculas

2. **WiFi de 5GHz**
   - El ESP8266 solo soporta 2.4GHz
   - Configura tu router en modo dual (2.4 + 5GHz) o solo 2.4GHz

3. **Canal WiFi incompatible**
   - ESP8266 soporta canales 1-11 (USA)
   - Canales 12-14 pueden no funcionar
   - Cambia canal del router a 1-11

4. **SSID oculto**
   - El ESP8266 puede tener problemas con SSIDs ocultos
   - Habilita broadcast del SSID temporalmente

### ❌ Problema: Se conecta pero pierde la conexión

**Causas comunes:**

1. **Señal WiFi débil**
   - Acerca el ESP8266 al router
   - Verifica RSSI en la interfaz web (debe ser > -70 dBm)

2. **IP conflictiva**
   - Reserva IP fija en el router para el MAC del ESP8266
   - O configura rango DHCP más amplio

3. **Alimentación insuficiente**
   - El ESP8266 necesita ~300mA en picos
   - Usa fuente de al menos 500mA
   - Verifica conexión a +5VSB de la fuente ATX

## Problemas con Interfaz Web

### ❌ Problema: No puedo acceder a la IP del ESP8266

**Diagnóstico:**

1. **Verifica la IP en el monitor serial:**
```
WiFi conectado!
IP: 192.168.1.XXX
```

2. **Prueba hacer ping:**
```bash
ping 192.168.1.XXX
```

3. **Verifica que estás en la misma red**
   - PC y ESP8266 deben estar en la misma subred

**Soluciones:**

- Usa `http://IP` (no `https://`)
- Prueba desde otro dispositivo
- Verifica firewall del PC
- Intenta con mDNS: `http://atx-watchdog.local` (puede no funcionar en Windows)

### ❌ Problema: La página web se ve mal o no carga

**Síntomas:** CSS no carga, botones no funcionan, etc.

**Causa:** El HTML está embebido en el código, debería funcionar siempre.

**Soluciones:**
- Limpia caché del navegador (Ctrl+F5)
- Prueba con modo incógnito
- Usa Chrome/Firefox (mejor compatibilidad)
- Verifica que el ESP8266 tenga suficiente memoria

## Problemas con MQTT

### ❌ Problema: Estado MQTT muestra "Desconectado"

**Diagnóstico por Monitor Serial:**

```
Conectando a MQTT...fallo, rc=-2
```

**Códigos de error MQTT:**
- `-4` : Timeout de conexión (broker no alcanzable)
- `-3` : Conexión perdida
- `-2` : Fallo de conexión
- `-1` : Desconectado
- `0` : Conectado OK
- `1` : Protocolo incorrecto
- `2` : Client ID rechazado
- `3` : Servidor no disponible
- `4` : Usuario/password incorrectos
- `5` : No autorizado

**Soluciones según el código:**

**rc=-4 (Timeout):**
- Verifica que el broker MQTT esté corriendo
- Prueba ping al broker: `ping [MQTT_SERVER]`
- Verifica puerto (default: 1883)
- Revisa firewall del broker

**rc=4 (Usuario/password incorrectos):**
- Verifica credenciales MQTT
- Si el broker no requiere auth, deja campos vacíos

**rc=2 (Client ID rechazado):**
- Cambia el Client ID en la configuración
- Verifica que no haya otro cliente con el mismo ID

### ❌ Problema: No recibo comandos MQTT

**Verificación:**

1. **Confirma que el topic es correcto:**
   - Topic CMD: `/watchdog/{clientID}/cmd`
   - Ejemplo: `/watchdog/watchdog-001/cmd`

2. **Verifica formato JSON:**
```json
{"cmd":"power"}
```
```json
{"cmd":"reset","duration":500}
```

3. **Monitorea en el serial:**
```
Mensaje MQTT recibido en /watchdog/watchdog-001/cmd: {"cmd":"power"}
Comando POWER recibido (duracion: 200 ms)
```

**Prueba desde terminal:**
```bash
# Suscribirse al status para verificar conectividad
mosquitto_sub -h [BROKER] -t "/watchdog/watchdog-001/status" -v

# Enviar comando
mosquitto_pub -h [BROKER] -t "/watchdog/watchdog-001/cmd" -m '{"cmd":"power"}'
```

## Problemas con Watchdog TCP

### ❌ Problema: Watchdog no detecta cuando el servidor está caído

**Diagnóstico por Monitor Serial:**

```
Probando conexion TCP a 192.168.1.100:22... OK
Watchdog: Host respondiendo correctamente
```

**Pero el servidor está caído?**

**Posibles causas:**

1. **Puerto equivocado**
   - Verifica que el puerto esté abierto: `telnet 192.168.1.100 22`
   - Prueba: `nmap -p 22 192.168.1.100`

2. **Servidor responde TCP pero servicio caído**
   - El TCP connect puede completarse incluso si el servicio falla
   - Prueba con otro puerto más crítico

3. **Firewall permite conexiones pero servicio no responde**
   - Verifica que el servicio esté realmente corriendo

**Soluciones:**
- Usa el puerto del servicio crítico (SSH, RDP, HTTP)
- Reduce el timeout para detección más rápida
- Aumenta intervalo de chequeo para evitar falsos positivos

### ❌ Problema: Watchdog reinicia el servidor cuando no debería (falsos positivos)

**Síntomas:**
```
Watchdog: Fallo #1 conectando al host
Watchdog: Fallo #2 conectando al host
WATCHDOG TIMEOUT!
```

**Causas:**

1. **Timeout muy corto**
   - Default: 120 segundos (2 minutos)
   - Aumenta a 300 segundos (5 minutos) para servidores lentos

2. **Problemas de red temporales**
   - Aumenta `watchdog_timeout_ms`
   - Reduce `watchdog_check_interval_ms` para más muestras

3. **Servidor tarda en arrancar servicio**
   - Después de reinicio, el servidor puede tardar minutos
   - Aumenta timeout considerablemente

**Configuración recomendada para evitar falsos positivos:**
```
Intervalo de chequeo: 30000 ms (30s)
Timeout: 300000 ms (5 minutos)
```
Esto significa: revisar cada 30s, y solo reiniciar si falla por 5 minutos continuos.

## Problemas con Botones Físicos

### ❌ Problema: Los botones físicos no funcionan

**Diagnóstico:**

1. **Verifica en monitor serial al presionar:**
```
Boton POWER fisico presionado!
Ejecutando click en pin 5 por 200 ms
```

2. **Si no aparece nada:**
   - Verifica conexión: D5 -> Botón -> GND
   - Prueba con multímetro: D5 debe estar en ~3.3V (pull-up)
   - Al presionar botón: D5 debe ir a 0V (GND)

3. **Verifica polaridad:**
   - Los botones NO tienen polaridad (da igual cómo conectes)
   - Pero DEBE conectarse entre D5/D6 y GND

**Prueba rápida:**
- Conecta directamente D5 a GND con cable
- Deberías ver mensaje en serial
- Si funciona, el problema es el botón físico

## Problemas de Compilación

### ❌ Error: Library not found

**Error completo:**
```
Library not found: WiFiManager
```

**Solución:**
```bash
# PlatformIO debería instalar automáticamente
# Si falla, fuerza la instalación:
pio lib install
```

### ❌ Error: Out of memory

**Síntoma:** El código no cabe en el ESP8266

**Solución:** El código actual es ~558 líneas y debería caber. Si tienes este error:
1. Verifica que estés compilando para `nodemcuv2` (4MB flash)
2. Limpia y recompila: `pio run --target clean && pio run`

## Obtener Ayuda

Si ninguna solución funciona:

1. **Monitor Serial:** Siempre revisa el monitor serial primero
2. **Logs:** Copia los mensajes de error completos
3. **Issue en GitHub:** Reporta el problema con:
   - Versión del código
   - Logs del serial
   - Configuración usada
   - Pasos para reproducir
