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

## F2b: opstelpagina om camera en pot te richten (10 november 2026)

- Nieuwe module `web.h/.cpp` (synchrone `WebServer`, zelfde model als het cameratabblad van de thuismeter): pagina `/`, endpoints `GET /api/photo` en `POST /api/photo/reference`. HTML/JS van de pagina zelf staat los in `web_setup_page.h/.cpp` (PROGMEM), zodat `web.cpp` niet te groot wordt — zelfde opsplitsing als `web_camera_tab.h/.cpp` in de thuismeter. Basis Auth op beide camera-endpoints (`CAMERA_USER`/`CAMERA_PASSWORD` in `include/secrets.h`, placeholder in `secrets.example.h`).
- Nieuwe module `camera_gate.h/.cpp` (pure functie, native getest vóór de implementatie): `CameraGate::canTakeTestPhoto()` beslist of een testfoto nu mag, met twee regels, in deze volgorde (bij allebei "nee" wint de eerste):
  1. **Fotoschema gaat voor.** Een geplande foto bezig (`Web::setScheduledPhotoBusy(true/false)`, gezet rond `Camera::capture()` in `main.cpp`'s `takePhoto()`) of binnen `kScheduleGuardSeconds` (5 s) gepland: 503, met `Retry-After`.
  2. **Snelheidslimiet op testfoto's.** Hooguit één per `kMinTestPhotoIntervalMs` (3000 ms): anders 429, met `Retry-After`.
- `GET /api/photo`: zelfde camera-instellingen als het schema (`Camera::capture(withFlash=false)`, dus ook eerst één frame weggooien — zie `camera.cpp`), `Cache-Control: no-store`. Testfoto's komen NIET op de SD-kaart. De foto wordt na het maken direct uit het camera-framebuffer gekopieerd naar een los PSRAM-buffer (`ps_malloc`) en de camera-buffer wordt onmiddellijk ge-`release()`d — zie CLAUDE.md "release photo buffers immediately". Dat PSRAM-buffer is ook wat "Save as reference" straks wegschrijft, zonder een nieuwe camera-aanroep.
- `POST /api/photo/reference`: schrijft dat laatste testfoto-buffer naar `/photos/reference/YYYY-MM-DD_HHMMSS.jpg` (UTC) via `SdStorage::saveReferencePhoto()` (nieuw, zelfde patroon als `savePhoto()`) en `Paths::referencePhotoPath()` (nieuw, native getest). 409 als er nog geen testfoto is.
- Rasteroverlay (kruis + derdelijnen) is puur in de browser (canvas, getekend over de `<img>`), geen firmware-kant.
- mDNS-hostname (`fermentatiebox.local`) komt al van `Ota::begin()` (ArduinoOTA registreert mDNS zelf); de opstelpagina heeft dus geen eigen mDNS-aanroep nodig.
- Eén camera-gebruiker tegelijk: niet apart afgedwongen met een lock — de hoofdloop is single-threaded en synchroon (`WebServer::handleClient()` en de geplande foto draaien nooit écht tegelijk), dus dat volgt al uit het bestaande ontwerp.
- `FIRMWARE_VERSION` naar `0.4.0`.

## F2c: gedimde flitser in plaats van een altijd-aan lampje (10 november 2026)

- Besluit: bij elke foto (schema én testfoto's) kort en gedimd de flitser-LED (GPIO4) aanzetten in plaats van het USB-lampje dat altijd aan staat (zie "Licht" onder "Tooling en ontwerp" hierboven). Reden: er is (nog) geen lampje dat permanent aanstaat bij de uiteindelijke opstelling; de flitser kort en op lage helderheid gebruiken voorkomt dat de pot de hele tijd verlicht/opgewarmd wordt door een lamp, en hergebruikt de LED die er al zit.
- GPIO4 in 1-bit SD-modus: al bevestigd in F1 hierboven (D1-lijn, niet gebruikt door de SD_MMC-driver in 1-bit-modus), zelfde bronnen (ESP-IDF sdmmc_host docs + arduino-esp32 SD_MMC-broncode).
- Helderheid nu PWM via LEDC in plaats van aan/uit (`digitalWrite`): `camera.cpp` zet GPIO4 op een eigen LEDC-kanaal (kanaal 7, 5 kHz, 8-bit resolutie — los van kanaal 0/timer 0 die de camera zelf gebruikt voor de XCLK-klok). Percentage (0–100%) → duty-waarde is pure logica in `flash_duty.h/.cpp` (`dutyForPercent`, native getest vóór de implementatie); 0% = flitser helemaal uit.
- Standaard 15% (`FlashSettings::kDefaultBrightnessPercent`), 200 ms opwarmtijd vóór het weggegooide frame (zelfde waarde als de thuismeter) — zie `camera.cpp`'s bestaande `kFlashWarmupMs`, ongewijzigd.
- `Camera::capture()` nam eerst een `bool withFlash`; nu een `int flashBrightnessPercent` (0 = geen flitser) zodat elke aanroeper zijn eigen helderheid kan kiezen: het schema gebruikt `Camera::flashBrightnessPercent()` (het ingestelde/opgeslagen percentage), een testfoto mag een ander percentage opgeven via `?brightness=` zonder het schema-percentage te veranderen.
- Opslag van het schema-percentage in NVS (`flash_settings.h/.cpp`, `Preferences`, namespace `"flash"`) zodat het een herstart overleeft. Geladen in `main.cpp`'s `setup()` na `Camera::begin()`, met `FlashSettings::kDefaultBrightnessPercent` (15%) als terugval zolang er nog niets is opgeslagen.
- Opstelpagina: keuzelijst (0/10/15/25/50/100%) voor testfoto's, laadt bij openen het huidige schema-percentage (`GET /api/flash/brightness`) als startwaarde; knop "Use for schedule" stuurt `POST /api/flash/brightness?percent=N`, dat zowel NVS bijwerkt als `Camera::setFlashBrightnessPercent()` direct toepast (geen herstart nodig). Elke testfoto toont het gebruikte percentage (uit de `X-Flash-Brightness-Percent`-responseheader, want de foto-response zelf is alleen de JPEG) naast de bestandsgrootte.
- Nooit aan blijven staan: `Camera::begin()` zet de LED bij elke opstart (ook na een watchdog-herstart) eerst expliciet op duty 0, vóórdat er iets anders gebeurt; `capture()` zet de LED ook op elk foutpad direct weer uit (ongewijzigd gedrag, nu via LEDC in plaats van `digitalWrite`).
- Bijwerking tijdens het bouwen: `sd_storage.cpp`'s bestaande defensieve "GPIO4 laag zetten na het mounten van de SD-kaart" (zie F1 hierboven) draaide bij élke aanroep van `SdStorage::begin()`, dus ook bij de 5-minuten-hermount-cyclus in `main.cpp` als de SD-kaart ooit mislukt. Een `pinMode()`/`digitalWrite()` op een pin die al aan een LEDC-kanaal hangt, koppelt die LEDC-koppeling los — dat zou de flitser na zo'n hermount stil laten stoppen met werken (niet aan laten blijven staan; failsafe, maar wel een regressie). Opgelost met een eenmalige guard (`flashPinInitialized`): de defensieve zet draait alleen nog bij de allereerste aanroep in `setup()`, vóór `Camera::begin()` de pin aan LEDC koppelt.
- `FIRMWARE_VERSION` naar `0.5.0`.

## F2d: kleine verbeteringen na proef 1 (12 november 2026)

- **Mislukte DHT22-metingen zichtbaar maken.** `sensor.cpp` doet nu één herhaalpoging na 2,5 s als een meting mislukt (zelfde interval als de minimale leesinterval van de DHT22-bibliotheek, dus een echte nieuwe meting i.p.v. de gecachte vorige waarde). Mislukt de meting dan nog steeds: toch een regel naar de dag-CSV, met lege waarden (`epoch,,`) in plaats van niets — zo is een gat in de data achteraf te verklaren als een mislukte meting i.p.v. een gemiste minuut. `SdStorage::writeMeasurement()` accepteert daarom nu ook een mislukte `SensorReading` (voorheen gaf de functie dan `false` terug zonder iets te schrijven).
- **Statusregel per uur naar `/data/status.csv`.** Nieuwe pure module `status_line.h/.cpp` (`StatusLine::formatLine()`, native getest vóór de implementatie): bouwt één CSV-regel (`epoch,firmware_version,uptime_s,readings_ok,readings_failed,photos_ok,photos_failed,sd_free_mb,free_heap,free_psram,last_error`) uit tellers die **sinds de vorige statusregel** lopen (niet sinds opstarten zoals de oude statusregel in `main.cpp` deed) — zo blijft de regel bruikbaar als "wat is er dit uur gebeurd" ook na dagenlang doordraaien. De tellers worden na elke regel gereset; `last_error` ook (bevat de laatst gelogde foutmelding sinds de vorige regel, leeg als er niets misging). Aanname: foutmeldingen bevatten geen komma (klopt voor alle foutstrings in deze firmware) — er wordt niet apart ge-escaped/ge-quote't, net als de andere CSV's in dit project. Dezelfde regel gaat ook naar de seriële monitor (`Serial.println`), ter vervanging van de oude losse statusregel. Schrijffout naar de SD-kaart: loggen, `sdReady` op `false` (zelfde hermount-cyclus als de andere SD-schrijffouten).
- `readings_ok`/`readings_failed` tellen het resultaat van de DHT22-meting zelf (na de retry), onafhankelijk van of de SD-schrijf daarna lukt; `photos_ok`/`photos_failed` tellen of de foto uiteindelijk ook echt opgeslagen is (zelfde als de bestaande logica al deed: geen SD-kaart telt ook als "niet opgeslagen"). SD-schrijffouten voor metingen/foto's zetten dus niet per se een apart teller, maar landen wel in `last_error`.
- **Vaste witbalans/belichting/gain.** Nieuwe module `camera_settings.h/.cpp` (zelfde opzet als `flash_settings.h`: NVS via `Preferences`, "Use for schedule"-knop op de opstelpagina, standaard alles op auto totdat iets opgeslagen is). Functies en waardebereiken rechtstreeks uit de officiële bron opgezocht (niet gegokt): `esp32-camera`'s `sensor.h` (gevendord in de Arduino ESP32-core, `~/.platformio/packages/framework-arduinoespressif32/.../esp32-camera/driver/include/sensor.h`) en `sensors/ov2640.c` (officiële `espressif/esp32-camera`-repo op GitHub) bevestigen:
  - Witbalans: `set_wb_mode(sensor, mode)`, `mode` 0 (auto) t/m 4 (presets Sunny/Cloudy/Office/Home — zelfde presets en volgorde als de officiële CameraWebServer-voorbeeldpagina van Espressif).
  - Belichting: `set_exposure_ctrl(sensor, enable)` (AEC aan/uit) + `set_aec_value(sensor, 0-1200)` (vaste belichtingswaarde, alleen relevant als AEC uit staat).
  - Gain: `set_gain_ctrl(sensor, enable)` (AGC aan/uit) + `set_agc_gain(sensor, 0-30)` (vaste gain, alleen relevant als AGC uit staat).
  - Bevestigd in de driverbroncode (`ov2640.c`): geen van deze setters controleert een andere instelling (bv. `set_wb_mode` kijkt niet naar `awb_gain`, `set_aec_value` kijkt niet naar `aec`) — het zijn drie onafhankelijke aanroepen, dus geen vergrendeling nodig tussen de instellingen onderling. Ook bevestigd dat de officiële `CameraWebServer`-voorbeeld-firmware (`app_httpd.cpp`) deze exacte functies zo aanroept vanuit zijn `/control`-endpoint.
  - `Camera::applySettings()` roept `set_whitebal(sensor, 1)` altijd aan (automatische witbalans-DSP aan, ook bij een vaste stand — alleen `wb_mode` bepaalt dan het preset); dit is een eigen keuze (niet uit de officiële bron overgenomen), gebaseerd op hoe de officiële voorbeeldpagina de presets standaard gebruikt (AWB + AWB-gain aan, `wb_mode` als preset-knop).
  - Opstelpagina: dropdowns voor witbalans (Auto/Sunny/Cloudy/Office/Home) en auto/vast voor belichting en gain (met een getalveld bij "vast"), eigen "Use for schedule"-knop. Elke testfoto toont de gebruikte instellingen via responseheaders (`X-White-Balance`, `X-Exposure`, `X-Gain`), zelfde patroon als de bestaande `X-Flash-Brightness-Percent`.
- **Interrupt watchdog (zie "Proeven" hieronder, 09:41 na de upload van 0.5.0).** Eigen modules (`sensor.cpp`, `camera.cpp`, `sd_storage.cpp`, `web.cpp`, `wifi_time.cpp`, `ota.cpp`, `main.cpp`) doorzocht op `noInterrupts`/`portENTER_CRITICAL`/ISR's/lange blokkerende secties: niets gevonden. De enige `delay()`'s die er zijn (flitser-warmup 200 ms, sensor-warmup 2 s bij opstarten, nu ook de nieuwe 2,5 s DHT22-retry) draaien buiten een ISR en met interrupts aan; geen van alle is lang genoeg om de interrupt-watchdog (standaard enkele honderden ms) te raken. Meest aannemelijke verklaring: de interrupt-watchdog trad op vlak na een OTA-upload, dus waarschijnlijk tijdens het wegschrijven/wissen van de nieuwe firmware-partitie zelf (SPI-flash-erase/write schakelt de cache en daarmee interrupts op beide cores kort uit; in combinatie met wifi-timing is dit een bekend incidenteel ESP32/Arduino-verschijnsel bij grote OTA-writes, niet iets in onze eigen code). Niet verder uitgezocht binnen de tijdbox van 15 minuten voor dit punt; nog te doen als het vaker voorkomt — zie hieronder.
- `FIRMWARE_VERSION` naar `0.6.0`.

## Proeven

### Proef 1: zuurdesemstarter, 1:2:2 (11–12 november 2026)

- **Opzet:** starter ~19 g, voeding 1:2:2 (dus ~38 g bloem + ~38 g water erbij), pot ~1,5 L, op het aanrecht in de keukenlucht (geen klimaatregeling). Firmware 0.5.0. Van ~10:00 (11 nov) tot ~09:00 (12 nov).
- **Techniek:** 93 van de 93 geplande foto's gemaakt, geen gaten in de fotoreeks; bestandsgrootte 34–37 KB met flits. Geen herstarts tijdens de proef, op één uitzondering na: om 09:41 een interrupt watchdog, kort na het uploaden van 0.5.0 (zie F2d hierboven voor het onderzoek). 1309 metingen, geen dubbelingen. Twee gaten in de metingen zonder bijbehorende herstart (17:20–19:06 en 19:47–20:04) — de foto's liepen in die gaten gewoon door, dus geen SD-kaart- of voedingsprobleem; oorzaak nog onbekend (mogelijk een tijdelijke wifi- of DHT22-hapering zonder dat de firmware dat als "mislukte meting" logde — F2d's lege-waarde-regel moet dit soort gaten voortaan wel vastleggen).
- **Klimaat:** 17,6–19,5 °C (gemiddeld 18,5 °C), 44,5–53,4 % relatieve vochtigheid.
- **Resultaat:** geen zichtbare rijzing op de foto's. Waarschijnlijke oorzaak: te koud (sourdough rijst traag onder 20 °C) in combinatie met de ruime 1:2:2-voeding en een verdunde/nog niet actieve starter.
- **Verbeterpunten voor de volgende proef (opstelling):**
  - Kleinere pot (de 1,5 L-pot maakt een kleine rijzing moeilijk te zien).
  - Camera lager richten.
  - De schaal niet midden in beeld laten staan (zodat de pot zelf meer in beeld komt).
  - De plek van de pot markeren, zodat foto's onderling goed te vergelijken zijn.
  - Zichtbaar kleurverschil tussen dag- en nachtfoto's (keukenlicht vs. alleen de flitser) — mogelijk relevant voor een latere "witbalans vast zetten"-stap (zie F2d hierboven).
  - Glimstrepen/reflecties van de flitser op de pot — hinderlijk bij het beoordelen van foto's, nog geen concrete fix.

## Nog te doen

- Op hardware testen: eerste USB-upload, wifi-verbinding, NTP-sync, DHT22-meting, watchdog, OTA-upload naar 0.1.1 (stappen staan in de PR-beschrijving/eindmelding van F0).
- ~~Op hardware testen: F1-spike (SD + camera + DHT22 tegelijk, OTA naar 0.2.0).~~ Gelukt op 9–10 november, zie hierboven. Nog door te geven: bytegrootte van één foto en capture/save-tijd in ms uit de serial-log.
- ~~Op hardware testen: F2 (fotoschema, bewaarbeleid, bootlog, OTA naar 0.3.0) — één nacht doordraaien.~~ ~~Op hardware testen: F2b (opstelpagina, OTA naar 0.4.0).~~ ~~Op hardware testen: F2c (gedimde flitser, OTA naar 0.5.0).~~ Alle drie gelukt tijdens proef 1 (zie "Proeven" hierboven): volledige nacht gedraaid op 0.5.0, 93/93 foto's met flits, 1309 metingen.
- Op hardware testen: F2d (OTA naar 0.6.0) — DHT22 loskoppelen tijdens een meting (retry + lege-waarden-regel `epoch,,` moet verschijnen in de dag-CSV), een uur laten draaien en `/data/status.csv` controleren (één regel per uur, tellers kloppen met de serial-log), elke witbalans-/belichtings-/gain-combinatie als testfoto proberen (headers tonen de juiste waarden, "Use for schedule" + herstart moet blijven staan), een nacht draaien en kijken of de interrupt watchdog van proef 1 zich herhaalt.
- **Interrupt watchdog (proef 1, 09:41, kort na de OTA-upload van 0.5.0):** geen oorzaak in onze eigen code gevonden binnen de tijdbox van F2d (zie F2d hierboven). Als dit bij een volgende OTA-upload terugkomt: eerst checken of het weer net na een upload is (wijst op de flash-erase/write zelf, niet op de lopende firmware) vóór er verder in de eigen modules gezocht wordt.
- **Als de SD-kaart uitvalt, gaan metingen en foto's verloren** (geen terugval naar LittleFS). Bewust, voorlopig: LittleFS heeft met OTA maar ~1,4 MiB ruimte (zie projectdocument §7, punt 2), genoeg voor metingen maar niet voor foto's; een aparte terugvalopslag is meer ontwerpwerk dan deze fase toelaat.
- F3: lokale webpagina (basis uit de thuismeter, `web.cpp`).
- Later: cloud-koppeling (Supabase, device-token), doelbereik/alerts, verwarmen.
