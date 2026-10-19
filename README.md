# Cafe R107 · Version 1.0.9

RFID-Kaffeeerfassung für ein kleines Büro: ESP32, RC522, Waveshare ST7789
240 × 240 und passiver elektromagnetischer Summer. PlatformIO/Arduino,
Weboberfläche und Daten in LittleFS. Kein Eingriff in die Espressomaschine.

## Neu in 1.0.9: Chip-Zeitpunkte und Sensorüberwachung , Display gedreht

### Update ohne vollständigen LittleFS-Upload

1. `git pull` und anschließend nur die Firmware hochladen: `pio run -t upload`.
2. Im Dateimanager auf Port 8080 das Verzeichnis **`/www/`** öffnen.
3. Genau diese drei Dateien aus dem lokalen Projekt einzeln ersetzen:

   | Lokale Datei | Ziel im ESP-LittleFS |
   |---|---|
   | `data/www/domain.js` | `/www/domain.js` |
   | `data/www/style.css` | `/www/style.css` |
   | `data/www/app.js` | `/www/app.js` |

   `domain.js` zuerst hochladen, anschließend die beiden anderen Dateien.
   Wenn der Dateimanager im flachen Modus arbeitet, beim Zielnamen den vollen
   `/www/...`-Pfad verwenden. Kein zusätzliches `www`-Unterverzeichnis erzeugen.
4. Danach die normale Website mit **Strg+F5** neu laden.

**Kein `uploadfs`!** `config.json`, `registry.json` und `journal.ndjson` nicht
ersetzen oder löschen. `index.html` und die Dateimanagerdateien bleiben ebenfalls
unverändert. Eine Sicherung vor dem Update ist trotzdem sinnvoll.

### Datum/Zeit neben Chip-ID

Unter „Personen & Chips“ steht direkt bei der UID ausschließlich Datum und
Uhrzeit des letzten neu erkannten Chipkontakts, einschließlich Sekunden.
Die Beschriftung „Zuletzt vorgehalten:“ wird nicht verwendet. Das gilt für
unbekannte und zugeordnete Chips, auch ohne Buchung oder innerhalb der Sperrzeit.
Liegenlassen aktualisiert den Zeitpunkt nicht ständig.

Die Metadaten werden pro neuem Kontakt atomar in **`/lastseen.json`** gesichert,
getrennt von Konfiguration, Zuordnungen und Journal. Die Datei wird vom Gerät
selbst angelegt, gehört ins Backup und darf beim Update nicht aus `data/`
überschrieben werden. Es werden maximal 130 UIDs vorgehalten; alte unzugeordnete
UIDs werden bei Bedarf zuerst verdrängt. Kein Flash-Schreiben im 100-ms-Poll.

Bei bestehenden Chips ist der Zeitpunkt vor dem ersten Kontakt unter 1.0.8
nicht rekonstruierbar: zunächst „—“. Bei Kontakt ohne Uhrzeit erscheint
„Uhrzeit unbekannt“. Der Start-/Laufzeitbezug erlaubt später dieselbe Datierung
wie bei Buchungen, sofern ein Zeitanker dieser Sitzung gespeichert wird.
Im Web wird Europe/Berlin mit Sommerzeit verwendet.
Fehler dieser zusätzlichen Datei erscheinen als Warnung und blockieren keine
ansonsten speicherbaren Kaffeebuchungen. Eine beschädigte `lastseen.json` wird
nicht automatisch überschrieben; vor Reparatur über den Dateimanager sichern.
Die Website aktualisiert die Zeitpunkte beim nächsten Poll (etwa acht Sekunden),
ohne gerade bearbeitete Namen/Auswahlfelder zu überschreiben.

### TOF und RFID: Prüfung jede Minute

Ein Supervisor läuft alle **60 Sekunden**, auch nach fehlgeschlagener
Initialisierung beim Start. Keine unbedingten periodischen Neustarts:

- **TOF:** I²C-ACK an 0x29 prüfen. Mehr als fünf Sekunden ohne abgeschlossene
  Messung gelten als Stillstand. Der nächste Minutencheck initialisiert dann
  den dedizierten I²C-Bus und den VL53L0X inklusive Dauermessung neu.
- Ungültige Entfernungen bei weiterhin abgeschlossenen Messungen (z. B. kein
  geeignetes Ziel) lösen **keine** Wiederherstellung aus. Kommunikationsfehler
  dürfen das Alter der letzten abgeschlossenen Messung nicht zurücksetzen.
- Bei erfolglosem Versuch erfolgt der nächste erst beim folgenden Minutencheck.
  Eine erfolgreiche Neuinitialisierung gilt erst nach einer neuen abgeschlossenen
  Messung wieder als gesund. Eine alte Distanz wird nicht als neue ausgegeben.
