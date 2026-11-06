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

## Pinkeuze DHT22 (F0)

**Keuze: GPIO13** (`src/sensor.cpp`), dezelfde pin als in de thuismeter. Daar is geen SD-kaart en geen camera, dus GPIO13 is vrij. In 4-bit SD-modus is GPIO13 echter bezet door de SD-kaart. Zodra de SD-kaart in F1 meedoet, wordt de pin opnieuw bepaald (1-bit SD-modus vermoedelijk GPIO2, 14, 15 — nog te bevestigen) uit de officiële documentatie, niet gegokt. Zie ook de opmerking in `src/sensor.h`.

## Handmatige uploadprocedure via USB

De automatische reset en de IO0/RST-knoppen van het (oude) MB-shield werkten in de thuismeter niet ("No serial data received"). Wat wel werkte:

1. Board los naast het shield, verbonden met 4 draadjes: 5V, GND, U0R, U0T.
2. Uploadstand: IO0-draadje naar GND, dan resetten door de 5V-draad los en vast te maken (USB blijft erin).
3. `pio run -t upload`.
4. IO0-draadje weg, opnieuw resetten via de 5V-draad om normaal op te starten.

Daarna via OTA (hostname `fermentatiebox.local`); USB blijft het vangnet als OTA ooit niet werkt.

Diagnoseladder bij uploadproblemen: zie poort (`pio device list` / `ls /dev/cu.*`), eventueel een TX-RX-loopbacktest, opstarttekst (`boot:0x13` = normaal, `boot:0x3 … waiting for download` = uploadstand), dan pas uploaden.

## Nog te doen

- Op hardware testen: eerste USB-upload, wifi-verbinding, NTP-sync, DHT22-meting, watchdog, OTA-upload naar 0.1.1 (stappen staan in de PR-beschrijving/eindmelding van F0).
- F1: SD-kaart in 1-bit-modus + camera + DHT22 tegelijk uitzoeken; pin van de DHT22 opnieuw bepalen; checken of de flitser-LED oplicht bij het starten van de SD-kaart; grootte van één SVGA-foto meten.
- F2: elke minuut een meting en elke 15 minuten een foto naar de SD-kaart, fotoschema en bewaarbeleid als pure functie met tests eerst.
- F3: lokale webpagina (basis uit de thuismeter, `web.cpp`).
- Later: cloud-koppeling (Supabase, device-token), doelbereik/alerts, verwarmen.
