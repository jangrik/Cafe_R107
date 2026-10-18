# Cafe R107 · Version 1.0.4

RFID-Kaffeeerfassung für ein kleines Büro: ESP32, RC522, Waveshare ST7789
240 × 240 und passiver elektromagnetischer Summer. PlatformIO/Arduino,
Weboberfläche und Daten in LittleFS. Kein Eingriff in die Espressomaschine.

## Neu in 1.0.4

- LCD-Hintergrundbeleuchtung BL liegt auf **GPIO 21**, statt GPIO 14.
- TOF bleibt an SDA 16 / SCL 17. Nur Firmware-Upload nötig.

## Neu in 1.0.3: Hintergrundbeleuchtung und Näherung

**Vor dem Flashen:** LCD **BL von 3V trennen und an GPIO 21 anschließen**.
Der BL-Anschluss des Waveshare-Moduls ist ein Steuereingang; keine zusätzliche
Transistorstufe erforderlich. VCC des LCD bleibt an 3V. Niemals GPIO 21 parallel
zur bisherigen festen 3V-Verbindung anschließen.

| VL53L0X-Modul | ESP32 |
|---|---|
| VIN/VCC (3,3-V-kompatibles Breakout-Modul) | 3V |
| GND | G |
| SDA | 16 |
| SCL | 17 |
| XSHUT / GPIO1 | unbeschaltet |

Nur ein VL53L0X wird verwendet; I²C-Adresse 0x29. Die Pinwahl lässt SPI und
RFID unverändert. Das Modul muss für 3,3 V Versorgung/Logik geeignet sein;
der nackte Sensorchip ist kein Ersatz für ein Breakout mit passender Versorgung.
Den Sensor nach vorn ausrichten, ohne Abdeckung direkt über der Optik.

- Nach dem Start und jedem neuen Chipkontakt bleibt das Licht **10 Minuten**
  an. Das gilt auch für unbekannte Chips und Kontakte innerhalb der Buchungssperre.
- Eine erkannte Annäherung bis **80 cm** weckt es für **30 Sekunden**.
  Sie zählt nicht als Chipkontakt und erzeugt keine Buchung. Restzeit nach
  einem Chipkontakt wird durch die kurze Näherungsfrist nicht verkürzt.
- Drei aufeinanderfolgende Nahmessungen lösen aus (ca. 0,3 s). Erst nach fünf
  Fern-/ungültigen Messungen, bei gültiger Ferne ab **110 cm**, wird neu scharf
  geschaltet. Dauerhafte Anwesenheit lässt das Licht nicht endlos an.
- Beim nächsten Chipkontakt leuchtet es sofort wieder zehn Minuten.
- Ohne angeschlossenen oder beim Start erkannten TOF-Sensor bleibt RFID als
  Aufweckquelle aktiv. Sensorfehler/fehlende Messungen erzeugen keine Buchungen.
- ESP32, RFID, WLAN und Webserver bleiben bei ausgeschaltetem Licht aktiv.
  Es wird nur BL abgeschaltet; unsichtbare Ranglisten werden nicht berechnet.
- Grenzwerte stehen in `include/Backlight.h`. Status/Entfernung sind zusätzlich
  über `/api/status` sichtbar (0 mm bedeutet keine gültige Entfernung).
- Die Pololu-VL53L0X-Bibliothek 1.3.1 wird von PlatformIO installiert.

**Update:** `git pull` und `pio run -t upload`. Kein LittleFS-Upload erforderlich.
Erste Prüfung: BL-Umverdrahtung, Annäherung, Buchung, zehn Minuten warten,
Wiederaufwecken durch Entfernen/erneutes Annähern und durch einen Chip.
Der Hardwaretest mit dem konkreten Sensormodul steht noch aus.

## Neu in 1.0.2

- Die vergrößerte Tasse der normalen Displayansicht zeigt **HH:MM** an,
  sobald die Uhrzeit bekannt ist. Ohne gültige Uhrzeit bleibt die Tasse innen leer.
- Drei Dampffäden bewegen sich im Sekundentakt. Nur die kleinen betroffenen
  Bildbereiche werden neu gezeichnet, nicht jede Sekunde das gesamte Display.
- Alle **60 Sekunden** erscheint für **5 Sekunden** eine Top-5-Liste.
  Reihenfolge: **heute → dieser Monat → insgesamt → heute …**. Dazwischen
  bleibt die normale Ansicht rund 55 Sekunden sichtbar. Erster Start nach
  einer Minute; Start- und Buchungsbestätigungen haben Vorrang.
