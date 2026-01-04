#!/usr/bin/env python3
"""
ATX Watchdog Client
Script para enviar keepalive al ESP8266 Watchdog via serial USB
Compatible con Linux y Windows
"""

import serial
import time
import sys
import argparse

def main():
    parser = argparse.ArgumentParser(description='ATX Watchdog Client - Envia keepalive por serial')
    parser.add_argument('-p', '--port', default='COM4', help='Puerto serial (default: COM4)')
    parser.add_argument('-b', '--baud', type=int, default=115200, help='Baudrate (default: 115200)')
    parser.add_argument('-i', '--interval', type=int, default=30, help='Intervalo entre keepalives en segundos (default: 30)')

    args = parser.parse_args()

    print(f"ATX Watchdog Client")
    print(f"Conectando a {args.port} @ {args.baud} baud...")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=2)
        print(f"Conectado exitosamente!")
        print(f"Enviando KEEPALIVE cada {args.interval} segundos")
        print("Presiona Ctrl+C para salir\n")

        while True:
            # Enviar keepalive
            ser.write(b'KEEPALIVE\n')
            ser.flush()

            # Leer respuesta (timeout 2 segundos)
            try:
                response = ser.readline().decode().strip()
                if response:
                    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                    if response == "ACK":
                        print(f"[{timestamp}] Keepalive OK - Watchdog activo")
                    else:
                        print(f"[{timestamp}] Respuesta: {response}")
                else:
                    print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Sin respuesta del watchdog")
            except Exception as e:
                print(f"Error leyendo respuesta: {e}")

            # Esperar intervalo
            time.sleep(args.interval)

    except serial.SerialException as e:
        print(f"Error abriendo puerto serial: {e}")
        print(f"\nPuertos disponibles:")
        try:
            from serial.tools import list_ports
            for port in list_ports.comports():
                print(f"  - {port.device}: {port.description}")
        except:
            pass
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nDeteniendo watchdog client...")
        ser.close()
        sys.exit(0)

if __name__ == '__main__':
    main()
