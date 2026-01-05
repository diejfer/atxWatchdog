# Changelog - ATX Watchdog

## Version 1.0 - 2026-01-04

### Implementado

#### Core
- ✅ Configuracion WiFi mediante WiFiManager (portal captivo)
- ✅ Sistema de archivos LittleFS para persistencia de configuracion
- ✅ WebServer HTTP en puerto 80 para configuracion
- ✅ Cliente MQTT con autenticacion opcional

#### Control de Hardware
- ✅ Control de GPIO D1 (GPIO5) para boton POWER (salida a motherboard)
- ✅ Control de GPIO D2 (GPIO4) para boton RESET (salida a motherboard)
- ✅ Botones fisicos en D5 (GPIO14) y D6 (GPIO12) para control local
- ✅ Debounce automatico de 300ms para botones fisicos
- ✅ Pull-up interno en pines de entrada (no requiere resistencias)
- ✅ Duracion de click configurable para Power y Reset
- ✅ Funcion de click con pulso LOW ajustable

#### Watchdog TCP
- ✅ Monitoreo de host:puerto via conexion TCP
- ✅ Intervalo de chequeo configurable
- ✅ Timeout configurable antes de ejecutar accion
- ✅ Contador de fallos consecutivos
- ✅ Reinicio automatico del servidor cuando no responde

#### Control MQTT
- ✅ Comando `power` via MQTT
- ✅ Comando `reset` via MQTT
- ✅ Duracion de click opcional en comandos
- ✅ Topics dinamicos: `/watchdog/{clientID}/cmd` y `/watchdog/{clientID}/status`
- ✅ Keepalive cada 60 segundos con informacion del sistema

#### Interfaz Web
- ✅ Pagina de configuracion responsive
- ✅ Configuracion de hostname y client_id
- ✅ Configuracion de broker MQTT (server, puerto, user, pass)
- ✅ Configuracion de tiempos de click para botones
- ✅ Configuracion de watchdog TCP (host, puerto, intervalos)
- ✅ Control manual de Power y Reset desde web
- ✅ Visualizacion de estado en tiempo real
- ✅ Guardado de configuracion con reinicio automatico

#### Seguridad y Estabilidad
- ✅ Validacion de configuracion
- ✅ Valores por defecto seguros
- ✅ Reintentos de conexion MQTT
- ✅ Timeout en conexiones TCP de prueba
- ✅ Reset de contadores despues de accion watchdog

#### Logging y Debug
- ✅ Mensajes informativos por Serial (115200 baud)
- ✅ Indicacion de estado de conexiones
- ✅ Logs de acciones del watchdog
- ✅ Debug de comandos MQTT recibidos

### Archivos del Proyecto

- `platformio.ini` - Configuracion PlatformIO con librerias
- `src/main.cpp` - Codigo principal (490 lineas)
- `README.md` - Documentacion completa
- `EJEMPLOS.md` - Ejemplos de uso y casos practicos
- `mqtt_control.py` - Script Python para control via MQTT
- `.gitignore` - Archivos a ignorar en git

### Dependencias

- WiFiManager v2.0.17
- PubSubClient v2.8
- ArduinoJson v7.2.1
- LittleFS (incluido en ESP8266 core)

### Valores por Defecto

```
hostname: "atx-watchdog"
client_id: "watchdog-001"
mqtt_port: 1883
power_click_ms: 200
reset_click_ms: 200
watchdog_port: 22
watchdog_check_interval_ms: 30000 (30 segundos)
watchdog_timeout_ms: 120000 (2 minutos)
watchdog_enabled: false
```

### Proximas Mejoras Sugeridas

- [ ] OTA (Over The Air) updates
- [ ] Autenticacion en webserver
- [ ] HTTPS para webserver
- [ ] Multiples hosts a monitorear
- [ ] Telegram/Discord notifications
- [ ] Dashboard con graficos historicos
- [ ] Soporte para MQTT over TLS
- [ ] Configuracion de acciones personalizadas (power, reset, ambos)
- [ ] API REST ademas de MQTT
- [ ] Integracion con Prometheus/Grafana

### Notas de Desarrollo

**Por que TCP en lugar de ICMP (ping)?**
El ESP8266 tiene soporte limitado para ICMP raw sockets. Ademas, verificar un puerto TCP especifico es mas preciso - confirma que el servicio esta realmente funcionando, no solo que el host esta encendido.

**Por que no watchdog serial?**
El watchdog TCP es mucho mas simple y no requiere software adicional en el host. Funciona inmediatamente con servicios ya existentes (SSH, RDP, HTTP, etc).

**Alimentacion del ESP8266**
Se recomienda alimentar desde +5VSB de la fuente ATX para que el watchdog funcione incluso cuando el servidor esta apagado.