- Bei einem Chipkontakt während der Rangliste erscheint sofort die normale
  Buchungsbestätigung. RFID-Erfassung und Webserver laufen weiter.
- Stornierte Bezüge werden nicht mitgezählt. Bei Gleichstand wird nach Name,
  danach nach Person-ID sortiert. Es erscheinen nur Personen mit mindestens
  einem Bezug im jeweiligen Zeitraum, maximal fünf.
- Ohne Uhrzeit zeigen Tages-/Monatsrangliste einen Hinweis; die Gesamtliste
  berücksichtigt auch undatierte Bezüge. Leere Listen werden erklärt.
- Die größeren Hinweis- und Bestätigungsschriften aus 1.0.1 bleiben erhalten.
- Version wird einheitlich aus `include/Domain.h` bezogen (auch im Web).

**Update von 1.0.1:** `git pull`, anschließend nur Firmware bauen und hochladen
(`pio run -t upload`). Kein LittleFS-Upload erforderlich; Webdateien,
Konfiguration, Personen und Buchungen bleiben bestehen.

## Bedienung

- Ein bekannter Chipkontakt speichert **einen Kaffee**. Erst nach Entfernen
  für mindestens 700 ms und mindestens 3 Sekunden Abstand zum letzten
  akzeptierten Chipkontakt kann erneut gebucht werden. Auch ein anderer Chip
  innerhalb dieser 3 Sekunden wird ignoriert; in diesem Fall erneut vorhalten.
- Unbekannte Chips erscheinen unter **Personen & Chips**. Sie buchen nichts.
  Person anlegen, Chip zuordnen, anschließend erneut vorhalten.
- Eine Person kann mehrere Chips besitzen. Die Zuordnung erfolgt lokal auf
  dem ESP32. Der Sketch liest nur UIDs und schreibt nicht auf die Chips.
- Das LCD bestätigt gespeicherte Buchungen. Bekannte Chips: 2500 Hz / 200 ms;
  unbekannte Chips: zwei kurze tiefere Töne. Fehlgeschlagene Speicherungen
  werden nicht als erfolgreiche Buchung angezeigt.
- Die Weboberfläche bietet Gesamtwerte, Tageswerte, auswählbare ISO-
  Kalenderwochen und Monate, jeweils nach Personen, einen Verlauf mit älteren
  Seiten und Storno. CSV enthält auch stornierte Bezüge mit Kennzeichnung.
- Storno legt einen zusätzlichen Journaleintrag an. Der ursprüngliche Bezug
  bleibt erhalten und wird aus den Summen ausgeschlossen. Umbenennen einer
  Person ändert deren angezeigten Namen auch bei alten Buchungen.
- Es wird die Chip-Buchung gezählt, **nicht der tatsächliche Maschinenbezug**.
  Shelly/Leistungsmessung und Geldkonten sind nicht Bestandteil dieser Version.

## Hardware / funktionierende Pinbelegung

Versorgung über USB. RC522 ausschließlich an 3,3 V anschließen.
Das Projekt verwendet das generische 4-MB-ESP32-Profil `esp32dev` und explizite
GPIO-Nummern; keine Annahme über einen herausgeführten GPIO 21 oder 5.

| Modul | Anschluss | ESP32 |
|---|---|---|
| RC522 | 3.3V / GND | 3V / G |
| RC522 | SDA (SPI-CS) / RST | 25 / 22 |
| RC522 | SCK / MOSI / MISO | 18 / 23 / 19 |
| ST7789 LCD | VCC / GND / BL | 3V / G / GPIO 21 |
| ST7789 LCD | CLK / DIN | 18 / 23, gemeinsam mit RFID |
| ST7789 LCD | CS / DC / RST | 26 / 27 / 33 |
| Summer-Treiber | Basisansteuerung | GPIO 32 über 270 Ω |
| VL53L0X | VIN / GND / SDA / SCL | 3V / G / 16 / 17 |

Summer: **PN2222A**, Emitter an GND, Kollektor an Summer −. Summer + über
**10 Ω, mindestens ¼ W**, an 3V. Basis zusätzlich über 10 kΩ an GND.
Freilaufdiode direkt parallel zum Summer, Kathodenring an Summer +.
Transistor-Pinbelegung anhand des konkreten Herstellers prüfen.
Dies ist der mit dem vorhandenen 16-Ω-Summer erprobte Aufbau für kurze Töne;
seine Dauerbetriebsdaten sind unbekannt. Der Summer darf nie direkt an GPIO.
Ein separater ESP-Timer begrenzt jeden Ton auch bei beschäftigtem Webserver.

