# Fermentatiebox

Hobby project: an ESP32-CAM (AI-Thinker) in a closed box that records
temperature, humidity and photos of a fermentation (starting with sourdough),
with a local web page and later a web app in the cloud. Later: control
(heating, humidity), CO2 and AI advice. The enclosure is built by my
brother; requirements are in docs/behuizing.md. Many modules come from the
home-monitor project.

## Rules
- When you give information, be very concise and sacrifice grammar for the sake of conciseness.
- Everything in this repo is in English (code, comments, logs, commits, PRs, docs),
  except NOTES.md and docs/behuizing.md, which stay Dutch.
- Small steps: one phase per branch and PR. Working first, then nice.
- This file is the source of truth. If it's wrong, fix it in the same PR.
- Secrets only in secrets.example.h (placeholders). Never commit secrets.h.
- Every choice (pin, threshold, interval) and every temporary simplification goes in NOTES.md, with date and reason.
- Look up current official docs. Never guess pinouts. Camera pins come from Espressif's camera_pins.h.
- New libraries only after approval. Versions pinned in platformio.ini, same in CI.
- Tests first for the photo schedule, retention policy, upload buffer, alerts and control logic (native env).
- One module per topic, with a small interface.
- Every firmware PR bumps FIRMWARE_VERSION; the PR description takes that version from the file.
- The agent does not run hardware commands: it prepares the command and says what I should see.

## Box (firmware)
- Every firmware build includes OTA. USB is the fallback.
- Watchdog on (also during OTA), log the reset reason.
- Time via NTP, internally UTC. No measurements without synced time.
- Save first (SD), then upload; only mark as uploaded after confirmation. (Not built yet: storage lands in F2.)
- Log storage/wifi errors, never break the measurement or the web page. No abort().
- Fixed buffers, snprintf, no String concatenation in loops; release photo buffers immediately.
- DHT22 on 3V3, never 5V. Avoid strapping pins and GPIO4 (flash LED).
- Flash LED not used for the fixed photo schedule; light comes from a USB lamp.
- Control happens on the box. Software is never the only safeguard against overheating.
- The box holds a device token, NEVER the Supabase service-role key.

## Cloud (from F5)
- Supabase: RLS on all tables, USING + WITH CHECK. System tables deny-by-default.
- Backend filters by user_id itself. Roles in user_roles.
- Migrations YYYYMMDDHHMMSS_description.sql. Upsert on unique(device_id, measured_at).
- Aggregations in SQL, time buckets in Europe/Amsterdam.
- No people in frame; no personal data in metadata.

## Structure and commands

### Stack
- PlatformIO Core (CLI), environments `esp32cam` (USB), `esp32cam_ota` (OTA, extends `esp32cam`), `native` (retention test only).
- Framework: Arduino. Platform: `espressif32@6.9.0` (pinned in platformio.ini).
- Libraries: `adafruit/DHT sensor library@1.4.7`, `adafruit/Adafruit Unified Sensor@1.1.15`. No other libraries.
- mDNS / OTA hostname: `fermentatiebox`.

### Modules (src/)
- `sensor.h/.cpp` — DHT22 reading. Pin GPIO13 for now (see NOTES.md; revisited in F1 once the SD card is in play).
- `wifi_time.h/.cpp` — wifi connection (with reconnect) and NTP time, internally UTC.
- `diag.h/.cpp` — reset reason.
- `ota.h/.cpp` — ArduinoOTA (wireless firmware update), hostname `fermentatiebox`, password from `include/secrets.h` (`OTA_PASSWORD`).
- `retention.h/.cpp` — pure function deciding which day files to delete under low free space. Not wired into the firmware yet (storage lands in F2); covered by a native test so that env stays meaningful from F0 onward.
- `version.h` — `FIRMWARE_VERSION`, bumped on every firmware PR.
- `main.cpp` — stays short: setup/loop calling the modules. F0 scope: measure every minute, connect wifi, sync NTP, OTA, watchdog. No camera, no SD card, no web page (F1–F3).

### Commands
- Build (`esp32cam`, the default): `pio run`
- Upload via USB: `pio run -t upload` (manual procedure, see NOTES.md — I run this myself)
- Upload via wifi (OTA): `OTA_PASSWORD=... pio run -e esp32cam_ota -t upload` (I run this myself)
- Native unittest of the retention policy (no hardware needed): `pio test -e native`
- Connected boards: `pio device list`
- Serial monitor (always with a time limit): `pio device monitor`

### CI
GitHub Actions (`.github/workflows/build.yml`), on push to `main` and on `pull_request`: PlatformIO 6.2.0, copy `secrets.example.h` to `secrets.h`, `pio run`, `pio test -e native`.
