# Cafe R107 · Version 1.0.14

RFID-Kaffeeerfassung für ein kleines Büro: ESP32, RC522, Waveshare ST7789
240 × 240 und passiver elektromagnetischer Summer. PlatformIO/Arduino,
Weboberfläche und Daten in LittleFS. Kein Eingriff in die Espressomaschine.


**Direkt zu:** [OTA-Update](#firmware-per-ota-aktualisieren) ·
[HTTP-API](#vollständige-http-api-stand-1014) ·
[Pinbelegung](#hardware--funktionierende-pinbelegung) ·
[Sicherung und Updates](#littlefs-sicherung-und-updates)

## Neu in 1.0.14: AP-only sofort und dauerhaft umschalten

Die Checkbox unter **Gerät & WLAN** speichert ihren Modus jetzt bereits beim
An-/Abwählen. Kein zusätzlicher Speichern-Klick und kein ESP-Neustart erforderlich.
Nur apOnly wird dabei gespeichert; noch nicht gespeicherte SSIDs/Kennwörter im
Formular werden nicht mitgesendet. Bei Speicherfehlern wird die Checkbox
zurückgestellt und eine Fehlermeldung angezeigt.

Nach erfolgreichem Speichern folgt die Umschaltung in der Hauptschleife,
frühestens 250 ms nach der Anforderung, damit zuerst die HTTP-Antwort gesendet
werden kann. Aktivieren beendet STA-Verbindung und Scan, stoppt NTP und
wechselt nach WIFI_AP. Die laufende Uhrzeit und der ESP bleiben erhalten.
Abwählen aktiviert AP+STA wieder; neue Suchläufe warten wie bisher, solange
noch ein Gerät am ESP-AP angemeldet ist. Die WLAN-Zugänge bleiben gespeichert.
**apOnly wird in config.json gespeichert und gilt auch nach Neustart.**

Wurde die Seite über die bisherige Client-IP geöffnet, ist diese nach dem
Abschalten nicht mehr erreichbar: mit Cafe-R107-… verbinden und
**http://192.168.4.1/** öffnen. Bei unbekanntem Ausgang einer unterbrochenen
Anfrage die Seite neu laden und den gespeicherten Modus prüfen.

Die Oberfläche zeigt nun sowohl den aktiven als auch den gespeicherten Modus.
Andere Änderungen am Formular (WLAN-Zugänge/AP-/Admin-Kennwort) benötigen
weiterhin einen Neustart. Auch beim normalen Speichern des Formulars wird
apOnly sofort übernommen; die Antwort erklärt, ob andere Änderungen noch
einen Neustart erfordern.

**Update von 1.0.13:** git pull, Firmware hochladen, dann über den Dateimanager
**data/www/app.js → /www/app.js** und
**data/www/index.html → /www/index.html** einzeln ersetzen. Strg+F5.
Kein uploadfs und kein Ersetzen der Datendateien. OTA-Anmeldung bleibt wie in 1.0.13.

API: POST /api/config mit **{"apOnly":true}** beziehungsweise false speichert
und plant die sofortige Umschaltung. Die Antwort ergänzt **modeChangeScheduled**
und **configuredApOnly**; **restartRequired** ist nur noch true, wenn andere
geänderte Einstellungen einen Neustart benötigen. GET /api/status ergänzt
**networkModeChangePending**. Bei einem seltenen Fehler der Radio-Umschaltung
bleibt apOnly der tatsächlich laufende Modus, configuredApOnly zeigt den
persistierten Zielmodus; die serielle Meldung mode_change enthält result=failed.
Ein erneuter Klick/Speichervorgang oder Neustart kann die Übernahme erneut versuchen.

## Neu in 1.0.13: AP-only, drei WLANs und serielle Diagnose

Unter **Gerät & WLAN** gibt es jetzt **„Nur Access Point – keine WLAN-Client-Verbindung“**
und drei WLAN-Plätze mit jeweils SSID und Kennwort. AP-only wird in config.json
persistiert: Nach Neustart läuft der ESP in WIFI_AP, ohne Client-Verbindungsversuche,
Scans oder NTP-Abfragen. Der Laptop verbindet sich als Client mit dem ESP-AP.
Browserzeit, Webserver, RFID, TOF, Dateimanager und OTA bleiben verfügbar.
Die gespeicherten WLAN-Zugänge bleiben beim Umschalten erhalten.

### OTA-Kennwort ab 1.0.13

OTA verwendet ab dieser Version das **gespeicherte Admin-Kennwort** des ESP.
Es steht nicht mehr fest im Quellcode. Das Kennwort in der lokalen Umgebungsvariable
**CAFE_OTA_PASSWORD** setzen; scripts/ota_auth.py übergibt es beim OTA-Upload.
Es wird nicht in platformio.ini gespeichert. Ein OTA-Upload ohne Variable wird
mit einer verständlichen Fehlermeldung abgebrochen; USB-Build und CI benötigen sie nicht.

Windows PowerShell:

```powershell
$env:CAFE_OTA_PASSWORD = Read-Host "Admin-Kennwort des ESP"
pio run -e cafe_r107_ota -t upload
Remove-Item Env:CAFE_OTA_PASSWORD
```

Linux/macOS (Bash):

```bash
read -rsp 'Admin-Kennwort des ESP: ' CAFE_OTA_PASSWORD; echo
export CAFE_OTA_PASSWORD
pio run -e cafe_r107_ota -t upload
unset CAFE_OTA_PASSWORD
```

**Beim ersten OTA-Update von 1.0.12 auf 1.0.13** muss die Variable noch das
bisherige OTA-Kennwort enthalten, weil die laufende alte Firmware den Upload
annimmt. Nach dem Update gilt das Geräte-Admin-Kennwort. Beim USB-Update ist
kein OTA-Kennwort nötig. Nach einer Admin-Kennwortänderung gilt das neue
Kennwort auch für OTA, sobald der ESP neu gestartet wurde.

### Update ohne Verlust der Gerätedaten

1. Sicherung über den Dateimanager, dann **git pull**.
2. Firmware per USB hochladen:

   ```sh
   pio run -e cafe_r107 -t upload
   ```

   Alternativ per OTA: **pio run -e cafe_r107_ota -t upload**.
3. Im Dateimanager das ESP-Verzeichnis **/www/** öffnen und die folgenden Dateien
   aus **data/www/** einzeln ersetzen: **domain.js**, **app.js**, **style.css**,
   zuletzt **index.html**. Danach Browser mit **Strg+F5** neu laden.
4. Gewünschte WLAN-Einstellungen speichern und den ESP per Reset oder
   **/api/restart** neu starten. Für die Modus-Checkbox ist ab 1.0.14 kein Neustart mehr nötig.

**Kein uploadfs und kein Ersetzen von config.json, registry.json, lastseen.json
oder journal.ndjson.** Alte config.json-Dateien bleiben lesbar: Die bisherige
SSID und ihr Kennwort werden als Platz 1 übernommen, Plätze 2/3 bleiben leer,
apOnly ist zunächst false. Erst beim nächsten Speichern werden die zusätzlichen
Felder atomar ergänzt. Bestehende Personen, Chips und Buchungen bleiben erhalten.

### Auswahl der WLAN-Zugänge

Ist AP-only aus, arbeitet der ESP als AP+STA. Die drei Plätze sind normale
SSID-/Kennwort-Zugänge (keine Einrichtung von Enterprise-802.1X oder Captive Portals).
Leere SSID = Platz nicht verwenden. Leeres Passwortfeld in der Website =
Kennwort des Platzes beibehalten; für ein offenes WLAN die jeweilige Checkbox
verwenden. Wird die SSID geändert, gegebenenfalls auch das Kennwort ändern.

Die Standardbibliothek WiFiMulti des gepinnten Cores 2.0.17 scannt synchron
und wartet beim Verbindungsaufbau. Deshalb wird hier stattdessen eine
asynchrone Auswahl verwendet, ohne blockierende Warteschleife in loop():

- Stärkstes sichtbares bekanntes WLAN zuerst; bei erfolglosem Aufbau nach
  15 Sekunden STA trennen und beim nächsten Suchlauf einen anderen Platz versuchen.
- Neue Suchläufe höchstens alle 30 Sekunden. Eine bestehende Verbindung wird
  nicht allein wegen eines stärkeren anderen WLANs gewechselt.
- Solange ein Gerät am ESP-AP angemeldet ist, keine neuen Suchläufe und keine
  neue Verbindung nach einem Suchlauf starten. Nach Abmeldung des letzten
  AP-Clients können die Versuche fortgesetzt werden. Ein bereits gestarteter
  Verbindungsversuch kann bis zu seinem Timeout weiterlaufen.
- Ein bereits laufender Scan wird fertig abgeholt; ist inzwischen ein AP-Client
  angemeldet, wird daraus keine neue Verbindung gestartet.
- AP und STA teilen den Funkkanal. Client-Verbindungsaufbau kann deshalb auch
  im asynchronen Ablauf den AP beeinflussen; AP-only schließt diese Ursache aus.
- Im AP-only-Modus gibt es überhaupt keine WLAN-Client-Suche, auch ohne Laptop.

### Serielle Untersuchung der Abbrüche

Monitor mit **115200 Baud** öffnen:

```sh
pio device monitor -b 115200
```

Beim Start steht **[BOOT] reset=…** mit Resetgrund. **[WIFI … ms]** enthält
Zeit seit ESP-Start, AP-Start/Stopp, Anmeldung/Abmeldung der AP-Clients
(MAC und AID), STA-Verbindungsereignisse, Scan-/Verbindungsversuche und Kanal.
STA-Abmeldungen enthalten den Grundcode mit Namen. Für AP-Client-Abmeldungen
liefert Core 2.0.17 keinen Grundcode: **reason=unavailable_in_core_2**.
Es wird kein Grund geraten. Kanal/Clientzahl/Status neben einem Ereignis sind
zum Zeitpunkt der Ausgabe gemessen; der Ereigniszeitstempel stammt aus dem Callback.

Alle 30 Sekunden und bei erkanntem Kanalwechsel erscheint eine Zustandszeile
mit AP-Clientzahl, Kanal, STA-Status, Scan-/Verbindungszustand, freiem Heap,
minimalem freiem Heap, größtem freien Block und verworfenen Diagnoseereignissen.
Callbacks schreiben nur in eine begrenzte Queue; Ausgabe und WLAN-Steuerung
laufen in der Hauptschleife. Neue Diagnosemeldungen enthalten keine Kennwörter.
Die bisherige Startausgabe enthält allerdings AP-/Admin-Kennwörter: vor dem Teilen
entfernen oder nur **[BOOT]**- und **[WIFI]**-Zeilen weitergeben.

Für den Bürotest zunächst AP-only aktivieren und neu starten. Laptop verbinden,
Monitor vom Start bis über einen Abbruch hinaus mitlaufen lassen, dazu unter Windows:

```cmd
ping -t 192.168.4.1
```

Die passende Uhrzeit des Abbruchs und die seriellen Zeilen davor/danach sichern.
Ein Vergleichstest mit deaktiviertem AP-only ist erst danach sinnvoll.
Ob der Laptop wegen fehlenden Internets wechselt, ist aus einem AP-Abmeldeereignis
allein nicht sicher erkennbar; dafür gegebenenfalls zusätzlich das Windows-WLAN-Protokoll nutzen.
Die Diagnose ersetzt noch keinen Dauertest am realen Büro-WLAN.

### API-Ergänzungen ab 1.0.13

**POST /api/config**, HTTP-Basic-Login und **X-Cafe-Request: 1** wie bisher:

```json
{
  "apOnly": true,
  "wifiNetworks": [
    {"ssid": "Buero"},
    {"ssid": "Zuhause", "password": "BEISPIEL-KENNWORT"},
    {"ssid": "Gastnetz", "password": ""}
  ]
}
```

wifiNetworks enthält, wenn übergeben, **genau drei Objekte** mit jeweils einer
SSID (maximal 32 UTF-8-Bytes). password ist optional (maximal 63 Bytes):
fehlend = bisheriges Kennwort dieses Platzes behalten, leer = offenes WLAN.
apOnly muss ein JSON-Boolean sein. Nicht übergebene Einstellungen bleiben erhalten.
Alte API-Aufrufe mit ssid/wifiPassword ändern weiterhin nur Platz 1, sofern kein
wifiNetworks-Array mitgeschickt wird. Die Datei behält schema:1 und spiegelt
Platz 1 zusätzlich in den alten Schlüsseln. Antwort unverändert:
**{"ok":true,"restartRequired":true}**; aktiviert erst nach Neustart.

**GET /api/status** ergänzt:

| Feld | Bedeutung |
|---|---|
| apOnly | Aktuell laufender Modus |
| configuredApOnly | Gespeicherter Modus für den nächsten Neustart |
| wifiNetworks | Drei gespeicherte Plätze, jeweils ssid und hasPassword; keine Kennwörter |
| activeSsid | Aktuell verbundenes WLAN, sonst leer; ssid bleibt kompatibel Platz 1 |
| wifiChannel / apClients | Aktueller Funkkanal / am ESP-AP angemeldete Clients |
| wifiScanRunning / wifiConnecting | Laufender Scan / Verbindungsversuch |
| wifiLogDropped | Nicht in die Ereignisqueue passende Ereignisse seit Start |
| freeHeap / minFreeHeap | Freier und minimaler freier Heap in Bytes |
| resetReason | Resetgrund als Kurztext |

Die Website zeigt gespeicherte WLANs beim ersten Laden des Formulars und überschreibt
keine laufenden Eingaben bei der automatischen Aktualisierung. Nach einem Speichern
zeigt der Status weiter den laufenden Modus, die zusätzlichen Konfigurationsfelder
dagegen die bereits gespeicherten Werte für den nächsten Neustart.

## Neu in 1.0.12: OTA über WLAN

ArduinoOTA ist aktiv: Hostname **CAFE-R107**, Standardport **3232**.
OTA ist ein eigener Upload-Dienst, kein HTTP-Endpunkt und kein Upload über
den Dateimanager.

### Firmware per OTA aktualisieren

Voraussetzung: Auf dem ESP läuft bereits 1.0.12 oder eine andere Firmware mit
ArduinoOTA. Die erstmalige Installation erfolgt über USB. Rechner und ESP
müssen sich über das lokale Netzwerk erreichen können.

1. Änderungen mit **git pull** herunterladen.
2. In **platformio.ini** die Umgebung **cafe_r107_ota** prüfen:
   upload_protocol = espota; upload_port = 192.168.123.60.
   Die Adresse gegebenenfalls durch die aktuelle Client-IP ersetzen.
3. Für 1.0.12 galt das damals fest konfigurierte OTA-Kennwort. Ab 1.0.13
   **CAFE_OTA_PASSWORD** lokal setzen; es muss zum Admin-Kennwort der laufenden
   Firmware passen (siehe Übergangshinweis oben).
4. Firmware über WLAN bauen und hochladen:

~~~sh
pio run -e cafe_r107_ota -t upload
~~~

Alternativ unter **Project Tasks → cafe_r107_ota → General → Upload**.
Nach erfolgreichem Upload startet der ESP automatisch neu. Die Version
anschließend unter **/api/status** kontrollieren.

Die Standardumgebung **cafe_r107** bleibt für USB:

~~~sh
pio run -e cafe_r107 -t upload
~~~

Bei direkter Verbindung zum ESP-AP kann upload_port auf **192.168.4.1**
gesetzt werden. Bei expliziter IP ist keine mDNS-Namensauflösung nötig.
Bei Problemen IP, Netzwerkverbindung und lokale Firewall prüfen; USB bleibt
als Alternative verfügbar.

**Firmware-OTA erhält LittleFS bei unveränderter Partitionstabelle.**
Kein uploadfs für ein gewöhnliches Update verwenden. Webdateien bei Bedarf
einzeln über den Dateimanager auf Port 8080 ersetzen. Nach dem Neustart muss
die Uhrzeit erneut per NTP oder Browser gesetzt werden.
Dieser Abschnitt beschreibt die Einführung von OTA in 1.0.12; die aktuelle Version ist 1.0.13.

## Vollständige HTTP-API (Stand 1.0.14)

### Adresse und Anmeldung

Alle folgenden Pfade gehören zum Webserver auf **Port 80**.
Basisadresse beispielsweise **http://192.168.123.60**, am AP **http://192.168.4.1**.

- Alle Endpunkte benötigen HTTP Basic Auth: Benutzer **admin**, Admin-Kennwort
  aus der Gerätekonfiguration. Im Browser erscheint die Login-Abfrage.
- **Jeder POST** benötigt zusätzlich **X-Cafe-Request: 1**.
  GET benötigt diesen Header nicht.
- POST mit Nutzdaten: JSON-Objekt, maximal **4096 Bytes**.
  Content-Type: application/json verwenden.
- Antworten sind JSON mit Cache-Control: no-store, außer beim CSV-Export.
- Zeitpunkte sind Unix-Sekunden (UTC), Kalenderauswertungen verwenden Berlin
  einschließlich Sommerzeit. Zeitwert 0 bedeutet unbekannt.
  Alterswerte und uptime sind Millisekunden.
- GET-Aliasse für Neustart, Display und RFID-Reinit führen Aktionen aus.
  Diese Aktionspfade nicht regelmäßig pollen.

### Alle Endpunkte

| Methode | Pfad | Parameter / JSON-Body | Funktion / Antwort |
|---|---|---|---|
| GET | /api/status | keine | Geräte-, Netzwerk-, Zeit-, Speicher- und Sensorstatus |
| GET | /api/registry | keine | people, cards, pending, lastSeen |
| GET | /api/report | Query optional: week=YYYY-Www, month=YYYY-MM | Summen insgesamt, heute, gewählte Woche und Monat, pro Person |
| GET | /api/history | Query optional: before=Buchungs-ID | Maximal 50 Bezüge, neueste zuerst; rows und more |
| GET | /api/export.csv | keine | CSV aller Bezüge einschließlich Stornos |
| GET | /api/display | keine | on und mode |
| POST | /api/display | {"mode":"on"}; alternativ off oder auto | Umschalten; ok, on, mode |
| GET / POST | /api/display/on | kein Body nötig | Dauerhaft einschalten; ok, on, mode |
| GET / POST | /api/display/off | kein Body nötig | Sofort aus wie nach Ablauf der Fristen; Modus auto, RFID/TOF können wieder wecken |
| GET / POST | /api/display/auto | kein Body nötig | Automatik aktivieren, Fristen nicht neu starten |
| GET | /api/tof | keine | Entfernung, Gültigkeit, Alter, Anwesenheit, diagnostics |
| GET | /api/diagnostics | keine | tof, rfid und Überwachungsparameter |
| GET / POST | /api/rfid/reinit | kein Body nötig | Nur RC522 zurücksetzen; ok und rfid; HTTP 200 oder 503 |
| GET / POST | /api/restart | kein Body nötig | ESP-Neustart planen; HTTP 202 mit ok, restarting, delayMs |
| POST | /api/person | {"name":"Beispielperson"}; zum Umbenennen zusätzlich "id":"Person-ID" | Anlegen/Umbenennen; ok und id |
| POST | /api/assign | {"uid":"UID","person":"Person-ID"} | Chip zuordnen/umhängen; ok |
| POST | /api/forget | {"uid":"UID"} | Zuordnung und Eintrag als neuer Chip entfernen; ok |
| POST | /api/cancel | {"id":123} | Bezug mit dieser Buchungs-ID stornieren; ok |
| POST | /api/time | {"epoch":1792490400} | Uhrzeit und gegebenenfalls Zeitanker setzen; ok |
| POST | /api/config | apOnly, wifiNetworks (3 Plätze), apPassword, adminPassword; legacy ssid/wifiPassword für Platz 1 | Speichern; ok und restartRequired:true |

Kein API-Befehl zum künstlichen Buchen, Löschen einer Person oder manuellen
TOF-Reset: Buchungen entstehen durch Chipkontakt; die TOF-Wiederherstellung
übernimmt die Sensorüberwachung.

### Status und Diagnose

**/api/status** liefert:

| Bereich | Felder |
|---|---|
| Version / Laufzeit | version, session, uptime, epoch |
| Netzwerk | apSsid, apIP, staConnected, staIP, ssid, filemanagerPort |
| Beleuchtung / Näherung | backlightOn, backlightMode, tofOK, distanceMm, presence |
| Daten / Speicher | readerOK, storageError, journalOK, fsUsed, fsTotal, pendingCount, lastSeenRevision, lastSeenError |
| Diagnose | tofDiagnostics, rfidDiagnostics |

fsUsed und fsTotal sind Bytes. distanceMm im Status ist bei fehlender gültiger
Entfernung **0**; für eindeutige Gültigkeit /api/tof verwenden.

**/api/tof** liefert available, valid, distanceMm, ageMs, presence, diagnostics.
available bedeutet erfolgreiche Initialisierung, auch nach Wiederherstellung.
valid erfordert 20–2000 mm und eine höchstens 500 ms alte gültige Messung.
Andernfalls ist distanceMm **null**. ageMs ist **null**, solange noch keine
abgeschlossene Messung vorliegt. Auch ungültige Entfernungen aktualisieren
das Alter: healthy:true und valid:false können gleichzeitig korrekt sein.

**/api/diagnostics** liefert tof, rfid, lastSeenError,
automaticEspRestart:false, checkIntervalMs:60000, tofStallMs:5000.

| Objekt | Felder |
|---|---|
| Beide Sensoren | healthy, error, lastError, checks, recoveryAttempts, successfulReinitializations, lastRecoveryReason, lastCheckAgeMs, lastRecoveryAgeMs |
| TOF zusätzlich | originalError, initError, initStage, initTimeout, lastI2cStatus, hardwareResets, recoveryPending, xshutPin |
| RFID zusätzlich | versionRegister, antennaEnabled, antennaControlBits, antennaCheckAgeMs, verification, cardLatched, polls, uidReads, consecutiveReadErrors, readFaultPending, lastWakeStatus, lastWakeStatusText, lastPollAgeMs, lastSelectStatus, lastSelectStatusText, lastUid, lastUidAgeMs |

Interpretation siehe auch Versionsabschnitte 1.0.10/1.0.11.
Zähler gelten seit dem letzten ESP-Start. Historische Fehler bleiben nach
Erholung sichtbar. verification:"registers_only" beim RFID bestätigt keine
vollständige HF-Funktion. Das gilt ebenfalls für ok:true nach RFID-Reinit:
Es bestätigt die Registerprüfung; anschließend den Chip erneut testen.
Vor der ersten Prüfung/Wiederherstellung sind die entsprechenden Alterswerte null;
vor dem ersten Poll/Select/UID-Empfang die zugehörigen Status-/UID-Werte und Alter.
lastSelectStatus kann aus einem früheren Poll stammen.

### Personen, Chips und Auswertung

**/api/registry** liefert people (Objekte mit id, name), cards (uid, person),
pending (Array neuer UIDs) und lastSeen (UID als Schlüssel, Unix-Zeit als Wert).
lastSeen-Wert 0 bedeutet Kontakt ohne zuordenbare Uhrzeit; ein fehlender
Schlüssel bedeutet keinen gespeicherten Kontakt.

UIDs exakt aus der Antwort übernehmen. **/api/assign** akzeptiert nur Chips,
die schon zugeordnet oder in pending vorhanden sind. Maximal 50 Personen,
100 zugeordnete Chips. Namen werden getrimmt, dürfen keine Steuerzeichen
enthalten und sind auf 48 UTF-8-Bytes begrenzt.
**/api/forget** löscht weder historische Bezüge noch separat gespeicherte
Kontaktzeitpunkte. Erneutes Vorhalten kann den Chip wieder als unbekannt erfassen.

**/api/report** verwendet ohne Query die aktuelle ISO-Woche und den aktuellen
Monat; ohne Uhrzeit leere Periodenschlüssel. Antwort: week, month, totals,
people, weeks, months. totals und Personeneinträge enthalten total, today,
week, month, undated; Personen zusätzlich id und name.
weeks und months listen Perioden mit nicht stornierten datierten Bezügen,
neueste zuerst. Undatierte Bezüge zählen zur Gesamtsumme, nicht zu Kalenderperioden.
Query-Perioden im Tabellenformat angeben; der Server validiert das Format
nicht gesondert. Ein nicht passender Schlüssel ergibt dafür null Bezüge.

**/api/history** liefert rows mit id, person, name, epoch, cancelled und more.
Nächste Seite: before gleich der kleinsten ID der aktuellen Seite setzen;
die Grenze ist exklusiv. more:true bedeutet weitere ältere Bezüge.
Stornos bleiben im Verlauf. Wiederholtes Stornieren derselben ID ist
erfolgreich, ohne einen weiteren Stornoeintrag zu erzeugen.

**/api/export.csv** liefert UTF-8 mit BOM und Semikolon-Trennung:
id;person_id;name;zeit_berlin;epoch;storniert.
Downloadname: Cafe_R107.csv. Namen entsprechen dem aktuellen Personennamen;
undatierte Zeitfelder bleiben leer. Stornierte Bezüge bleiben enthalten.

### Uhrzeit, Konfiguration und Aktionen

**/api/time** erwartet Unix-Sekunden im Bereich 2024–2037.
Die Uhr wird sofort gesetzt; ein Zeitanker kann undatierte Bezüge derselben
Startsitzung datieren. Keine batteriegepufferte Echtzeituhr.
Dieser Endpunkt benötigt schreibbereiten Speicher. Falls das Speichern des
Zeitankers scheitert, kann HTTP 200 mit ok:false zurückkommen.

Bei **/api/config** bleiben nicht übergebene Felder unverändert.
ssid maximal 32 Bytes, wifiPassword maximal 63 Bytes.
Ein leeres wifiPassword **löscht** das Client-WLAN-Kennwort.
Leere apPassword/adminPassword behalten dagegen ihren bisherigen Wert;
nichtleere Kennwörter müssen 8–63 Bytes lang sein.
Eine leere ssid beendet Client-Verbindungsversuche nach dem nächsten Neustart.
Der Aufruf speichert nur; Netzwerk-/Zugangsänderungen gelten erst nach
Neustart, den der Endpunkt nicht selbst auslöst.

**/api/display/on** erzwingt Beleuchtung bis Moduswechsel/Neustart.
**off** löscht beide Aktivitätsfristen und aktiviert auto; neuer RFID-Kontakt
oder erneute TOF-Näherung können sofort wieder einschalten.
**auto** hebt den erzwungenen Modus auf, verlängert aber keine Frist.
Die Modi werden nicht gespeichert; nach Neustart gilt auto.
Gemeint ist die Hintergrundbeleuchtung an GPIO21; Buchung und Sensorik laufen weiter.

**/api/rfid/reinit** setzt nur den Reader über RST22 zurück.
Kartensperre und Buchungssperrzeit bleiben erhalten; ESP, Uhrzeit und TOF laufen weiter.
Antwort: ok und rfid; bei gescheiterter Registerprüfung HTTP 503.

**/api/restart** antwortet zuerst mit HTTP 202:
{"ok":true,"restarting":true,"delayMs":500}.
Der Neustart erfolgt nach etwa 500 ms in der Hauptschleife.
Wiederholte Anforderungen verschieben einen geplanten Neustart nicht nach hinten.
Uhrzeit muss erneut synchronisiert werden; Konfiguration und Buchungen bleiben.

### Fehlerantworten

| HTTP-Code | Bedeutung |
|---|---|
| 200 | Normaler erfolgreicher Aufruf; gegebenenfalls ok zusätzlich prüfen |
| 202 | ESP-Neustart angenommen |
| 400 | Ungültiges JSON, Parameter, Name, Uhrzeit oder Konfiguration |
| 401 | HTTP-Basic-Anmeldung fehlt/ist falsch; Login-Abfrage |
| 403 | POST ohne korrekten X-Cafe-Request: 1 |
| 404 | Pfad, Person, Buchung oder zuvor erfasster Chip nicht gefunden |
| 409 | Personen-/Chiplimit erreicht |
| 503 | Speicher/Journal nicht bereit oder RFID-Reinit gescheitert |
| 507 | Schreiben fehlgeschlagen |

Anwendungsfehler sind üblicherweise JSON {"error":"Beschreibung"}.
RFID-Reinit liefert bei HTTP 503 stattdessen ok:false und das rfid-Objekt.
Die Authentifizierungsantwort kann vom JSON-Format abweichen.

### Aufrufbeispiele

Im Browser nach Admin-Anmeldung:

- http://192.168.123.60/api/status
- http://192.168.123.60/api/report?week=2026-W43&month=2026-10
- http://192.168.123.60/api/display/off
- http://192.168.123.60/api/rfid/reinit

Windows/PowerShell: curl.exe -u admin fragt das Admin-Kennwort ab.

~~~powershell
curl.exe -u admin http://192.168.123.60/api/diagnostics
curl.exe -u admin "http://192.168.123.60/api/history?before=123"
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" http://192.168.123.60/api/display/off
curl.exe -u admin -X POST -H "X-Cafe-Request: 1" -H "Content-Type: application/json" --data-binary "@person.json" http://192.168.123.60/api/person
~~~

Die lokale Datei person.json enthält beispielsweise {"name":"Beispielperson"}.
Unter Linux/macOS curl statt curl.exe verwenden.

### Dateimanager und OTA

ESPFMfGK läuft separat auf **Port 8080**, mit demselben HTTP-Admin-Login.
Seine Dateiverwaltungsrouten gehören nicht zur obigen Cafe-R107-API.
Oberfläche: http://192.168.123.60:8080/ oder http://192.168.4.1:8080/.
ArduinoOTA auf Port 3232 verwendet ab 1.0.13 das Geräte-Admin-Kennwort und PlatformIO,
keinen Pfad unter /api/.

## Neu in 1.0.11: RFID-Diagnose und gezielter Reader-Reset

**Update von 1.0.10:** `git pull`, dann `pio run -t upload`.
Nur Firmware; keine Website-Dateien und kein LittleFS-Upload erforderlich.
TOF-XSHUT bleibt an IO4. Keine zusätzliche Verdrahtung nötig.

Im Browser **`http://192.168.123.60/api/rfid/reinit`** aufrufen (Admin-Login).
GET und POST werden unterstützt; POST benötigt wie bisher `X-Cafe-Request: 1`.
Der Befehl setzt nur den RC522 über RST22 zurück und initialisiert ihn neu.
ESP, Uhrzeit, TOF und Daten bleiben erhalten. Die Kartensperre und Buchungssperrzeit
bleiben bestehen, damit eine weiterhin vorgehaltene Karte nicht nochmals bucht.
Die Antwort enthält `ok` und die RFID-Diagnose; bei gescheiterter Registerprüfung
HTTP 503. `ok:true` bestätigt die Registerprüfung, keine erfolgreiche Chip-Erkennung.

Unter `/api/diagnostics` und `/api/status` zeigt RFID zusätzlich:

- `antennaEnabled`, `antennaControlBits` (TX1/TX2), `antennaCheckAgeMs`:
  Zustand bei der letzten Registerprüfung, kein Nachweis eines funktionierenden HF-Feldes.
- `cardLatched`: aktuelle Kartensperre; Karte zum erneuten Buchen entfernen.
- `polls`, `uidReads`, `lastUid`, `lastUidAgeMs`, `lastPollAgeMs`.
- `lastWakeStatus` / `lastWakeStatusText`: letzter WUPA-Status;
  `lastSelectStatus` / `lastSelectStatusText`: letzter Auswahlstatus,
  der auch von einem früheren Poll stammen kann.
- `consecutiveReadErrors`: aufeinanderfolgende auffällige Polls;
  `verification: registers_only` macht die begrenzte Aussage von `healthy` deutlich.

Vor dem ersten entsprechenden Vorgang sind Status/UID/Alter null.
Der Minutencheck prüft jetzt neben VersionReg auch die Aktivierung beider
Antennentreiber (TxControlReg). Drei Proben mit Mehrheitsentscheidung tolerieren
einen einzelnen Ausreißer. Bei fehlerhaften Registern oder zehn aufeinanderfolgenden
auffälligen Polls folgt ein Reader-Reset. Dazu zählen WUPA-Fehler außer normalen
Timeouts/Kollisionen sowie fehlgeschlagene Kartenauswahl nach erfolgreichem WUPA.
Jeder unauffällige Poll setzt den Fehlerzähler zurück. Eine bereits erkannte
Fehlerserie bleibt als `readFaultPending` bis zur Wiederherstellung oder einer
erfolgreichen UID-Lesung gespeichert. Erfolgreiche UID-Lesevorgänge
bleiben unverändert auf eine Buchung pro neuem Kontakt begrenzt.

**Grenze:** Stille HF-Ausfälle, die nur wie eine abwesende Karte erscheinen,
können ohne Testkarte nicht automatisch erkannt werden. Längere Nichtbenutzung
führt deshalb nie allein zu einem Reset. Kein regelmäßiger Selbsttest und kein
automatischer ESP-Neustart. Bei erneutem Ausfall zunächst Diagnose sichern, dann
`/api/rfid/reinit` aufrufen und anschließend den Chip erneut testen.

## Neu in 1.0.10: TOF-Hardware-Reset

**Verdrahtung:** VL53L0X **XSHUT → ESP32 GPIO4 (IO4)**, bei ausgeschalteter
Versorgung anschließen. GPIO1 am Sensor bleibt frei. SDA16/SCL17 bleiben bestehen.
GPIO4 wird ausschließlich LOW geschaltet oder als Eingang ohne Pull-up
freigegeben, niemals aktiv HIGH. Der Pull-up auf dem vorhandenen Modul übernimmt
beim Freigeben den Ruhepegel (am verwendeten Modul wurden 3,3 V gemessen).

**Update von 1.0.9:** `git pull`, dann `pio run -t upload`.
Nur Firmware hochladen; keine Website-Dateien und kein LittleFS-Image erforderlich.
Die Displaydrehung aus 1.0.9 bleibt erhalten.

Bei einem Fehler versucht der Minutencheck zuerst eine normale Neuinitialisierung.
Wenn bis zum nächsten Check keine Messung angekommen ist, folgt ein Hardware-Reset:
XSHUT 10 ms LOW, freigeben, 10 ms warten, Bibliothekszustand zurücksetzen und
Sensor neu initialisieren. Weitere erfolglose Versuche bleiben bei Hardware-Reset.
Nach einer empfangenen Messung startet ein späterer neuer Fehler wieder mit der
Software-Stufe. Der ESP wird dabei nicht neu gestartet.

In `/api/diagnostics` und `/api/tof` kommen für TOF hinzu:

- `originalError`: erster beim Supervisor erkannter Fehler der letzten Störung;
  bleibt auch nach Wiederherstellung sichtbar und wird bei einer neuen Störung ersetzt.
- `initStage` / `initError`: aktueller Schritt / gescheiterter Schritt
  (`address_probe`, `stop_continuous`, `sensor_init`, `timing_budget`, `start_continuous`).
  `sensor_init` bezeichnet den Bibliotheksaufruf, keine interne Kalibrierungs-Unterstufe.
- `initTimeout`: Timeout während der letzten Initialisierung;
  `lastI2cStatus`: letzter Wire-Status (0 = erfolgreich).
- `hardwareResets`, `recoveryPending`, `xshutPin` (4).

`successfulReinitializations` zählt beim TOF jetzt erst nach einer neuen
abgeschlossenen Messung, genau einmal je Wiederherstellungsversuch. Eine Messung
außerhalb des nutzbaren Entfernungsbereichs bestätigt ebenfalls die Kommunikation,
weckt aber nicht das Display. Alte Messzeitpunkte bleiben für `ageMs` erhalten.
Ein Hardware-Reset kann nicht jeden elektrischen Defekt oder Versorgungsausfall beheben.
Der Dauertest mit dem realen Modul steht noch aus.

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
- Bei erfolglosem Versuch folgt beim nächsten Minutencheck die XSHUT-Stufe (ab 1.0.10).
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
Beim RFID zählen erfolgreiche Initialisierungsaufrufe; beim TOF (ab 1.0.10)
zählt erst die folgende abgeschlossene Messung als Erfolg und bestätigt `healthy:true`.
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

- `available`: Sensor ist erfolgreich initialisiert, beim Start oder nach Wiederherstellung.
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
| XSHUT | GPIO 4 (seit 1.0.10) |
| GPIO1 | unbeschaltet |

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
| VL53L0X | VIN / GND / SDA / SCL / XSHUT | 3V / G / 16 / 17 / 4 |

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

- Standard: AP und Client laufen gleichzeitig (`WIFI_AP_STA`); mit apOnly nur AP (`WIFI_AP`). Der AP bleibt auch bei
  fehlendem oder falschem Client-WLAN verfügbar. Das Gerät wartet beim Start
  nicht auf eine Verbindung. Der Dateimanager startet ebenfalls im AP-Betrieb.
- Bei Client-Verbindung wird die vom Router zugewiesene Adresse auf dem LCD
  und im Web angezeigt. Dateimanager: dieselbe Adresse auf **Port 8080**.
- Beim WLAN-Kanalwechsel durch den verbundenen Client kann die AP-Verbindung
  kurz unterbrochen werden; danach wieder verbinden.
- NTP startet nur bei bestehender Client-Verbindung und deaktiviertem AP-only-Modus. Zeitzone Berlin mit
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

4 MB Flash: zwei App-Partitionen zu je 1,5 MiB, LittleFS **960 KiB**.
Seit 1.0.12 sind Firmware-Updates über ArduinoOTA möglich (siehe oben). **Firmware-Upload** lässt bei unveränderter Partitionstabelle
LittleFS bestehen; **Filesystem-Upload ersetzt das komplette Dateisystem**.

| Datei auf dem ESP | Inhalt |
|---|---|
| `/config.json` | Drei Client-WLANs, AP-only-Modus, AP- und Admin-Kennwort |
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


