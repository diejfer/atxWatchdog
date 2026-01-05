# Diagrama de Conexiones - ATX Watchdog

## Esquema Completo

```
┌─────────────────────────────────────────────────────────┐
│                  ESP8266 NodeMCU                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Vin ────────────────────────> +5VSB (Fuente ATX)      │
│  GND ────────────────────────> GND (Fuente/Motherboard)│
│                                                         │
│  SALIDAS (a Motherboard):                              │
│  D1 (GPIO5)  ─────────────────> PWR_SW pin             │
│  D2 (GPIO4)  ─────────────────> RESET_SW pin           │
│                                                         │
│  ENTRADAS (botones fisicos opcionales):                │
│  D5 (GPIO14) <────┬───────── Boton POWER               │
│  D6 (GPIO12) <────┼───────── Boton RESET               │
│                   │                                     │
│                   └───────── GND comun                  │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## Conexion a Motherboard ATX

### Pines del Panel Frontal (Front Panel Connectors)

La mayoria de las motherboards tienen un bloque de pines similar a este:

```
PWR_SW    RESET_SW
┌──┬──┐  ┌──┬──┐
│  │  │  │  │  │     PWR LED    HDD LED
└──┴──┘  └──┴──┘     ┌──┬──┬──┐  ┌──┬──┐
                      │  │  │  │  │  │  │
                      └──┴──┴──┘  └──┴──┘
```

### Conexiones Necesarias

**Para POWER:**
```
NodeMCU D1 (GPIO5) ──────> Cualquier pin de PWR_SW
NodeMCU GND ─────────────> GND de la motherboard
```

**Para RESET:**
```
NodeMCU D2 (GPIO4) ──────> Cualquier pin de RESET_SW
(Usa el mismo GND)
```

**NOTA:** Los conectores PWR_SW y RESET_SW NO tienen polaridad. Puedes conectar a cualquiera de los dos pines.

## Botones Fisicos en el Watchdog

Si quieres agregar botones fisicos al ESP8266 para control local:

```
      Boton POWER              Boton RESET
         ┌───┐                    ┌───┐
         │   │                    │   │
    D5 ──┤   ├── GND         D6 ──┤   ├── GND
         │   │                    │   │
         └───┘                    └───┘
     (Pulsador                (Pulsador
    momentaneo)               momentaneo)
```

**Caracteristicas:**
- No necesitan resistencias (pull-up interno activado)
- Conectar un terminal a D5/D6 y otro a GND
- Al presionar, el pin va a LOW (0V)
- Debounce automatico de 300ms en software

## Alimentacion

### Opcion 1: Desde Fuente ATX (Recomendado)

```
Fuente ATX (Conector 20/24 pines)
┌────────────────────┐
│ Pin 9:  +5VSB (morado) ───────> NodeMCU Vin
│ Pin 3,5,7,15,17:  GND ───────> NodeMCU GND
└────────────────────┘

+5VSB = 5V Standby (siempre activo, incluso con PC apagado)
```

**Ventajas:**
- El watchdog funciona incluso con PC apagado
- Puede encender el servidor remotamente
- No depende del estado del sistema operativo

### Opcion 2: Desde USB del Host

```
Host PC USB ───────> NodeMCU USB
```

**Limitaciones:**
- Solo funciona si el USB sigue alimentado con PC apagado
- Algunos BIOS tienen opcion "USB Power in S5" que debe estar habilitada
- No todos los motherboards soportan esto

## Esquema de Prueba Simple

Para probar el watchdog sin conectarlo a una motherboard real:

```
1. Conectar LEDs en D1 y D2 para visualizar los pulsos:

   D1 ──────┤>├───── GND  (LED + resistencia 220Ω)

   D2 ──────┤>├───── GND  (LED + resistencia 220Ω)

2. Agregar botones de prueba en D5 y D6:

   D5 ──── [Boton] ──── GND

   D6 ──── [Boton] ──── GND

3. Alimentar por USB
```

## Ejemplo de Montaje en Gabinete

Si quieres integrar el watchdog en el gabinete del servidor:

```
┌──────────────────────────────────────────┐
│         Panel Frontal del Gabinete       │
├──────────────────────────────────────────┤
│                                          │
│  [Boton POWER]  [Boton RESET]           │
│       │              │                   │
│       │              │                   │
│    GPIO14         GPIO12                │
│       │              │                   │
│       └──────┬───────┘                   │
│              │                           │
│          [NodeMCU]                       │
│           │   │                          │
│        GPIO5 GPIO4                       │
│           │   │                          │
│           └───┴──> [Motherboard]         │
│                                          │
└──────────────────────────────────────────┘
```

## Cableado Detallado

### Lista de Conexiones

| Desde (NodeMCU) | Hacia | Descripcion |
|----------------|-------|-------------|
| Vin | +5VSB fuente ATX | Alimentacion 5V |
| GND | GND motherboard | Tierra comun |
| D1 (GPIO5) | PWR_SW pin | Salida para Power |
| D2 (GPIO4) | RESET_SW pin | Salida para Reset |
| D5 (GPIO14) | Boton Power + GND | Entrada boton Power (opcional) |
| D6 (GPIO12) | Boton Reset + GND | Entrada boton Reset (opcional) |

### Cable Recomendado

- **Para señales (D1, D2, D5, D6):** Cable de red (UTP) cat5e/cat6
- **Para alimentacion (Vin, GND):** Cable calibre 22-24 AWG
- **Longitud maxima recomendada:** 1-2 metros

### Conectores

- **Dupont hembra-hembra:** Para conectar a pines del NodeMCU
- **Dupont hembra:** Para conectar a pines de motherboard
- **JST o terminal block:** Para botones fisicos

## Seguridad

**IMPORTANTE:**
- NO conectar nunca Vin a +5V o +12V de la fuente (solo +5VSB)
- Verificar polaridad antes de conectar
- Los pines D1 y D2 trabajan en logica inversa (HIGH=reposo, LOW=click)
- NO conectar directamente a 12V o voltajes mayores
- Usar proteccion (fusible 1A) en linea de alimentacion si es posible
