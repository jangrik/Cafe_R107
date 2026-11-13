# Cafe R107 · Version 1.1.6

RFID-Kaffeeerfassung für ein kleines Büro: ESP32, RC522, Waveshare ST7789
240 × 240 und passiver elektromagnetischer Summer. PlatformIO/Arduino,
Weboberfläche und Daten in LittleFS. Kein Eingriff in die Espressomaschine.


**Direkt zu:** [OTA-Update](#firmware-per-ota-aktualisieren) ·
[HTTP-API](#vollständige-http-api-stand-1014) ·
[Pinbelegung](#hardware--funktionierende-pinbelegung) ·
[Sicherung und Updates](#littlefs-sicherung-und-updates)

## Neu in 1.1.6

Unter **Übersicht → Die letzten Buchungen** lässt sich ein Bezug mit
„Ohne Zuordnung“ über **Zuordnen** einer vorhandenen Person zuweisen.
Auch ältere, nicht stornierte Bezüge können zugeordnet werden. Der ESP
hängt einen Journaleintrag an und verwendet die Zuordnung in Verlauf,
Statistik, Rangliste und CSV. Die ursprüngliche Bezugs-ID und Bezugszeit
bleiben erhalten.

Für das Update von 1.1.5 die Firmware aktualisieren und `data/www/app.js`
im Dateimanager als `/www/app.js` ersetzen. Danach die Seite neu laden.

## Neu in 1.1.5

Der Shelly sendet die letzten fünf Sekundenmesswerte mit jedem Lebenszeichen.
Im ESP bleibt bis zu eine Stunde im RAM. Unter **Leistungsverlauf** lassen sich
Leistung und Erkennungsphasen ansehen, das Zeitfenster verschieben und bestätigte
Bezüge an grünen Markierungen erkennen. Die CSV unter
`/api/power.csv?download=1` enthält den gesamten verfügbaren Verlauf.
Längere Verbindungsunterbrechungen hinterlassen Lücken. Ein ESP-Neustart leert
den Verlauf. Das Verbindungssymbol auf dem Gerät ist jetzt 34 × 20 Pixel groß.

Für das Update die Firmware installieren, dann `index.html`, `app.js` und
`style.css` aus `data/www/` im Dateimanager einzeln in `/www/` ersetzen.
Das Shelly-Skript `shelly/coffee-detector.js` aktualisieren, dabei `API_KEY`
und gegebenenfalls `ESP_URL` übernehmen. Das Skript starten und die Webseite
neu laden. Der lokale Shelly-Puffer bleibt bei 180 Messwerten.

## Neu in 1.1.3

Die Erkennung kann nach einem gültigen Mahlabschnitt direkt in die Brühphase
wechseln, wenn Pumpe oder Heizung weiterlaufen. Die vorherige Version verlangte
an dieser Stelle eine Leistungspause unter 30 W und konnte deshalb warme
Folgebezüge verwerfen. Aktive Pumpen-/Heizzeit wird jetzt aufsummiert, damit
weit auseinanderliegende kurze Leistungsspitzen keine volle Brühphase ergeben.
Eine kurze erste Heiz-/Pumpenphase mit anschließender Vorbrühpause wartet auf
die Fortsetzung, statt den validierten Mahlabschnitt sofort zu verwerfen.
Beide ursprünglichen EP0800-CSV-Messreihen liegen als Testdaten unter
test/fixtures und werden bei jedem JavaScript-Testlauf nachgespielt.
Der Mindestmahlabschnitt von 3 Sekunden, die maximale Mahlphase von 18 Sekunden
und die Bestätigung des Bezugsendes bleiben Kriterien. Die Korrektur deckt den
gemeldeten Phasenwechsel im Testmodell ab. Die Messkurve des konkreten
fehlgeschlagenen warmen Bezugs liegt noch nicht vor.

Das Shelly-Skript hält automatisch die letzten 180 Leistungsmesswerte im RAM.
Nach einem weiteren Fehlversuch innerhalb von drei Minuten im Browser
http://<Shelly-IP>/script/<Skript-ID>/trace öffnen und die CSV zusammen mit
/script/<Skript-ID>/status sichern. Der Status enthält den letzten Entscheid
und dessen Geräte-Laufzeit. Ein Neustart des Skripts leert den Messpuffer.
Der Export ist auch während laufender Erkennung möglich.

Auf der Übersicht sitzt der Shelly-Status rechts neben „Wer hat schon Kaffee?“
über dem CSV-Button. Er zeigt Bereitschaft, Erkennungsphase, aktuelle
Wirkleistung und gemeldete Fehler. Bei fehlendem gesunden Lebenszeichen wird
die Leistung ausgeblendet. Die Aktualisierung folgt dem vorhandenen
Acht-Sekunden-Intervall. Dunkle Buttons werden beim Hover deutlich brauner
mit Schatten; helle Buttons erhalten einen goldenen Hintergrund und
kräftigeren Rand. Der Fokusrahmen für Tastaturbedienung bleibt sichtbar.

Zum Update Firmware 1.1.3 installieren und die Dateien data/www/index.html,
data/www/app.js und data/www/style.css einzeln nach /www/ hochladen.
Das Shelly-Skript durch shelly/coffee-detector.js ersetzen, den eigenen
API_KEY und gegebenenfalls ESP_URL übernehmen, speichern und neu starten.
Anschließend die ESP-Webseite vollständig neu laden. Das gesamte
Dateisystem-Image würde die gespeicherten Gerätedaten ersetzen.

## Neu in Firmware 1.1.2: Journal der Vormerkungen

Eine erfolgreich angelegte Chip-Vormerkung wird als `kind: "reservation"` mit
fortlaufender ID, Personen-ID, epoch, session und uptime in journal.ndjson
protokolliert. Sie zählt nicht als Kaffee und wird nach einem Neustart nicht
als aktive Vormerkung wiederhergestellt. Erneutes Vorhalten bei bestehender
Vormerkung legt keine weitere Vormerkung an. Bei Schreibfehler wird keine
Vormerkung bestätigt. Direkte Bezugszuordnungen und Offline-Sofortbuchungen
bleiben coffee-Einträge.

Alle neuen Journalzeilen enthalten `_info.person` (Namensstand beim Schreiben)
und `_info.datetime` (Berliner Ortszeit mit CET/CEST, sonst „Zeit unbekannt“).
Diese Zusatzfelder werden nicht ausgewertet. Alte Zeilen bleiben unverändert;
spätere Zeitanker ändern die bereits geschriebenen Infotexte nicht.

Für eine manuelle Nachbuchung nach Sicherung eine reservation-Zeile kopieren,
`kind` auf `coffee` setzen und mit einer neuen ID größer als alle bisherigen
IDs am Dateiende anfügen; person, epoch, session und uptime beibehalten.
Nur nach Prüfung, dass der Bezug nicht bereits gebucht wurde. Anschließend
ESP neu starten. Das Original als Nachweis stehen lassen. ID-Lücken sind erlaubt,
doppelte oder rückläufige IDs nicht. Eine Vormerkung allein belegt keinen Bezug.

Update: Nur Firmware hochladen; kein LittleFS-Upload erforderlich.
Das vorhandene Shelly-Erkennungsskript bleibt unverändert.

## Shelly-Skript 1.1.1

Nach einem erkannten Mahlvorgang mit anschließender Pause wird eine Brühphase
ab 35 W Pumpenleistung berücksichtigt. Die bisherige Grenze von 350 W
konnte warme Bezüge ohne längere Heizphase verwerfen. Das ist eine plausible
Ursache für einen gemeldeten fehlenden zweiten Kaffee; eine Messkurve dieses
konkreten Bezugs liegt noch nicht vor. Mahlmuster, Mindestdauer und
Bezugsendebestätigung bleiben Voraussetzungen.

Zum Einspielen shelly/coffee-detector.js im Shelly-Skripteditor ersetzen,
den vorhandenen API_KEY und gegebenenfalls ESP_URL wieder eintragen,
speichern und neu starten. Für diese Änderung genügt das Shelly-Skript.
Vor einem Skriptneustart offene Übertragungen möglichst abschließen lassen.

Die Diagnose unter /script/<id>/status zeigt zusätzlich recognized
(erkannte und zum Speichern vorgesehene Bezüge seit Skriptstart) und
lastDecision (zuletzt abgeschlossener oder verworfener Erkennungsversuch).
Abbruchgründe werden auch in der Shelly-Skriptkonsole ausgegeben.
Die Prüfung umfasst nun zwei aufeinanderfolgende simulierte Kaffees mit
Heizleistung beim ersten und reiner Pumpenleistung beim zweiten sowie
Spülen, Heißwasser und einen abgebrochenen kurzen Pumpenlauf.

## Neu in 1.1.1

Die WLAN-Suche wartet auf das Ereignis SCAN_DONE, bevor sie die Suchergebnisse
ausliest. In Arduino-ESP32 2.0.17 führt ein vorzeitiger Aufruf von scanComplete()
bei der gewählten Kanalzeit von 120 ms bereits nach 2400 ms zum Fehler -2.
Der tatsächliche AP+STA-Suchlauf benötigte im gemeldeten Protokoll rund
4350 ms. Die Ergebnisse wurden dadurch verworfen, bevor sie bereitstanden.
Ein eigener 15-Sekunden-Watchdog beendet einen Suchlauf, dessen Abschlussereignis
ausbleibt. Auch mit verbundenem Shelly bleiben STA-Verbindungsversuche möglich.

Für das Update von 1.1.0 genügt ein Firmware-Upload. Das Shelly-Skript und
die Webdateien sind weiterhin zur Version 1.1.1 kompatibel.

## Neu in 1.1.0

Die Philips-EP0800-Erkennung läuft auf einem Shelly Plug M Gen3 oder Plug S Gen3.
Das Skript steht in [shelly/coffee-detector.js](shelly/coffee-detector.js).
Es erkennt einen Mahlabschnitt mit anschließender Brühphase. Die ersten
Schwellen stammen aus zwei Messreihen mit insgesamt zwei Kaffees und zwei
Heißwasserbezügen. Sie sind ein Ausgangspunkt für den Praxistest. Ein Bezug
mit vorgemahlenem Kaffee hat kein Mahlmuster und wird damit nicht erkannt.

### Installation

1. Das gesamte LittleFS über den Dateimanager sichern, insbesondere
   config.json, registry.json, lastseen.json und journal.ndjson.
2. Firmware 1.1.0 über PlatformIO bauen und hochladen.
3. Die beiden Dateien data/www/app.js und data/www/index.html über den
   Dateimanager einzeln nach /www/app.js und /www/index.html hochladen.
   Anschließend die Browserseite vollständig neu laden.
4. Den Shelly als Client ausschließlich mit dem ESP-WLAN verbinden.
   Matter deaktivieren. Das bisherige Messskript stoppen und dessen Autostart
   ausschalten. Ein neues Skript mit coffee-detector.js anlegen.
5. In der ESP-Website unter „Gerät & WLAN“ den Shelly-Schlüssel ablesen
   und im Skript als API_KEY eintragen. ESP_URL ist standardmäßig
   http://192.168.4.1/api/shelly und muss zur AP-Adresse passen.
6. Das Skript speichern, starten und „Run on startup“ aktivieren.
   Die ESP-Website muss „Erkennung aktiv“ zeigen.
   /script/<id>/status auf dem Shelly zeigt Zustand, Leistung und Puffer.

Ein vollständiges Dateisystem-Image überschreibt Gerätedaten. Für dieses
Update sind einzelne Webdateien vorgesehen. Ältere Firmware versteht den
neuen Journaltyp brew nicht. Ein Downgrade erfordert die passende Sicherung.

### Zuordnung und akustische Erinnerung

Das Skript sendet alle fünf Sekunden ein authentifiziertes Lebenszeichen.
Nach mehr als 15 Sekunden ohne gesundes Lebenszeichen bucht ein bekannter
Chip sofort einen Kaffee. Ein Netzwerkausfall wird dabei wie ein gestopptes
Skript behandelt, weil der ESP diese Fälle nicht sicher unterscheiden kann.

Bei aktiver Erkennung merkt sich der ESP genau einen vorgehaltenen Chip
für 120 Sekunden. Ein weiterer Scan ersetzt die Vormerkung nicht.
Die Vormerkung wird beim bestätigten Bezug verbraucht. Alternativ kann der
Chip bis einschließlich zehn Sekunden nach erkanntem Bezugsende vorgehalten
werden. Die vier Sekunden zur Bestätigung des Bezugsendes werden auf diese
Frist angerechnet. Danach folgen 30 Sekunden aus Gruppen schneller,
abwechselnd 2100 und 3200 Hz hoher Pieptöne. RFID und Webserver bleiben währenddessen bedienbar. Ein nachgereichter Chip ordnet den
Bezug zu und beendet die Erinnerung auch während dieser 30 Sekunden.
Danach wird der Bezug als „Ohne Zuordnung“ gebucht. Er zählt zur Gesamtmenge
und erscheint im Verlauf, aber nicht in der persönlichen Top 5.

Bereits vor der Übertragung ältere Bezüge werden ohne Alarm übernommen.
Offene Bezüge nach einem ESP-Neustart werden ohne Alarm als unzugeordnet
abgeschlossen. Die Vormerkung eines noch nicht zugeordneten Chips liegt
nur im RAM. Ein Neustart verwirft sie.

### Speicherung und Wiederholungen

Der Shelly hält bis zu 40 erkannte Bezüge in KVS vor, jeweils mit eigener ID.
Er löscht sie erst nach einer passenden ESP-Speicherbestätigung.
Der ESP schreibt zunächst einen brew-Datensatz ins bestehende Journal und
später den zugeordneten coffee-Datensatz. Wiederholungen und ein Neustart
zwischen beiden Schritten erzeugen keinen zweiten Bezug. Bei ausgeschaltetem
ESP läuft die Erkennung weiter, solange der Shelly Strom hat.
Ein voller Puffer oder ein Speicherfehler wird im Skriptstatus gemeldet und
deaktiviert die gesunde Erkennungsbestätigung. Weitere Bezüge bei vollem
Puffer können nicht gespeichert werden. Ein noch laufender Bezug wird bei
einem Skript-/Shelly-Neustart verworfen.

Zeitlich passende Sofortbuchungen aus einem Verbindungsabbruch können mit
nachgelieferten Bezügen verknüpft werden, ohne erneut zu zählen. Das erfolgt
anhand der Laufzeit in derselben ESP-Sitzung oder bekannter Uhrzeiten
und ist bei zeitlich überlappenden Bezügen eine Näherung. Ohne vergleichbare
Zeitbasis bleiben nachgelieferte Bezüge unzugeordnet.
Der ESP liefert seine gültige Uhrzeit an das Skript. Nach einem Shelly-Neustart
ohne Zeitquelle bleibt die Zeit unbekannt. Leistungs-Rohdaten werden nicht
dauerhaft geschrieben, nur abgeschlossene Ereignisse.

### Access Point und AP+STA

„Gerät & WLAN“ listet aktuell verbundene AP-Clients mit MAC- und IP-Adresse.
Ein erkanntes Shelly-Lebenszeichen markiert den entsprechenden Client.
Vom Notebook im selben ESP-WLAN lässt sich die IP des Shelly direkt öffnen.
Diese Verbindung verwendet das AP-Netz. Eine Weiterleitung aus dem Büronetz
in das AP-Netz ist nicht eingerichtet.

STA-Wiederverbindung funktioniert auch bei verbundenen AP-Clients. Kurze
asynchrone WLAN-Suchläufe wiederholen sich dann höchstens einmal pro Minute.
ESP32 verwendet für AP und STA denselben Funkkanal. Beim Wiederverbinden mit
einem anderen Kanal müssen sich AP-Clients gegebenenfalls neu verbinden.

### Prüfungen für 1.1.0

Beim Nachspielen der beiden gelieferten CSVs erkennt das Skript genau zwei
Kaffees in Reihe 1 (Endmarken 105,005 und 216,005 Sekunden) und keinen Kaffee
in der Heißwasser-Reihe 2.

Native Tests prüfen Fristen, Fallback-Zeitfenster und AP+STA-Suchintervalle.
Die vorhandenen Displaytests verwenden nun die tatsächlich konfigurierten
Seiten- und Beleuchtungszeiten statt veralteter fester Werte.
JavaScript-Tests prüfen Erkennung, Messlücken, Wiederanlauf aus KVS und die
Bestätigung vor dem Löschen gepufferter Ereignisse.
Vor dem produktiven Einsatz sind ein kompletter Kaffeebezug mit Chip,
ein Bezug ohne Chip, Heißwasser, ein ESP-Ausfall und ein gestopptes
Shelly-Skript am echten Gerät zu prüfen.

## Neu in 1.0.17: TOF-Qualitätsdiagnose für falsche Nahwerte

Diese Diagnoseversion ergänzt **GET /api/tof** um **rawDistanceMm** und ein
Objekt **quality**. Sie ändert bewusst noch keine Grenzwerte und verwirft noch
keine Messungen anhand der Signalqualität. **valid** verwendet weiterhin die
bestehende Transport-, Frische- und 20–2000-mm-Prüfung; ein falscher Nahwert
kann die TOP-Anzeige daher weiterhin auslösen. Die Messwerte dienen zuerst
zum Vergleich der Fenstersituation ohne Hand mit einer echten Hand bei ca. 100 mm.

### Neue Felder

| Feld | Bedeutung |
|---|---|
| rawDistanceMm | Ungefilterte Entfernung des zuletzt vollständig gelesenen Ergebnisblocks, auch außerhalb 20–2000 mm; bei fehlendem/fehlerhaftem Block null |
| quality.readOK | Letzter gelesener Ergebnisblock war vollständig (12 Bytes); kein Urteil über Messqualität oder erfolgreiche Interruptquittierung |
| quality.readStatus | Wire-EndTransmission-Status; 254 ist der Anwendungscode für einen unvollständigen Ergebnisblock |
| quality.ageMs / fresh | Alter des Ergebnisblocks in ms / höchstens 500 ms alt |
| quality.deviceStatusRaw | Vollständiges Statusregisterbyte |
| quality.deviceStatusCode | Geräte-Code aus Bits 6:3 des Statusbytes |
| quality.deviceStatusName | Einordnung des Geräte-Codes, siehe unten |
| quality.deviceRangeComplete | Geräte-Code ist 11 (range_complete); keine vollständige ST-Qualitätsfreigabe |
| quality.signalRateMcps | Rücksignal, Millionen Zählimpulse pro Sekunde |
| quality.ambientRateMcps | Umgebungslichtsignal, gleiche Einheit |
| quality.effectiveSpads | Effektive Zahl der verwendeten Detektorzellen (SPADs), einschließlich Bruchteil |
| quality.signalToAmbientRatio | Rücksignal geteilt durch Umgebungslichtsignal; null bei Ambient=0; kein berechnetes statistisches SNR |
| quality.deviceStatusCounts | Array mit 16 Zählern, Index = Geräte-Code; seit ESP-Start, ohne Zurücksetzen bei Sensor-Reinitialisierung |
| quality.statusSource | raw_device_register |
| quality.qualityChecksApplied | false: Die Diagnose verändert die Akzeptanz der Entfernung noch nicht |
| quality.hostSigmaComputed | false: Die vollständige Host-Sigma-Schätzung der ST-API wird hier nicht berechnet |

Codes 1/2/3 heißen hardware_failure, 4 signal_failure, 6/9 phase_failure,
8/10 minimum_range_failure, 11 range_complete. Andere Codes werden als
no_classified_range bezeichnet. **Diese Rohcodes sind nicht die RangeStatus-
Nummern der vollständigen ST-API** (dort bedeutet beispielsweise 0 gültig).
Auch bei Code 11 kann eine vollständige Host-Qualitätsprüfung noch Fehler melden.
Es wird deshalb kein erfundener Sigma-Wert oder pauschaler Grenzwert ausgegeben.

Vor dem ersten Ergebnisblock und nach einem fehlgeschlagenen Blocklesen sind
rawDistanceMm und die Ergebnisfelder null; fresh ist false. Nach älteren
Ergebnisblöcken können Werte weiter sichtbar sein: ageMs/fresh beachten.
Die bisherigen Diagnosefelder und ageMs auf der obersten Ebene bleiben erhalten.
Ein Qualitätsproblem oder ein Ziel außerhalb der Reichweite ist allein kein
I2C-Stillstand und löst deshalb keinen automatischen Reset aus.

Entfernung und Qualität kommen aus **demselben 12-Byte-Ergebnisblock ab 0x14**,
der vor dem Löschen des Messinterrupts gelesen wird. Es wird keine zusätzliche
Einzelmessung gestartet. Unvollständige I2C-Antworten werden als Transportfehler
behandelt. Das Layout und die Geräte-Code-Einordnung folgen der ST-Implementierung
von GetRangingMeasurementData/get_pal_range_status; die verwendete Pololu-
Bibliothek las bisher lediglich das Entfernungswort aus diesem Block.

Referenzen:

- [ST VL53L0X API-Handbuch UM2039](https://www.st.com/resource/en/user_manual/um2039-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf)
- [ST-API in Adafruits Treiber: Ergebnisblock](https://github.com/adafruit/Adafruit_VL53L0X/blob/master/src/core/src/vl53l0x_api.cpp)
- [ST-API: Geräte-Status und Host-Qualitätsprüfung](https://github.com/adafruit/Adafruit_VL53L0X/blob/master/src/core/src/vl53l0x_api_core.cpp)

### Vergleich am Gerät

1. Firmware aktualisieren; **keine Website-Dateien und kein uploadfs** erforderlich.
2. Ohne Hand in der problematischen Fensterausrichtung **/api/tof** mehrmals
   öffnen und vollständige JSON-Antworten sichern, besonders bei falschen ~100 mm.
3. Mit echter Hand bei ca. 100 mm ebenso mehrere Antworten sichern, möglichst
   direkt anschließend bei denselben Lichtverhältnissen.
4. Optional seriell bei 115200 Baud mitlesen: Bei laufenden Messungen erscheint
   höchstens einmal pro Sekunde eine **[TOF … ms]**-Zeile mit Rohentfernung,
   Geräte-Status, Signal, Ambient, SPADs und distanceAccepted.

Die neue Zeile enthält keine Kennwörter. Vor dem Teilen eines vollständigen
seriellen Logs die bisherige Startausgabe mit AP-/Admin-Kennwörtern entfernen.
Erst anhand des Vergleichs lassen sich passende Qualitätsfilter beurteilen.
Ein dauerhaft als gut gemeldeter falscher Nahwert kann mit den Rohwerten allein
unter Umständen weiterhin nicht von einer Hand unterschieden werden.

## Neu in 1.0.16: TOP-Zyklus gegen kurze TOF-Aussetzer stabilisieren

Ein einzelner ungültiger oder veralteter Messwert hat bisher die Rangliste
sofort beendet und den Zyklus auf Heute zurückgesetzt. Das konnte auch durch
eine verzögerte Hauptschleife nach HTTP-/Dateizugriffen ausgelöst werden.

Die Rangliste überbrückt jetzt **700 ms ohne frischen Nahwert**. Währenddessen
laufen Seitenfolge und Zehn-Sekunden-Timer unverändert weiter, auch wenn eine
Unterbrechung genau auf den Seitenwechsel fällt. Erst nach dauerhaft fehlendem
Nahwert endet die Anzeige; der nächste Handkontakt beginnt wieder mit Heute.
Die 120-mm-Einschaltschwelle bleibt unverändert. Zusammen mit der vorhandenen
Messwert-Frische von höchstens 500 ms kann das Ende nach komplett ausbleibenden
Sensormessungen bis etwa 1,2 Sekunden nach der letzten Nahmessung erfolgen.

Vor der Anzeigeentscheidung wird der TOF nochmals nichtblockierend abgefragt,
falls vorher Webserver/Dateimanager die Schleife verzögert haben.
GET /api/status ergänzt **rankingActive** und **rankingPeriod**
(today, month, all; bei inaktiver Anzeige der zurückgesetzte Wert today).
Das hilft, den Zyklus während eines Dauertests zu beobachten.

**Update:** git pull und nur Firmware hochladen. Keine Website-Dateien und
kein LittleFS-Upload. OTA-Datei und bisherige Einstellungen bleiben unverändert.
Tests decken ausdrücklich einen kurzen Aussetzer am Zehn-Sekunden-Wechsel,
fortgesetzte Seitenfolge, tatsächliches Entfernen und millis-Überlauf ab.
Ein Sensor-Dauertest am realen Gerät steht noch aus.

## Neu in 1.0.15: TOP-Seiten per Hand und lokale OTA-Datei

### TOP 5 durch Hand vor dem TOF

Die periodische 60-Sekunden-Anzeige entfällt vollständig. Bei einer frischen,
gültigen Distanz **unter 120 mm** erscheinen die TOP 5 sofort, solange keine
Startanzeige oder Buchungsbestätigung Vorrang hat. Reihenfolge:
**Heute → Dieser Monat → Insgesamt → Heute …**, jeweils **10 Sekunden**.
Die Hand hält die Hintergrundbeleuchtung aktiv. Nach Entfernen der Hand,
einer ungültigen Messung oder einem mehr als 500 ms alten Messwert erscheint
wieder die normale Anzeige. Bei erneutem Vorhalten beginnt der Zyklus mit Heute.
Die gültige Sensor-Untergrenze bleibt 20 mm; genau 120 mm startet keine Rangliste.
Die normale Näherungserkennung bei 80 cm bleibt für das Aufwecken bestehen.
Bei fehlender Uhrzeit zeigen Heute/Monat den bisherigen Hinweis; Insgesamt
enthält auch undatierte Bezüge. Eine RFID-Buchung behält ihre Bestätigung und
wird nicht von der Rangliste verdeckt.

### Eigenes OTA-Kennwort in einer lokalen Datei

Im Projektordner **ota.local.example.json** nach **ota.local.json** kopieren
und den Platzhalter ersetzen:

```json
{
  "password": "HIER_DEIN_EIGENES_OTA_KENNWORT_EINTRAGEN"
}
```

**ota.local.json** und deren lokale Sicherungskopien **ota.local.json.*** sind
in .gitignore ausgeschlossen. Nur die kennwortfreie Vorlage gehört ins Repository.
Die echte Datei liegt im Projektstamm, nicht in data/ und nicht im LittleFS.
Die Build-Skripte lesen sie automatisch für USB- und OTA-Builds. In die Firmware
wird der von ArduinoOTA benötigte MD5-Kennworthash eingebunden; das lokale Kennwort
wird nicht als Klartext-Konstante in den Quellcode geschrieben. Das OTA-Kennwort
ist damit unabhängig vom Admin-Kennwort der Website.

Upload aus PlatformIO wie gewohnt, ohne vorherige Umgebungsvariable:

```sh
pio run -e cafe_r107_ota -t upload
```

Wichtig beim ersten Wechsel von 1.0.14 auf das neue eigene Kennwort:
Die **laufende** Firmware erwartet noch ihr bisheriges Kennwort (in 1.0.14 das
Geräte-Admin-Kennwort). Dafür kann die lokale Datei vorübergehend so aussehen:

```json
{
  "password": "NEUES_OTA_KENNWORT",
  "uploadPassword": "BISHERIGES_OTA_KENNWORT_DER_LAUFENDEN_FIRMWARE"
}
```

password bestimmt die neu gebaute Firmware, uploadPassword authentifiziert nur
den aktuellen Upload. Nach erfolgreichem Wechsel uploadPassword entfernen,
damit weitere Uploads automatisch password verwenden. Alternativ das erste
Update per USB durchführen; dafür wird kein bisheriges OTA-Kennwort gebraucht.

Fehlt die lokale Datei, bleibt der bisherige Fallback erhalten: Die Firmware
verwendet das Geräte-Admin-Kennwort; ein OTA-Upload benötigt dann weiterhin
CAFE_OTA_PASSWORD. Das ermöglicht auch GitHub-Builds ohne private Datei.
Eine vorhandene, aber fehlerhafte lokale Datei bricht den Build mit einer
Fehlermeldung ab, statt unbemerkt ein anderes Kennwort zu verwenden.
Lokale Dateiinhalte und Kennwörter werden durch die Konfigurationsskripte nicht geloggt.

**Update von 1.0.14:** git pull, lokale OTA-Datei wie oben anlegen, Firmware
hochladen (USB oder OTA mit Übergangskennwort). **Keine Website-Dateien** und
kein LittleFS-Upload nötig. Vorhandene Konfiguration und Bezüge bleiben bestehen.
GitHub Actions prüft zusätzlich die lokale Dateiverarbeitung und baut eine
OTA-Firmware mit einer ausdrücklich nur für CI bestimmten Testkonfiguration.
Der Test am echten TOF und der OTA-Upload am Gerät stehen noch aus.

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