## Erstinstallation in VS Code / PlatformIO

1. Repository klonen und den Ordner mit `platformio.ini` in PlatformIO öffnen:

   ```sh
   git clone https://github.com/jangrik/Cafe_R107.git
   cd Cafe_R107
   ```

2. **Build**, dann **Upload** der Firmware ausführen (`pio run`,
   `pio run -t upload`). Abhängigkeiten und ESP32-Plattform sind festgelegt;
   ESPFMfGK 2.0.17 liegt im Projekt unter `lib/`.
3. Nur bei der **Erstinstallation** das LittleFS-Abbild aus `data/` hochladen.
   Es enthält die Anwendung unter `/www/` und die Dateimanager-Webdateien
   `/fm.html`, `/fm.css`, `/fm.js`.

   Windows PowerShell:

   ```powershell
   $env:CAFE_ALLOW_FS_ERASE="YES"
   pio run -t uploadfs
   Remove-Item Env:CAFE_ALLOW_FS_ERASE
   ```

   Linux/macOS:

   ```sh
   CAFE_ALLOW_FS_ERASE=YES pio run -t uploadfs
   ```

   Der Schutz in `scripts/guard_fs_upload.py` blockiert versehentliches
   `uploadfs`. Ein reines `buildfs` erstellt nur ein Abbild und löscht nichts.
4. Reset drücken und seriellen Monitor mit **115200 Baud** öffnen.
   Beim ersten Start erzeugt das Gerät getrennte zufällige Kennwörter für
   Access Point und Admin. Sie erscheinen im seriellen Monitor und beim Start
   15 Sekunden auf dem Display. Später bleiben sie über Neustarts erhalten.
5. Mit **Cafe-R107-XXXX** verbinden. Web: **http://192.168.4.1/**,
   Login **admin** mit dem angezeigten Admin-Kennwort.
6. **Gerät & WLAN** öffnen, Browserzeit übernehmen, optional Client-WLAN
   eintragen und speichern. Nach Konfigurationsänderungen Reset drücken.
   Die neuen WLAN-/Zugangseinstellungen gelten nach dem Neustart. Ein leeres
   Passwortfeld behält den bisherigen Wert; das gesonderte Kontrollkästchen
   löscht das Client-WLAN-Passwort für offene WLANs.
7. Namen anlegen, Chips vorhalten und zuordnen. Der erste bekannte Bürochip
   ist bewusst nicht im öffentlichen Quellcode voreingetragen.

## Netzwerk und Uhrzeit

- AP und Client laufen gleichzeitig (`WIFI_AP_STA`). Der AP bleibt auch bei
  fehlendem oder falschem Client-WLAN verfügbar. Das Gerät wartet beim Start
  nicht auf eine Verbindung. Der Dateimanager startet ebenfalls im AP-Betrieb.
- Bei Client-Verbindung wird die vom Router zugewiesene Adresse auf dem LCD
  und im Web angezeigt. Dateimanager: dieselbe Adresse auf **Port 8080**.
- Beim WLAN-Kanalwechsel durch den verbundenen Client kann die AP-Verbindung
  kurz unterbrochen werden; danach wieder verbinden.
- NTP startet erst bei bestehender Client-Verbindung. Zeitzone Berlin mit
  Sommerzeit, ISO-Wochen beginnen montags. Zulässiger Zeitbereich 2024–2037
  passend zum gepinnten Arduino-Core 2.0.17 mit 32-Bit-Zeitbasis.
- Ohne Uhrzeit werden Bezüge mit zufälliger Startkennung und 64-Bit-Laufzeit
  gespeichert. Der erste gültige Zeitanker derselben Sitzung (NTP/Browser)
  erlaubt die nachträgliche Datierung. Es erfolgt kein erfundenes Datum.
- Wird vor der ersten Zeitsynchronisation ausgeschaltet, bleiben diese Bezüge
  ohne Datum. Sie zählen zur Gesamtsumme, nicht zu Wochen/Monaten. Ein Zeitanker
  eines späteren Starts darf sie nicht umdatieren. Das Web weist darauf hin.
- Es gibt keine batteriegepufferte Echtzeituhr. AP-Nutzung ohne Internet:
  nach jedem Neustart kurz die Browserzeit übernehmen.

## LittleFS, Sicherung und Updates

