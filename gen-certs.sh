#!/bin/bash
# Genera certificado self-signed para HiveMQ TLS
# Ejecutar en el VPS: bash gen-certs.sh <IP_O_DOMINIO_DEL_VPS>
#
# Ejemplo: bash gen-certs.sh 123.45.67.89
#          bash gen-certs.sh mqtt.tudominio.com

set -e

HOST=${1:-"mqtt.jamonero.local"}
CERT_DIR="./certs"
PASSWORD="jamonero2026"

echo "Generando certificados para: $HOST"

mkdir -p "$CERT_DIR"

# 1. Generar clave privada + certificado self-signed
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

# 2. Convertir a PKCS12 (formato que acepta HiveMQ)
openssl pkcs12 -export \
    -in "$CERT_DIR/server.crt" \
    -inkey "$CERT_DIR/server.key" \
    -out "$CERT_DIR/hivemq.p12" \
    -name hivemq \
    -passout "pass:$PASSWORD"

# 3. Extraer el certificado en formato PEM para el ESP32
cp "$CERT_DIR/server.crt" "$CERT_DIR/ca_cert.pem"

echo ""
echo "Certificados generados en $CERT_DIR:"
echo "  hivemq.p12  → HiveMQ TLS (montar en container)"
echo "  ca_cert.pem → copiar al sketch del ESP32"
echo ""
echo "Contenido de ca_cert.pem (para pegar en main.cpp):"
echo "---"
cat "$CERT_DIR/ca_cert.pem"
