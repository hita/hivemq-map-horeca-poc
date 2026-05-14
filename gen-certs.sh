#!/bin/bash
# Generates a self-signed certificate for HiveMQ TLS
# Run on the VPS: bash gen-certs.sh <VPS_IP_OR_DOMAIN>
#
# Example: bash gen-certs.sh 123.45.67.89
#          bash gen-certs.sh mqtt.yourdomain.com

set -e

HOST=${1:-"mqtt.jamonero.local"}
CERT_DIR="./certs"
PASSWORD="jamonero2026"

echo "Generating certificates for: $HOST"

mkdir -p "$CERT_DIR"

# 1. Generate private key + self-signed certificate
openssl req -x509 -newkey rsa:4096 \
    -keyout "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.crt" \
    -days 825 -nodes \
    -subj "/C=ES/ST=Andalucia/L=Sevilla/O=Jamonero/CN=$HOST" \
    -addext "subjectAltName=IP:${HOST},DNS:${HOST}" 2>/dev/null || \
openssl req -x509 -newkey rsa:4096 \
    -keyout "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.crt" \
    -days 825 -nodes \
    -subj "/C=ES/ST=Andalucia/L=Sevilla/O=Jamonero/CN=$HOST"

# 2. Convert to PKCS12 (native format accepted by HiveMQ)
openssl pkcs12 -export \
    -in "$CERT_DIR/server.crt" \
    -inkey "$CERT_DIR/server.key" \
    -out "$CERT_DIR/hivemq.p12" \
    -name hivemq \
    -passout "pass:$PASSWORD"

# 3. Extract certificate in PEM format for the ESP32
cp "$CERT_DIR/server.crt" "$CERT_DIR/ca_cert.pem"

echo ""
echo "Certificates generated in $CERT_DIR:"
echo "  hivemq.p12  → HiveMQ TLS (mount in container)"
echo "  ca_cert.pem → copy to ESP32 sketch"
echo ""
echo "Contents of ca_cert.pem (paste into main.cpp):"
echo "---"
cat "$CERT_DIR/ca_cert.pem"
