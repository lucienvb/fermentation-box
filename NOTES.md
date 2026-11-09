# Notes

## Besluiten (uit het projectdocument, hoofdstuk 2)

### Basis (4 oktober 2026)
- Board: ESP32-CAM (AI-Thinker, OV2640) met ESP32-CAM-MB-shield. Reden: goedkoop, zuinig, camera ingebouwd, microSD-sleuf; embedded werk is waar dit project technisch moet groeien.
- Firmware: Arduino-framework (C++) via PlatformIO. Reden: meeste voorbeelden/libraries voor camera en DHT22 zijn voor Arduino; vaste versies in `platformio.ini`, native tests op de laptop; bewezen in de thuismeter.
- Sensor: DHT22. Reden: goed genoeg voor zuurdesem en kombucha; zwak bij hoge luchtvochtigheid/condens, later te vervangen (bv. SHT4x) vóór koji; daarom achter een interface (`sensor.h`).
- Eerste product: zuurdesem (het aanmaken van de starter is de eerste batch). Reden: makkelijk, kort ritme, rijzing zichtbaar op foto's.
- Opstelling: dichte, ondoorzichtige plastic opbergbox. Reden: nodig voor later verwarmen; geen daglicht dat de foto's beïnvloedt.
- Opslag: lokaal op microSD, later naar Supabase met een device-token. Reden: de SD-buffer is later toch nodig bij wifi-storingen.
- Voeding: USB-lader 5V, minimaal 2A, korte goede kabel. Reden: brownouts bij zwakke voeding of slechte kabel.
- Samenwerking: broer doet de behuizing en alles wat mechanisch is; ik doe elektronica, firmware en software.

### Tooling en ontwerp (6–7 oktober 2026)
- PlatformIO: de CLI (`pio`), Core 6.2.0, platform `espressif32@6.9.0`. Reden: Claude Code en Conductor werken in de terminal.
- Conductor en Claude Code: parallelle workspaces mogen, maar één agent tegelijk uploadt. Reden: één board aan één USB-poort; zelf overzicht houden over het echte board.
- Rust: nu niet, kandidaat voor de regelbox bij het verwarmen later.
- Betrouwbaarheid: watchdog, herstartreden loggen, eerst opslaan dan versturen, vaste buffers. Zie hoofdstuk 6.5 van het projectdocument.
- SD-kaart: microSDHC, 8–32 GB, FAT32, bekend merk. Reden: groter dan 32 GB gebruikt exFAT.
- Opstelling: elektronica nooit in de pot; camera van buitenaf, sensor meet de lucht, board hoger dan de pot.
- Licht: nu een USB-ledlampje, altijd aan, neutraal wit; later een geschakeld ledje via een transistor.
- Voeding via het MB-shield: een gewone 5V-USB-lader van minstens 2A in de micro-USB van het shield is veilig.

