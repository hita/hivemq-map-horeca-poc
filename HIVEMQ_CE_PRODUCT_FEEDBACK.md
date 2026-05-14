# HiveMQ CE — Product Feedback from a First-Time Self-Managed Deployment

**Context:** Deployed HiveMQ CE 2025.5 on a Ubuntu 24.04 VPS (Docker) to validate an MQTT-based IoT architecture for a MAP (Modified Atmosphere) HoReCa device. ESP32-S3 device publishing telemetry over MQTT/TLS. Built with Claude Code as the development assistant. Total time to working end-to-end: ~90 minutes.

This feedback has two layers. The first is tactical: what broke, what worked, what I'd fix in the CE developer experience. The second is strategic: how CE functions as a product-led growth motion toward HiveMQ Platform, where that motion works, and where it breaks down.

---

## Part I — The CE → Enterprise Upgrade Journey

### CE is a PLG motion. The question is whether it's a well-designed one.

HiveMQ CE is not a product in isolation. It's an acquisition channel — a way for developers to validate MQTT architecture with a real broker before their organization commits to HiveMQ Platform. The quality of that channel determines whether the upgrade happens with HiveMQ or with a competitor.

A well-designed PLG motion does three things:
1. Lets the user succeed at their current stage (POC, prototype, small fleet)
2. Surfaces the gaps explicitly as they approach the next stage
3. Makes the upgrade path obvious when they hit the ceiling

CE does the first reasonably well. It mostly fails at the second and third.

---

### The Journey in Stages

**Stage 1: POC — CE is the right tool**

Single device, TLS setup, basic telemetry validation. This is exactly what CE is for. The developer is answering: *"Does MQTT work for my architecture? Can I connect an ESP32 to a self-managed HiveMQ broker over TLS?"* CE answers both questions. Nothing more is needed here.

**Stage 2: Small fleet validation (2–20 devices)**

The first signals should appear here, but they don't. As soon as a second device connects, the developer needs:

- **Fleet visibility**: which devices are online, message rates per topic, connection health. Without Control Center, there's no answer — the developer writes scripts or installs MQTT Explorer. The gap is real but invisible. There's no moment where CE says *"you're ready for something more."*
- **Basic auth**: allow-all is correct for POC. At device 2, the question becomes *"should any client be allowed to publish to any topic?"* The security warning fires on every boot but doesn't link to a solution.
- **Topic ACLs**: multi-device architectures immediately surface the need to isolate device namespaces. CE has no answer; the developer doesn't know where to look.

**Stage 3: Pre-production (first real customer, 20+ devices)**

This is where CE structurally can't serve the user, and the upgrade becomes necessary:

| Need | CE | Platform |
|---|---|---|
| High availability / clustering | ✗ | ✓ |
| Control Center (fleet observability) | ✗ | ✓ |
| Enterprise Security Extension (auth/authz, LDAP, OAuth 2.0) | ✗ | ✓ |
| Data Hub (schema validation, payload transformation) | ✗ | ✓ |
| Kafka / database integrations | ✗ | ✓ |
| Audit logging (compliance, GDPR, HACCP) | ✗ | ✓ |
| Professional support (SLA) | ✗ | ✓ |

For a food safety application — which this MAP device POC represents — the Data Hub and audit logging gaps are determinative. A production deployment serving HoReCa operators needs schema validation at the broker (is this a valid MAP telemetry payload?), traceable device status changes, and a compliance audit trail. None of these are available in CE. The developer doesn't discover this gradually; they hit it as a wall.

**Stage 4: Production — you're in sales territory**

At this point the conversation is Platform vs. a competitor, not CE vs. Platform. The PLG motion has either worked or it hasn't.

---

### Where the PLG Motion Breaks Down

**The upgrade signal is absent at the moment of friction.**

When a developer can't see their fleet (no Control Center), the correct response is: *"Cluster visibility is available in HiveMQ Platform — [link]."* Instead, CE is silent. The developer concludes the tool doesn't have this capability and starts evaluating alternatives.

**The security warning is punishment, not invitation.**

The ASCII banner warning about unauthenticated access appears on every boot. It creates anxiety without offering a next step. The correct framing is: *"CE runs in allow-all mode. When you're ready to enforce authentication and topic-level authorization, HiveMQ Platform includes the Enterprise Security Extension."* One sentence converts a warning into a conversion moment.

**Data Hub and the Kafka extension aren't visible from CE at all.**

A developer building an IoT pipeline — the exact buyer HiveMQ wants — needs to connect their MQTT broker to Kafka, validate schemas at ingestion, and pipe telemetry to analytics systems. These are core Data Streaming use cases. CE gives no indication these capabilities exist. The developer discovers them through separate research, if at all.

**No upgrade CTA anywhere in the CE experience.**

No startup log. No documentation callout. No link in the error messages. CE is a dead end by design, when it should be a funnel.

---

### What Good Looks Like

HashiCorp Vault, Grafana OSS, and Confluent Community Edition all solve this problem differently, but share a common pattern: when the user hits a capability ceiling, the product tells them explicitly what the ceiling is and what's on the other side.

For HiveMQ CE, the minimum viable version of this looks like:

1. **On startup**, a one-line summary: *"Running HiveMQ CE — no clustering, no Control Center, no Enterprise Extensions. See hivemq.com/platform for production capabilities."*
2. **On the security warning**, add: *"To enforce authentication: hivemq.com/enterprise-security-extension"*
3. **In docs**, a single page titled "CE vs. Platform" that maps each CE limitation to the Platform feature that resolves it — with an honest answer about which stage each feature becomes necessary.

