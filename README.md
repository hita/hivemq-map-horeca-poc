# MAP HoReCa Device × HiveMQ CE — MQTT/TLS POC

A working end-to-end demonstration of an ESP32-S3 IoT device publishing real-time telemetry to a self-managed HiveMQ Community Edition broker over MQTT/TLS.

**Built entirely with [Claude Code](https://claude.ai/code)** — from architecture decisions to certificate generation, Docker configuration, ESP32 firmware, and Python subscriber. Total time to a working system: ~90 minutes.

---

## What This Demonstrates

A MAP (Modified Atmosphere) preservation device for HoReCa simulates the exact data pipeline a real deployment would use:

```
ESP32-S3 (edge device)
  │  MQTT/TLS — port 8883
  │  X.509 certificate pinning
  │  QoS 0 telemetry / QoS 1 commands / retained LWT
  ▼
HiveMQ CE (self-managed, Docker, Ubuntu VPS)
  │  Topic schema:  jamonero/{device_id}/telemetry
  │                 jamonero/{device_id}/status  (retained LWT)
  │                 jamonero/{device_id}/command
  ▼
Python fleet subscriber (real-time terminal dashboard)
```

**Telemetry payload:**
```json
{
  "device_id": "device001",
  "o2_pct": 0.3,
  "temp_c": 4.1,
  "humidity_pct": 76.2,
  "map_active": true,
  "cycle_id": "MAP-001",
  "timestamp": 1240
}
```

The simulation runs a MAP purge cycle: O2 starts at ~21% and decreases to <0.3% over ~10 minutes, at which point `map_active` becomes `true` — representing a device that has successfully displaced atmospheric oxygen with CO2/N2.

---

## Requirements

| Component | Version tested |
|---|---|
| Ubuntu VPS | 24.04 LTS |
| Docker + Compose | 29.4.3 + v5.1 |
| ESP32-S3 board | N16R8 variant |
| PlatformIO | CLI or VS Code extension |
| Python | 3.10+ |
| paho-mqtt | 2.x |

---

## Setup

### Step 1 — Clone and generate certificates

```bash
git clone <this-repo>
cd jamonero-hivemq-poc

# Copy to your VPS
scp -r . root@YOUR_VPS_IP:~/jamonero-poc/

# SSH in and generate TLS certificates
ssh root@YOUR_VPS_IP
cd ~/jamonero-poc
bash gen-certs.sh YOUR_VPS_IP
```

The script generates:
- `certs/hivemq.p12` — PKCS12 keystore for HiveMQ
- `certs/ca_cert.pem` — certificate for the ESP32 and Python subscriber

### Step 2 — Start HiveMQ CE

```bash
# On the VPS
docker compose up -d
docker logs hivemq-jamonero --tail 20
```

Expected output includes:
```
Started TCP Listener with TLS on address 0.0.0.0 and on port 8883.
Started HiveMQ in 3232ms
```

Open firewall ports:
```bash
ufw allow 8883/tcp   # MQTT/TLS
ufw allow 1883/tcp   # MQTT plaintext (optional, for initial testing)
```

### Step 3 — Flash the ESP32

Copy `certs/ca_cert.pem` content and paste it into `esp32/src/secrets.h`:

```bash
cp esp32/secrets.h.example esp32/src/secrets.h
# Edit esp32/src/secrets.h with your WiFi credentials and the certificate
```

`esp32/src/secrets.h`:
```cpp
#pragma once
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

The CA certificate is already embedded in `esp32/src/main.cpp` if you're using this repo after running `gen-certs.sh` on your own VPS — replace with your own cert output.

Flash:
```bash
cd esp32
pio run --target upload   # downloads toolchain on first run (~2 min)
pio device monitor        # watch serial output
```

### Step 4 — Subscribe to telemetry

```bash
pip install paho-mqtt

python3 subscriber/subscriber.py YOUR_VPS_IP \
  --cert certs/ca_cert.pem
```

You'll see real-time output like:
```
[10:32:15] device001
  O2:   0.3%  |  Temp: 4.1°C  |  HR: 76.2%  |  MAP ACTIVO
  Topic: jamonero/device001/telemetry
```

---

## Architecture Decisions — Why These Choices

**Why HiveMQ CE (self-managed) instead of HiveMQ Cloud or AWS IoT Core?**
Self-managed deployment mirrors how HiveMQ's enterprise customers (automotive, pharma, manufacturing) actually run the broker — on-premises or at the edge, not as a managed service. This is the deployment model the product is designed for.

**Why PKCS12 instead of JKS?**
HiveMQ 2025.x accepts PKCS12 natively. JKS (Java KeyStore) conversion via `keytool` is no longer required. PKCS12 is the standard; use it.

**Why QoS 0 for telemetry, QoS 1 for commands and status?**
Telemetry is high-frequency and loss-tolerant — a missed O2 reading is fine, the next one arrives in 10 seconds. Commands and device status changes are low-frequency and loss-intolerant — a missed "start MAP cycle" or an incorrect "device offline" state has operational consequences.

**Why X.509 per-device certificates instead of username/password?**
Per-device certificates bind identity to the device cryptographically, not to a shared secret. A compromised device can be revoked individually without rotating credentials across the fleet. This is the correct model for industrial IoT.

---

## Known Limitations of This POC

- **No authentication enforcement.** HiveMQ CE runs with the `allow-all` extension — any client can connect and publish to any topic. A production deployment requires the HiveMQ Enterprise Security Extension or a custom extension.
- **Self-signed certificate.** The cert is not issued by a public CA, so browsers and most clients will reject it without explicitly trusting it. For production, use Let's Encrypt with a domain.
- **No topic ACLs.** Any subscriber can receive any device's telemetry. A multi-tenant deployment needs per-device ACLs.
- **Simulated sensors.** The ESP32 publishes algorithmically generated values, not real sensor readings. Connecting a physical O2 sensor (e.g., DFRobot SEN0322 or LOX-02-S) requires additional firmware work.
- **HiveMQ CE has no management UI.** The Control Center (web UI) is an Enterprise-only feature in HiveMQ 2025.x. Use MQTT Explorer or the subscriber script for broker observability.

---

## Generated with Claude Code

This entire project was built in a single Claude Code session:

- Architecture designed through conversation
- `gen-certs.sh` written and debugged by Claude
- `docker-compose.yml` and `config.xml` generated and iterated
- ESP32 firmware written, board variant identified (ESP32-S3 N16R8), toolchain configured
- Permissions bug diagnosed and fixed (`chmod 644` on certs after identifying the container UID mismatch)
- Python subscriber written and tested
- End-to-end validation run (MQTT plaintext → MQTT/TLS → ESP32 → broker → subscriber)

The product feedback document (`HIVEMQ_CE_PRODUCT_FEEDBACK.md`) was written based on the actual friction points encountered during this session — not hypothetical issues.

**Total wall-clock time from zero to working ESP32 → HiveMQ/TLS → subscriber:** ~90 minutes.

---

## Files

```
jamonero-hivemq-poc/
├── docker-compose.yml              # HiveMQ CE service definition
├── config/
│   └── config.xml                  # HiveMQ config: TCP + TLS listeners
├── certs/
│   ├── ca_cert.pem                 # Server cert (public, share with clients)
│   ├── hivemq.p12                  # PKCS12 keystore (keep private)
│   └── server.key                  # Private key (keep private)
├── gen-certs.sh                    # Self-signed cert generator
├── esp32/
│   ├── platformio.ini              # ESP32-S3 board config
│   ├── secrets.h.example           # WiFi credentials template
│   └── src/
│       ├── main.cpp                # MQTT firmware with TLS and telemetry sim
│       └── secrets.h               # WiFi credentials (gitignored)
├── subscriber/
│   └── subscriber.py               # Python fleet monitor
├── HIVEMQ_CE_PRODUCT_FEEDBACK.md   # Product feedback from this deployment
└── .gitignore                      # Excludes secrets.h and private keys
```
