#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

// ─── Configuration ───────────────────────────────────────────────────────────
// WIFI_SSID and WIFI_PASSWORD are defined in secrets.h

const char* MQTT_HOST     = "187.124.36.17";
const int   MQTT_PORT     = 8883;
const char* DEVICE_ID     = "device001";
const char* MQTT_USER     = "";   // empty if no authentication configured
const char* MQTT_PASS     = "";

// Server certificate (paste output of gen-certs.sh → ca_cert.pem)
// Paste between R"(   )" without modifying line breaks
const char* CA_CERT = R"(
-----BEGIN CERTIFICATE-----
MIIFvTCCA6WgAwIBAgIUYym9pFDjosOluiJpa5fl/xiSvdswDQYJKoZIhvcNAQEL
BQAwXjELMAkGA1UEBhMCRVMxEjAQBgNVBAgMCUFuZGFsdWNpYTEQMA4GA1UEBwwH
U2V2aWxsYTERMA8GA1UECgwISmFtb25lcm8xFjAUBgNVBAMMDTE4Ny4xMjQuMzYu
MTcwHhcNMjYwNTE0MTAxNTQ4WhcNMjgwODE2MTAxNTQ4WjBeMQswCQYDVQQGEwJF
UzESMBAGA1UECAwJQW5kYWx1Y2lhMRAwDgYDVQQHDAdTZXZpbGxhMREwDwYDVQQK
DAhKYW1vbmVybzEWMBQGA1UEAwwNMTg3LjEyNC4zNi4xNzCCAiIwDQYJKoZIhvcN
AQEBBQADggIPADCCAgoCggIBAL3DF9EgV8HtkhNvEx0B2CxNQ4AswN+nTCsmM9Vu
T+iVkIB7OMoRDLOPhNz2pnZ7UEZc/6u2GC2JfZQ4qATTEW+SKuoWsvkEP82Q+7iA
daabM7k7HMvQpRJBAB+RXom85BO/+krR9zAjkvnzlhF5fyYML8/kc0heyBMqz+ps
DQpakIDPk39dhPng3fAGFS+HvpRhY+M1DL9Y0L1HtlR+wf26tEbrtlogtEnnmkeP
AVjQ6CW2QnUKN36xKjbbUguiVAvrzQjd/01lF1cejkstw/9ZZKN7eO7kr/Tb6RSM
P/mkz/MCDBxj/KxIXmE3olFDyWo1ha6EMYX9W7NRKeL96hIz/G08rw5pjrsqI88r
WIW4qmcCsBeW3G8OJDxHqxcKYg6rAYWeCV9zTi+/F3ZNWPWrco+pRk3z/Lxdw4pz
8FI6lLoHuQYFzhl4Luk5l/eEMm+MlT2FLHw+Y459Fv4Xda0fnjTGd63rUYXZozMr
aUuHKSJA+/rT67dUZvDopyyewHZ/ZCO5Xw1AbK5+Fqmfjva/FkWi8d9qILjI76uu
dJypBcHK0rfSTqt/8+yJgKRKgt0/PbdispZQm78uTyvR8S1S7v91aPTD5eVAatPU
kRCqMo5aXknJOW6ydZaXf8VWGKT1V+Zgru0Z16kNRcDUlyz1/adAb9lg5KRfmb1b
do/BAgMBAAGjczBxMB0GA1UdDgQWBBSH6v5qsLJATqd0T8WPBN2shx2gyTAfBgNV
HSMEGDAWgBSH6v5qsLJATqd0T8WPBN2shx2gyTAPBgNVHRMBAf8EBTADAQH/MB4G
A1UdEQQXMBWHBLt8JBGCDTE4Ny4xMjQuMzYuMTcwDQYJKoZIhvcNAQELBQADggIB
AHCg5FchEZ83TSTdGSOj5vt4GPDS6ui0uqlbBfo+8EA0ebsFAbuYntaGZpwtKjnA
XJwGyLzpqWdi4rHAps+MvIHqZtSgyEAHHbTizxLBlm5hEs056i1RKVKEoXI7zzzp
NKP/8zFD6PaDf5ADipfaCslS1qGiq3NE2hQgP+CfXbPwtwDNk39x7s5lBbAqhxkF
u0sVpvkRI6dKfM924z9br36+6l4Hezj6rUotAKLURH6xNjVHhmAak0fHtng9V6Es
PAvXPRJLRzryo3/VEfOPRt11jJVqaLwOmM7iozJbklrwB1G4w6CjisAQMrAIklla
0yf27lv/7F1jqMoQrNYPwc+dkHzNzB4Az3dLDwTB3KGPvJSUQ3JV9JNRkRAW2sgK
q/lWd9Kb3Wo8BZHhQ98JJC0ScVhiG2YjJIcmh9sZErV2uDd+2yhwfosaimbzlon8
Wj37cB7i1x4PXFtp24aQHQ7mUv2cxsfB2w+MDBatFvzhAA1/Hy7cvuQ4KZTkEgNm
pXjrxoCy52U03Dg+mP+M6/nLoUoEoHVynKCg88Pb/lSIzimVkrBBFZVCVQTgzX7a
Ip2hgJevjXoqdE4YK0+00pDoOP1FqjiMAk2jeL/OvGN2nsbCtK6odPJWRc5fhbAr
FTBcsPzrAWcHVC6pyO736cx61aCEE2V/AVdF79rXpaVA
-----END CERTIFICATE-----
)";