None of this requires changing the CE product. It's documentation and log messaging.

---

## Part II — CE Developer Experience: Tactical Friction Points

These are distinct from the PLG gaps above. Where PLG gaps are strategic (CE doesn't convert), DX friction points are operational (CE is harder to set up than it should be). Both matter, but they have different owners and different urgency.

### 1. Control Center removed from CE — not documented prominently

**What happened:** `config.xml` includes a `<control-center>` section. The broker starts, port 8080 is mapped by Docker, but nothing serves HTTP. There's no warning in the logs that Control Center is an Enterprise-only feature. The config section is silently accepted and ignored.

**Impact:** A developer following any documentation from HiveMQ 4.x expects a management UI. They'll debug port accessibility, firewall rules, and Docker networking before discovering the feature was removed. ~20 minutes wasted.

**Recommendation:**
- Log a clear `WARN` on startup when `<control-center>` is present in CE: *"Control Center is not available in HiveMQ Community Edition. Remove this section or upgrade to HiveMQ Platform."*
- Update the CE quick-start docs with a visible callout.

*Note: This is both a DX bug and a PLG gap. The silence around Control Center is the most common reason a CE developer reaches for MQTT Explorer or a competitor's managed broker.*

---

### 2. TLS setup requires silent debugging of file permissions

**What happened:** HiveMQ CE runs as a non-root user inside the container. Certificate files generated by root (chmod 600 by default from `openssl`) can't be read by the HiveMQ process. The error message is: *"Not able to create SSL server context. Reason: Cannot find KeyStore at path '/opt/hivemq/certs/hivemq.p12'"*

**Impact:** "Cannot find" is misleading — the file exists, it just can't be read. The container restarts repeatedly with the same message.

**Recommendation:**
- Distinguish `ENOENT` from `EACCES` in the error message. The OS provides this for free.
- Add a startup health check: *"KeyStore found at path X but not readable by current process (uid=10000). Check file permissions."*
- Document `chmod 644` explicitly in the TLS configuration guide.

---

### 3. `config.xml` read-only warning on every startup is noise

**What happened:** Every startup logs `WARN — The configuration file /opt/hivemq/conf/config.xml is read only and cannot be written by HiveMQ.` Expected behavior when mounting a read-only config from a Docker volume.

**Recommendation:** Downgrade to INFO or suppress when the config was successfully read. The write attempt is for dynamic config reload — it's not critical for operation.

---

### 4. Security warning drowns out legitimate startup errors

**What happened:** The large ASCII security banner spans ~8 lines on every startup. When TLS was failing, this banner appeared *before* the actual error, making log scanning harder.

**Recommendation:** Show it once on first launch, persist a flag to suppress on subsequent restarts. Or collapse to a single log line. The warning is appropriate — the format isn't.

---

### 5. Version naming discontinuity creates documentation confusion

**What happened:** CE 2025.5 is running. Most search results reference HiveMQ 4.x. Configuration format and feature availability differences between 4.x and 2025.x are not mapped anywhere obvious.

**Recommendation:** A single "4.x → 2025.x migration" page or a one-paragraph callout in the CE docs. The version naming change is the first thing a returning developer needs to understand.

---

## Part III — What Worked Well

- **Native TLS is solid.** Once permissions were fixed, TLS 1.3 came up cleanly with a standard PKCS12 cert. No JKS conversion needed — a meaningful DX improvement over older versions.
- **MQTT spec compliance is impeccable.** QoS 0/1, retained messages, LWT — all behaved exactly as the spec defines.
- **Docker startup is fast.** ~3 seconds to a listening broker. This matters for CI/CD loops and local development.
- **PKCS12 support without keytool.** The right call. Standard format, no Java ceremony.
- **Allow-all extension is the correct default.** Permissive for development, clearly warned. The behavior is predictable.

---

## Part IV — Recommendations for LLM Agents Working with HiveMQ CE

Non-obvious steps that will save debugging time:

1. **After generating certs with openssl, always `chmod 644` the .p12 and .key files** before starting the container. HiveMQ runs as UID 10000 and will silently fail to read root-owned 600 files.
2. **Remove the `<control-center>` section from config.xml in CE deployments.** It does nothing and creates false expectations.
3. **Test on plaintext port 1883 first**, then switch to TLS 8883. This isolates network/firewall issues from TLS configuration issues.
4. **The PKCS12 format is native** — no need to convert to JKS.
5. **For ESP32 with WiFiClientSecure:** use `setCACert()` with the PEM string of the server certificate. Self-signed certs work if the full cert is embedded. Do not use `setInsecure()` beyond initial local testing.
6. **HiveMQ CE 2025.x has no built-in management UI.** Use MQTT Explorer (desktop), mosquitto_sub (CLI), or a custom subscriber script for broker visibility.

---

## Summary

HiveMQ CE is a production-quality MQTT broker with excellent protocol compliance and fast setup. The core implementation is not the problem.

The problem is that CE doesn't behave like a product with a future for the developer using it. The upgrade path to Platform is invisible from inside the CE experience — no signals, no links, no explicit capability maps. A developer who hits the Control Center gap, the auth gap, or the Kafka integration gap has to discover HiveMQ Platform through separate research. Many won't. They'll reach for a hosted MQTT broker or a competitor's free tier instead.

The DX friction points are real and worth fixing, but they're second-order. The first-order issue is that CE is designed as a standalone product when it should be designed as the first step in a journey. The difference between those two things is mostly documentation, log messaging, and one comparison page — not engineering work.