### Start fermentatiebox (5 november 2026)
- Nu beginnen met de fermentatiebox, geen extra spikes meer in de thuismeter. Reden: de thuismeter heeft het algemene board-werk al bewezen.
- Nieuwe repo. Modules uit de thuismeter **kopiëren**, nog geen gedeelde library. Reden: twee voorbeelden zijn te weinig om te weten wat echt gedeeld moet worden.
- Hergebruik: `sensor`, `wifi_time`, `diag`, `ota`, `version`, `retention` + native test, CI-opzet (`build.yml`, secrets-placeholder, `native`-env).
- Tot de cloud er is: een lokale webpagina op het board (komt in F3, niet in F0).
- OTA vanaf de eerste firmware. Reden: de handmatige USB-uploadprocedure met dit shield is omslachtig (zie hieronder); OTA scheelt veel gedoe.
- Board: een ander exemplaar dan de thuismeter. De thuismeter blijft draaien.
- Taal (6 november 2026): Engels in de repo (code, commentaar, logregels, commits, PR's, `README.md`, `CLAUDE.md`, `docs/`). Nederlands blijft: het projectdocument, `NOTES.md` (dit bestand) en `docs/behuizing.md`.

## Pinkeuze DHT22 (F0, bevestigd in F1)

**Keuze: GPIO13** (`src/sensor.cpp`), dezelfde pin als in de thuismeter. Daar is geen SD-kaart en geen camera, dus GPIO13 is vrij. In 4-bit SD-modus is GPIO13 echter bezet door de SD-kaart. Zodra de SD-kaart in F1 meedoet, wordt de pin opnieuw bepaald (1-bit SD-modus vermoedelijk GPIO2, 14, 15 — nog te bevestigen) uit de officiële documentatie, niet gegokt. Zie ook de opmerking in `src/sensor.h`.

**Bevestigd (9 november 2026, F1):** GPIO13 blijft vrij. De ESP32 heeft maar één SDMMC-slot (slot 1) met vaste pinnen, niet instelbaar via de GPIO-matrix (dat kan alleen op de ESP32-S3): CLK=GPIO14, CMD=GPIO15, D0=GPIO2, D1=GPIO4, D2=GPIO12, D3=GPIO13. In 1-bit-modus (`SD_MMC.begin("/sdcard", true)`) gebruikt de driver alleen CLK/CMD/D0; D1/D2/D3 (GPIO4, 12, 13) worden niet geconfigureerd in `sdmmc_slot_config_t` en blijven dus vrij. Bronnen:
- Espressif, ESP-IDF SDMMC Host Driver (officiële docs, slot 1 pin-tabel): https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/sdmmc_host.html
- `espressif/arduino-esp32`, `libraries/SD_MMC/src/SD_MMC.cpp` en `.h` (broncode: `begin(mountpoint, mode1bit=false, ...)`, `_pin_d1/_pin_d2/_pin_d3` blijven `-1` als mode1bit en je ze niet zelf instelt).

**GPIO4 (flitser) in 1-bit-modus:** dus NIET door de SD-driver gebruikt (dat is alleen de D1-lijn in 4-bit-modus). `sd_storage.cpp` zet GPIO4 voor de zekerheid toch expliciet laag direct na `SD_MMC.begin()`, onafhankelijk van of `Camera::begin()` daarna lukt — zie `kFlashLedPin` in `sd_storage.cpp` en `camera.cpp`. Op hardware nog te bevestigen dat de LED niet oplicht tijdens het starten van de SD-kaart (zie eindmelding).

**Camerapinnen (CAMERA_MODEL_AI_THINKER):** letterlijk overgenomen uit het officiële CameraWebServer-voorbeeld van Espressif:
https://raw.githubusercontent.com/espressif/arduino-esp32/master/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h

## Handmatige uploadprocedure via USB

De automatische reset en de IO0/RST-knoppen van het (oude) MB-shield werkten in de thuismeter niet ("No serial data received"). Wat wel werkte:

1. Board los naast het shield, verbonden met 4 draadjes: 5V, GND, U0R, U0T.
2. Uploadstand: IO0-draadje naar GND, dan resetten door de 5V-draad los en vast te maken (USB blijft erin).
3. `pio run -t upload`.
4. IO0-draadje weg, opnieuw resetten via de 5V-draad om normaal op te starten.

Daarna via OTA (hostname `fermentatiebox.local`); USB blijft het vangnet als OTA ooit niet werkt.

Diagnoseladder bij uploadproblemen: zie poort (`pio device list` / `ls /dev/cu.*`), eventueel een TX-RX-loopbacktest, opstarttekst (`boot:0x13` = normaal, `boot:0x3 … waiting for download` = uploadstand), dan pas uploaden.

## F1: SD-kaart (1-bit) + camera + DHT22 tegelijk (spike, 9 november 2026)

- Nieuwe modules: `paths.h/.cpp` (pure padfuncties, native getest vóór de implementatie), `sd_storage.h/.cpp` (SD-start in 1-bit-modus, CSV-regel wegschrijven, foto wegschrijven), `camera.h/.cpp` (overgenomen uit de thuismeter, Engels, zelfde SVGA/PSRAM-logica, geen flitser voor het vaste schema).
- `FIRMWARE_VERSION` naar `0.2.0`.
- Eén gecombineerd interval (1 minuut) voor zowel meting als foto in deze spike; F2 splitst dit in een 1-minuut-meting en een 15-minuten-fotoschema (los, pure functies, tests eerst).
- `SD_MMC.begin("/sdcard", true, /*format_if_mount_failed=*/false)`: kaart wordt NIET geformatteerd bij een mislukte mount (voorkomt dataverlies op een kaart met oude data); een mislukte mount wordt alleen gelogd.
- Nog niet gemeten op hardware: werkelijke kaartgrootte/vrije ruimte, `psramFound()`, vrije heap/PSRAM, grootte van één SVGA-foto, of de flitser-LED echt uit blijft tijdens het starten van de SD-kaart. Zie de vragen in de eindmelding van de F1-PR.

## Nog te doen

- Op hardware testen: eerste USB-upload, wifi-verbinding, NTP-sync, DHT22-meting, watchdog, OTA-upload naar 0.1.1 (stappen staan in de PR-beschrijving/eindmelding van F0).
- Op hardware testen: F1-spike (SD + camera + DHT22 tegelijk, OTA naar 0.2.0) — zie eindmelding van de F1-PR voor wat ik in de seriële monitor moet zien.
- F2: elke minuut een meting en elke 15 minuten een foto naar de SD-kaart, fotoschema en bewaarbeleid als pure functie met tests eerst. SD-bewaarbeleid (`retention.h/.cpp` bestaat al als pure functie, nog niet aan SD gekoppeld).
- F3: lokale webpagina (basis uit de thuismeter, `web.cpp`).
- Later: cloud-koppeling (Supabase, device-token), doelbereik/alerts, verwarmen.
