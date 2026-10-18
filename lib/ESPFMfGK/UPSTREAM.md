# ESPFMfGK 2.0.17

Quelle: https://github.com/holgerlembke/ESPFMfGK
Commit: ac3b699c35705d34df06ed4a978fb5add4310463

Die Originaldateien aus src/, library.properties und LICENSE.md sind enthalten;
die Original-Webdateien aus filemanager/ liegen in ../../data/ als fm.*.
Die Lizenz des Autors einschließlich der dort genannten Einschränkungen bleibt
unverändert in LICENSE.md. Die lokalen Änderungen sind:

1. In src/ESPFMfGK.h ist fileManagerServerStaticsInternally deaktiviert, damit
   fm.html, fm.css und fm.js aus LittleFS ausgeliefert werden.
2. Die POST-Upload-Handler fileManagerReceiver und fileManagerReceiverOK prüfen
   ebenfalls die Authentifizierung, bevor Uploaddaten auf Dateien zugreifen.
   In Upstream 2.0.17 war die Prüfung für diese Handler nicht vorhanden.

Die Initialisierung im Anwendungsprojekt wartet nicht auf WL_CONNECTED.