- **RFID:** VersionReg dreimal lesen und mit der beim Start erkannten Version
  vergleichen. Ein einzelner Ausreißer wird toleriert. Bei wiederholten Fehlern
  RST an GPIO 22 pulsen und PCD_Init ausführen, danach erneut prüfen.
- Keine Karte im Feld beziehungsweise ein normaler Karten-Timeout ist kein
  RFID-Fehler. Die vorhandene Kartensperre bleibt bei Wiederherstellung erhalten,
  damit ein liegen gelassener Chip keine zusätzliche Buchung erzeugt.
- Es gibt **keinen automatischen ESP-Neustart**. Uhrzeit, Netzwerk und
  gespeicherte Daten bleiben bei diesen Wiederherstellungen erhalten.

Ohne XSHUT-/Versorgungsschaltung kann ein hart blockierter oder elektrisch
fehlerhafter TOF unter Umständen nicht per Software wiederbelebt werden.
Ein RFID-Registertest beweist die SPI-Erreichbarkeit, nicht die vollständige
Funktion der Antenne. Fehler provozieren und am echten Gerät prüfen.

### Diagnose-API

**GET `/api/diagnostics`** mit dem bisherigen Admin-Login liefert `tof` und
`rfid` mit:

- `healthy`, `error`: aktueller Diagnosezustand bzw. erkannte Ursache;
- `lastError`: zuletzt beim Supervisor festgestellte Fehlerursache;
- `checks`, `recoveryAttempts`, `successfulReinitializations`;
- `lastRecoveryReason`, `lastCheckAgeMs`, `lastRecoveryAgeMs`;
- RFID zusätzlich `versionRegister`;
- `automaticEspRestart:false`, `checkIntervalMs:60000`, `tofStallMs:5000`.

Vor der ersten Prüfung/Wiederherstellung sind die zugehörigen Alterswerte null.
Initialisierungserfolge zählen erfolgreiche Initialisierungsaufrufe; beim TOF
bestätigt erst die folgende abgeschlossene Messung `healthy:true`.
Mögliche Fehler: `i2c_not_responding`, `measurement_stalled`, `not_initialized`,
`i2c_measurement_error`, `version_register_mismatch`, `reinitialization_failed`.
`waiting_for_measurement` ist ein vorübergehender Zustand nach Initialisierung.

Die TOF-Diagnose steht zusätzlich unter `diagnostics` in `/api/tof` und beide
Diagnosen unter `tofDiagnostics`/`rfidDiagnostics` in `/api/status`.
Die Geräteseite zeigt kurze Sensorzustände mit Wiederherstellungszählern.
`available` in `/api/tof` bedeutet weiterhin erfolgreiche Initialisierung;
für den tatsächlichen Betrieb zusätzlich `valid`, `ageMs` und die Diagnose prüfen.
Die Zähler gelten seit dem letzten ESP-Start und werden nicht ins Flash geloggt.
`/api/registry` bleibt kompatibel und ergänzt `lastSeen` als UID→Epoch-Abbildung;
0 steht für einen erfassten Kontakt ohne zuordenbare Uhrzeit.

## Neu in 1.0.7: Gerätebefehle direkt im Browser

Die vier Befehle **Neustart, Display On, Off und Auto** sind zusätzlich per
GET verfügbar. Im Browser anmelden und die passende Adresse öffnen:

- `http://192.168.123.60/api/restart`
- `http://192.168.123.60/api/display/on`
- `http://192.168.123.60/api/display/off`
- `http://192.168.123.60/api/display/auto`

Über den Access Point dieselben Pfade an `http://192.168.4.1` verwenden.
GET benötigt nur den vorhandenen Admin-Login, keinen zusätzlichen Header.
POST funktioniert unverändert mit `X-Cafe-Request: 1`. Andere schreibende APIs
bleiben POST-Endpunkte. Display-Off bleibt auslösbar durch neue RFID-/TOF-Aktivität.
Nur Firmware aktualisieren: `git pull` und `pio run -t upload`; kein FS-Upload.

## Neu in 1.0.6: Display-Off mit weiter aktiver Aufweckfunktion

`POST /api/display/off` beendet sofort beide Beleuchtungsfristen und wechselt
in `auto`, genau wie nach Ablauf der regulären Inaktivitätszeit. Ein neuer
RFID-Kontakt schaltet das Licht wieder für zehn Minuten ein; eine erneute
TOF-Näherung weckt es für 30 Sekunden mit der bisherigen Filterung/Hysterese.
Auch JSON `{"mode":"off"}` an `/api/display` hat dieses Verhalten.
Die Antwort lautet `{"ok":true,"on":false,"mode":"auto"}`.
`on` bleibt dauerhaftes Einschalten, `auto` hebt diesen erzwungenen Zustand auf.
Nur Firmware aktualisieren, kein LittleFS-Upload erforderlich.