4 MB Flash: zwei App-Partitionen zu je 1,5 MiB, LittleFS **960 KiB**. OTA wird
in dieser Version nicht angeboten; die App-Partitionen lassen eine spätere
Erweiterung zu. **Firmware-Upload** lässt bei unveränderter Partitionstabelle
LittleFS bestehen; **Filesystem-Upload ersetzt das komplette Dateisystem**.

| Datei auf dem ESP | Inhalt |
|---|---|
| `/config.json` | Client-WLAN, AP- und Admin-Kennwort |
| `/registry.json` | Personen, Chipzuordnungen, neue Chips |
| `/journal.ndjson` | Anhängendes Journal: Kaffee, Storno, Zeitanker |
| `/www/*` | Weboberfläche ohne externe CDN-Abhängigkeiten |
| `/fm.*` | ESPFMfGK-Oberfläche |

Konfiguration und Zuordnungen werden über temporäre Dateien und Rename
ersetzt. Das Journal wird je Buchung angehängt und geschlossen. Bei
beschädigtem Journal werden neue Buchungen blockiert statt Daten zu löschen.
**Kein automatisches Formatieren**, auch bei Mountfehlern nicht.

Vor Updates und regelmäßig im Dateimanager **alle Dateien als ZIP sichern**.
Das Backup enthält Kennwörter und personenbezogene Daten und gehört nicht in
GitHub. CSV ist ein zusätzlicher lesbarer Export, kein vollständiges Backup.
Der verbleibende Speicher wird im Web angezeigt; bei weniger als 16 KiB frei
werden weitere Buchungen abgewiesen. Eine automatische Datenlöschung gibt es
nicht. Aufbewahrung hängt von der Anzahl Buchungen und Gerätestarts ab.

Webseiten später einzeln mit dem Dateimanager unter `/www/` ersetzen.
Bei Wiederherstellung zuerst Firmware/ggf. frisches FS aufspielen, dann die
Backup-Dateien über ESPFMfGK zurückkopieren und neu starten. Nach manuellen
Änderungen an `/config.json`, `/registry.json` oder `/journal.ndjson` ist immer
Reset nötig, damit Speicherzustand und Dateien wieder übereinstimmen.
Während manueller Journal-/Registry-Wartung keine Chips vorhalten.

## Zugang

Web und Dateimanager verwenden HTTP-Authentifizierung mit demselben lokalen
Admin-Zugang. Es ist ein Verwaltungsgerät fürs lokale WLAN, ohne TLS und ohne
Internetfreigabe. Jeder mit diesem Zugang kann Buchungen sehen/stornieren und
über den Dateimanager sämtliche Dateien bearbeiten. Das öffentliche Repository
enthält keine echten Chips, Namen, WLAN-Kennwörter oder Buchungsdaten.

## Entwicklung / Tests

```sh
pio run
pio run -t buildfs
g++ -std=c++11 -Wall -Wextra -Werror -Iinclude test/native/test_domain.cpp -o /tmp/test-domain
/tmp/test-domain
npm test
npm run check
```

GitHub Actions baut Firmware und LittleFS und prüft Kalendergrenzen, ISO-
Wochenjahr, Sommerzeit, Offline-Zeitanker und Web-Hilfsfunktionen. Binärdateien
werden als Workflow-Artefakt angeboten. Hardware-Abnahme nach dem Flashen:

- Start ohne bekanntes WLAN; Web **und** Dateimanager per AP erreichbar.
- Browserzeit setzen, Person anlegen, neuen Chip zuordnen.
- Einmal vorhalten zählt genau eins; Liegenlassen erzeugt keine Wiederholung.
- Nach Entfernen und 3 Sekunden erneut buchen, anschließend Reset: Werte bleiben.
- Storno, Umbenennen, historische Wochen-/Monatswahl und CSV prüfen.
- Client-WLAN einrichten; AP und Client-Adresse anschließend beide prüfen.
- Ohne Zeit buchen, in derselben Sitzung Browserzeit setzen: Bezug wird datiert.
- Ohne Zeit buchen, vor Synchronisation neu starten: Bezug bleibt undatiert.
- Backup herunterladen und getrennt aufbewahren.

Tests auf dem Rechner ersetzen keine Abnahme auf deinem ESP32.

## ESPFMfGK 2.0.17

Von [Holger Lembke](https://github.com/holgerlembke/ESPFMfGK), eingefrorener
Upstream-Commit `ac3b699c35705d34df06ed4a978fb5add4310463`.
Lokale Anpassungen und Lizenz: [lib/ESPFMfGK/UPSTREAM.md](lib/ESPFMfGK/UPSTREAM.md).
