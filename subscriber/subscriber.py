#!/usr/bin/env python3
"""
MAP HoReCa Device × HiveMQ — Fleet Subscriber
Muestra telemetría en tiempo real de todos los dispositivos conectados.
Uso: python3 subscriber.py <IP_VPS> [--port 8883] [--no-tls]
"""

import argparse
import json
import ssl
import sys
from datetime import datetime
import paho.mqtt.client as mqtt

# ─── Colores para terminal ────────────────────────────────────────────────────
GREEN  = "\033[92m"
YELLOW = "\033[93m"
RED    = "\033[91m"
CYAN   = "\033[96m"
RESET  = "\033[0m"
BOLD   = "\033[1m"

def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print(f"{GREEN}✓ Conectado a HiveMQ{RESET}")
        client.subscribe("jamonero/#", qos=1)
        client.subscribe("fleet/#", qos=1)
        print(f"{CYAN}Suscrito a jamonero/# y fleet/#\nEsperando telemetría...{RESET}\n")
    else:
        print(f"{RED}✗ Conexión fallida: {reason_code}{RESET}")

def on_message(client, userdata, msg):
    ts = datetime.now().strftime("%H:%M:%S")
    topic = msg.topic

    try:
        payload = json.loads(msg.payload.decode())
    except Exception:
        payload = msg.payload.decode()

    if "/telemetry" in topic:
        device = payload.get("device_id", "?")
        o2     = payload.get("o2_pct", "?")
        temp   = payload.get("temp_c", "?")
        hum    = payload.get("humidity_pct", "?")
        active = payload.get("map_active", False)

        o2_color = GREEN if float(o2) < 1.0 else (YELLOW if float(o2) < 5.0 else RED)
        map_str  = f"{GREEN}MAP ACTIVO{RESET}" if active else f"{YELLOW}purgando{RESET}"

        print(f"{BOLD}[{ts}] {device}{RESET}")
        print(f"  O2:   {o2_color}{o2}%{RESET}  |  Temp: {temp}°C  |  HR: {hum}%  |  {map_str}")
        print(f"  Topic: {topic}")
        print()

    elif "/status" in topic:
        online = payload.get("online", False) if isinstance(payload, dict) else False
        status = f"{GREEN}ONLINE{RESET}" if online else f"{RED}OFFLINE{RESET}"
        print(f"[{ts}] {topic}: {status}\n")

    else:
        print(f"[{ts}] {topic}: {payload}\n")

def on_disconnect(client, userdata, disconnect_flags, reason_code, properties=None):
    print(f"{YELLOW}Desconectado (rc={reason_code}){RESET}")

def main():
    parser = argparse.ArgumentParser(description="MAP HoReCa device fleet subscriber")
    parser.add_argument("host", help="IP o dominio del VPS")
    parser.add_argument("--port", type=int, default=8883)
    parser.add_argument("--no-tls", action="store_true", help="Usar puerto 1883 sin TLS")
    parser.add_argument("--cert", default="../certs/ca_cert.pem", help="Ruta al certificado CA")
    args = parser.parse_args()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="jamonero-fleet-monitor")
    client.on_connect    = on_connect
    client.on_message    = on_message
    client.on_disconnect = on_disconnect

    if not args.no_tls:
        port = args.port if args.port != 1883 else 8883
        context = ssl.create_default_context()
        try:
            context.load_verify_locations(cafile=args.cert)
            print(f"TLS: usando certificado {args.cert}")
        except Exception:
            print(f"{YELLOW}Cert no encontrado en {args.cert}, usando verificación del sistema{RESET}")
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        client.tls_set_context(context)
    else:
        port = 1883
        print(f"{YELLOW}Modo sin TLS (puerto 1883){RESET}")

    print(f"Conectando a {args.host}:{port}...")
    client.connect(args.host, port, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nSubscriber detenido.")
        client.disconnect()

if __name__ == "__main__":
    main()