// ─── Topics ──────────────────────────────────────────────────────────────────

String topicTelemetry() { return "jamonero/" + String(DEVICE_ID) + "/telemetry"; }
String topicStatus()    { return "jamonero/" + String(DEVICE_ID) + "/status"; }
String topicCommand()   { return "jamonero/" + String(DEVICE_ID) + "/command"; }

// ─── Sensor simulation ───────────────────────────────────────────────────────
// Simulates a MAP cycle: O2 drops from ~21% to ~0.3% over 10 minutes, then holds

float simulateO2() {
    static float o2 = 20.9;
    static bool purging = true;
    if (purging) {
        o2 = max(0.3f, o2 - 0.4f);
        if (o2 <= 0.3f) purging = false;
    }
    return o2 + random(-5, 5) * 0.01;
}

float simulateTemp()     { return 4.0 + random(-20, 20) * 0.05; }   // 3-5°C
float simulateHumidity() { return 75.0 + random(-50, 50) * 0.1; }   // 70-80%

// ─── MQTT + WiFi ─────────────────────────────────────────────────────────────

WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
unsigned long lastPublish = 0;
const long publishInterval = 10000;  // 10 seconds

void onCommand(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    Serial.println("CMD received on " + String(topic) + ": " + msg);
    // TODO: parse commands (O2 setpoint, MAP cycle start/stop, OTA trigger)
}

void connectWifi() {
    Serial.print("Connecting WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // macros from secrets.h
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi OK — IP: " + WiFi.localIP().toString());
}

void connectMqtt() {
    espClient.setCACert(CA_CERT);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onCommand);
    mqtt.setBufferSize(512);

    String clientId = "jamonero-" + String(DEVICE_ID);
    String willTopic = topicStatus();
    String willMsg = "{\"online\":false,\"device_id\":\"" + String(DEVICE_ID) + "\"}";

    Serial.print("Connecting to HiveMQ");
    while (!mqtt.connected()) {
        if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                         willTopic.c_str(), 1, true, willMsg.c_str())) {
            Serial.println(" OK");
            // Publish online status
            String onlineMsg = "{\"online\":true,\"device_id\":\"" + String(DEVICE_ID) + "\"}";
            mqtt.publish(willTopic.c_str(), onlineMsg.c_str(), true);
            // Subscribe to commands
            mqtt.subscribe(topicCommand().c_str(), 1);
        } else {
            Serial.print(" Failed (rc=" + String(mqtt.state()) + "), retrying in 5s");
            delay(5000);
        }
    }
}

void publishTelemetry() {
    JsonDocument doc;
    doc["device_id"]    = DEVICE_ID;
    doc["timestamp"]    = millis() / 1000;
    doc["o2_pct"]       = round(simulateO2() * 100) / 100.0;
    doc["temp_c"]       = round(simulateTemp() * 10) / 10.0;
    doc["humidity_pct"] = round(simulateHumidity() * 10) / 10.0;
    doc["map_active"]   = (doc["o2_pct"].as<float>() < 1.0);
    doc["cycle_id"]     = "MAP-001";

    char buffer[512];
    serializeJson(doc, buffer);
    mqtt.publish(topicTelemetry().c_str(), buffer, false);  // QoS 0 for high-frequency telemetry
    Serial.println("TX → " + String(buffer));
}

// ─── Arduino loop ────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== MAP HoReCa Device × HiveMQ POC ===");
    connectWifi();
    connectMqtt();
}

void loop() {
    if (!mqtt.connected()) connectMqtt();
    mqtt.loop();

    unsigned long now = millis();
    if (now - lastPublish > publishInterval) {
        lastPublish = now;
        publishTelemetry();
    }
}
