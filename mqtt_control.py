#!/usr/bin/env python3
"""
ATX Watchdog MQTT Control
Script para enviar comandos al watchdog via MQTT
Requiere: pip install paho-mqtt
"""

import paho.mqtt.client as mqtt
import json
import argparse
import time

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Conectado al broker MQTT")
        # Suscribirse al topic de status para ver respuestas
        topic = f"/watchdog/{userdata['client_id']}/status"
        client.subscribe(topic)
        print(f"Suscrito a: {topic}")
    else:
        print(f"Error conectando: {rc}")

def on_message(client, userdata, msg):
    print(f"\nMensaje recibido de {msg.topic}:")
    try:
        payload = json.loads(msg.payload.decode())
        print(json.dumps(payload, indent=2))
    except:
        print(msg.payload.decode())

def send_command(client, client_id, cmd, duration=None):
    topic = f"/watchdog/{client_id}/cmd"

    payload = {"cmd": cmd}
    if duration is not None:
        payload["duration"] = duration

    message = json.dumps(payload)
    print(f"\nEnviando a {topic}:")
    print(message)

    result = client.publish(topic, message)

    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print("Comando enviado exitosamente")
    else:
        print(f"Error enviando comando: {result.rc}")

def main():
    parser = argparse.ArgumentParser(description='Control del ATX Watchdog via MQTT')
    parser.add_argument('-H', '--host', default='localhost', help='Broker MQTT (default: localhost)')
    parser.add_argument('-p', '--port', type=int, default=1883, help='Puerto MQTT (default: 1883)')
    parser.add_argument('-u', '--user', help='Usuario MQTT')
    parser.add_argument('-P', '--password', help='Password MQTT')
    parser.add_argument('-c', '--client-id', default='watchdog-001', help='Client ID del watchdog (default: watchdog-001)')
    parser.add_argument('command', choices=['power', 'reset', 'listen'], help='Comando a ejecutar')
    parser.add_argument('-d', '--duration', type=int, help='Duracion del click en milisegundos')

    args = parser.parse_args()

    # Crear cliente MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="watchdog-control")
    client.user_data_set({'client_id': args.client_id})
    client.on_connect = on_connect
    client.on_message = on_message

    if args.user:
        client.username_pw_set(args.user, args.password)

    print(f"Conectando a {args.host}:{args.port}...")
    try:
        client.connect(args.host, args.port, 60)
        client.loop_start()

        time.sleep(1)  # Esperar conexion

        if args.command == 'listen':
            print("\nEscuchando mensajes del watchdog...")
            print("Presiona Ctrl+C para salir\n")
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                print("\nSaliendo...")
        else:
            send_command(client, args.client_id, args.command, args.duration)
            time.sleep(2)  # Esperar respuesta

        client.loop_stop()
        client.disconnect()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    main()