## Neu in 1.0.5: Geräte-API

Alle Endpunkte verwenden den vorhandenen HTTP-Basic-Login **admin** mit dem
Admin-Kennwort. POST-Aufrufe erfordern den Header
`X-Cafe-Request: 1`. Seit 1.0.7 sind Neustart und Display On/Off/Auto zusätzlich
als GET für die Browser-Adresszeile verfügbar, ohne diesen Header.
Zugriff über AP- und Client-Adresse.

| Methode | Pfad | Funktion |
|---|---|---|
| GET / POST | `/api/restart` | Neustart anfordern; HTTP 202, danach etwa 500 ms Verzögerung |
| GET | `/api/display` | Aktuellen Lichtzustand und Modus lesen |
| GET / POST | `/api/display/on` | Beleuchtung dauerhaft einschalten |
| GET / POST | `/api/display/off` | Sofort aus wie bei abgelaufener Zeit; RFID/TOF bleiben aktiv |
| GET / POST | `/api/display/auto` | Bisherige Zeit-/TOF-Automatik wieder aktivieren |
| POST | `/api/display` | Alternativ JSON `{"mode":"on"}`, `off` oder `auto` |
| GET | `/api/tof` | Letzte TOF-Messung mit Einheit, Gültigkeit und Alter |

**Display** meint die Hintergrundbeleuchtung an GPIO 21. `on` übersteuert
die Automatik bis zum nächsten Moduswechsel oder Neustart. `off` löscht beide
laufenden Aktivitätsfristen und aktiviert `auto`: RFID und TOF können das Licht
anschließend wieder einschalten. Chipkontakte, Buchung, Summer und Webserver
arbeiten unverändert weiter. `auto` alleine setzt keine Fristen neu.
Der Modus wird nicht in LittleFS gespeichert; nach Reset gilt `auto`.

Beispiele für Windows/PowerShell (auf Linux/macOS `curl` statt `curl.exe`):
Das Kennwort wird interaktiv abgefragt, nicht in die Kommandozeile geschrieben.
Die Adresse gegebenenfalls durch die Client-IP ersetzen.

```sh
curl.exe -u admin http://192.168.4.1/api/tof
curl.exe -u admin http://192.168.4.1/api/display
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" http://192.168.4.1/api/display/off
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" http://192.168.4.1/api/display/on
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" http://192.168.4.1/api/display/auto
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" http://192.168.4.1/api/restart
```

Beispielantwort Display nach Ausschalten: `{"on":false,"mode":"auto"}`.
Eine erfolgreiche POST-Umschaltung liefert zusätzlich `"ok":true`.
Ungültige Modi liefern HTTP 400 und ändern den bisherigen Modus nicht.

Beispielantwort TOF:

```json
{"available":true,"valid":true,"distanceMm":432,"ageMs":34,"presence":true}
```

- `available`: Sensor wurde beim Start erfolgreich initialisiert.
- `valid`: letzte Messung gültig und höchstens 500 ms alt.
- `distanceMm`: Entfernung in **Millimetern**, bei ungültiger/veralteter Messung
  oder fehlendem Sensor **null** (kein erfundener Null-Abstand).
- `ageMs`: Alter der letzten abgeschlossenen Messung in Millisekunden,
  **null**, wenn noch keine Messung vorliegt. Das gilt auch für ungültige Messungen.
- `presence`: gefilterter Anwesenheitszustand mit der bisherigen Hysterese.

Die Abfrage verwendet die laufenden, etwa alle 100 ms aktualisierten Messungen
und wartet nicht auf eine neue Messung. Ein fehlender Sensor liefert weiterhin
HTTP 200 mit `available:false`, `valid:false` und `distanceMm:null`.
Neustartantwort: `{"ok":true,"restarting":true,"delayMs":500}` (HTTP 202).
Die Antwort wird zuerst gesendet; der Neustart erfolgt anschließend in der
Hauptschleife nach abgeschlossenen synchronen Dateizugriffen. Weitere
Neustartanforderungen schieben einen bereits geplanten Neustart nicht hinaus.

**Update:** Nur `git pull` und `pio run -t upload`; kein LittleFS-Upload nötig.
Build/Tests prüfen zusätzlich manuelle Modi, ungültige Modi, Messwert-Frische
und die verzögerte Neustartlogik einschließlich `millis()`-Überlauf. API und
Neustart müssen nach dem Flashen noch am echten Gerät geprüft werden.

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
| `/lastseen.json` | Letzte Chipkontakte (ab 1.0.8, vom Gerät angelegt) |
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
