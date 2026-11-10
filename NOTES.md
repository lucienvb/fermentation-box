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

**Op hardware bevestigd (9–10 november 2026):** SD-kaart (14910 MB, 14893 MB vrij), camera (SVGA, PSRAM: ja) en DHT22 starten alle drie goed, `psramFound()`: ja, vrije heap 238632 bytes, vrije PSRAM 3973123 bytes direct na opstarten. Na een voedingswissel (zie brownout-bevinding hieronder) heeft het board twee dagen achter elkaar aan het stopcontact gehangen en staan er voor beide dagen een CSV-bestand in `/data` en een map met jpg's in `/photos` op de kaart: de volledige cyclus (SD + camera + DHT22 + wifi, elke minuut meting + foto) draait dus stabiel door. Nog niet apart gemeten: bytegrootte van één foto en capture/save-tijd in ms (staan al in de serial-log als het board aan de PC hangt, nog niet doorgegeven).

**Brownout bij opstarten (9 november 2026):** board bleef resetten ("Brownout detector was triggered") steeds net na het printen van de opstartdiagnostiek, vóór de wifi-verbinding lukte — `Diag::resetReasonString()` gaf hier terecht `brownout (power supply too low)`. Oorzaak: voeding via de datakabel van een PC-USB-poort (vaak maar 0,5–0,9 A); SD + camera (PSRAM, 2 framebuffers) + wifi-radio trekken samen een stroompiek die de PC-poort niet aankan. **Fix: voeden via een losse 5V/2A+ wandlader in plaats van de PC**, zoals al in hoofdstuk "Basis" hierboven staat — met die voeding draaide het board stabiel door (zie boven). Dit is dus geen firmwarebug; wel een reden om bij toekomstige stroompieken (bv. als het verwarmen erbij komt) eerst de voeding te controleren vóór je in de code gaat zoeken.

## F2: vast fotoschema, bewaarbeleid aan de SD-kaart, bootlog (10 november 2026)

- Nieuwe module `photo_schedule.h/.cpp` (pure functie, geen Arduino-afhankelijkheid, native getest vóór de implementatie): `slotStart()` rondt een UTC-tijd af naar de vaste kloktijd-tijdvakken (:00/:15/:30/:45 bij 15 minuten), `isDue()` vergelijkt het tijdvak van nu met het laatst vastgelegde tijdvak. Werkt door direct op de epoch af te ronden (epoch 1970-01-01 00:00:00 UTC ligt al op elke tijdvakgrens), dus geen aparte dag/uur-berekening nodig; werkt voor elk interval dat 60 deelt (15 en 5 beide getest).
- Dit vervangt de F1-aanpak (één gecombineerd interval op een geheugenteller): de meting blijft een vaste 1-minuut-geheugenteller (`kMeasurementIntervalMs` in `main.cpp`), de foto wordt nu bepaald uit de klok. Reden: een teller "mist" een tijdvak na een late herstart of maakt een dubbele foto net na een herstart; de klok niet.
- Na een herstart (tijdvak-status in het RAM dus leeg, sentinelwaarde `0`) wordt alsnog precies één foto gemaakt voor het lopende tijdvak, ook als het board er middenin terugkomt — bewust: liever één foto niet exact op de kloktijd dan een gemist tijdvak.
- Bewaarbeleid (`retention.h/.cpp`, al in F0 als pure functie) nu aan de SD-kaart gekoppeld via `SdStorage::enforceRetention()`: werkt op de dagmappen onder `/photos` (naam zonder `.csv`-extensie, in tegenstelling tot de oorspronkelijke F0-tests die cijfer-CSV-bestandsnamen gebruikten — de functie is naam-gebaseerd en dus generiek genoeg gebleven). CSV-bestanden onder `/data` komen niet in de lijst die aan `filesToDelete()` wordt gegeven, dus die worden nooit verwijderd. De map van vandaag wordt expliciet als `currentDayFile` doorgegeven.
- Ondergrens: **500 MB vrij** (`Retention::kMinFreeBytes`), als constante. Reden: ruime marge t.o.v. de ~60 KB per foto uit F1; bij deze kaartgrootte (14,9 GB) duurt het jaren voor dit ooit wordt geraakt, maar de check moet er toch zijn voor als de kaart kleiner is of vol loopt door iets anders.
- `DayFileInfo.sizeBytes`, en `freeBytes`/`minFreeBytes` in `filesToDelete()`, zijn van `uint32_t` naar `uint64_t` gegaan: een echte SD-kaart (14,9 GB, F1) past niet in `uint32_t`-bytes (max ~4,3 GB). Dit was tot nu toe onopgemerkt omdat de F0-tests met kleine testwaarden draaiden.
- Controle hooguit één keer per uur (`kHourlyMs` in `main.cpp`), niet bij elke foto — dagmap-groottes optellen loopt over de hele `/photos`-boom en is te duur om vaker te doen.
- Elke opstart een regel naar `/data/boots.csv` (`SdStorage::writeBootRecord()`): `epoch,firmware_version,reset_reason`. Geschreven zodra de tijd gesynchroniseerd is (niet eerder: een epoch heeft een gesynchroniseerde klok nodig) en zodra de SD-kaart klaar is; de epoch wordt gecorrigeerd met de verstreken `millis()` sinds opstarten, zodat hij het moment van opstarten weergeeft en niet het moment waarop NTP klaar was.
- SD-kaart niet beschikbaar bij opstarten, of een latere schrijffout (meting, foto of bootlog): loggen, blijven meten/fotograferen (zonder SD-opslag), en elke 5 minuten (`kSdRetryIntervalMs`) opnieuw `SdStorage::begin()` proberen. Een schrijffout zet `sdReady` ook op `false`, zodat die ook de hermount-cyclus in gang zet (niet alleen een mislukte start).
- Eén statusregel per uur in de seriële monitor (`kHourlyMs`, zelfde interval als het bewaarbeleid maar los bijgehouden): aantal foto's, aantal metingen, aantal fouten sinds opstarten, vrije ruimte op de kaart, vrije heap.
- `FIRMWARE_VERSION` naar `0.3.0`.

## Nog te doen

- Op hardware testen: eerste USB-upload, wifi-verbinding, NTP-sync, DHT22-meting, watchdog, OTA-upload naar 0.1.1 (stappen staan in de PR-beschrijving/eindmelding van F0).
- ~~Op hardware testen: F1-spike (SD + camera + DHT22 tegelijk, OTA naar 0.2.0).~~ Gelukt op 9–10 november, zie hierboven. Nog door te geven: bytegrootte van één foto en capture/save-tijd in ms uit de serial-log.
- Op hardware testen: F2 (fotoschema, bewaarbeleid, bootlog, OTA naar 0.3.0) — één nacht doordraaien.
- **Als de SD-kaart uitvalt, gaan metingen en foto's verloren** (geen terugval naar LittleFS). Bewust, voorlopig: LittleFS heeft met OTA maar ~1,4 MiB ruimte (zie projectdocument §7, punt 2), genoeg voor metingen maar niet voor foto's; een aparte terugvalopslag is meer ontwerpwerk dan deze fase toelaat.
- F3: lokale webpagina (basis uit de thuismeter, `web.cpp`).
- Later: cloud-koppeling (Supabase, device-token), doelbereik/alerts, verwarmen.
