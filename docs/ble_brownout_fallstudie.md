# Fallstudie: Brown-Out-Bootloop beim BLE-Start (M5 Teil C2)

> Wissensdatenbank-Dokument, von Claude Code gepflegt (siehe `CLAUDE.md`). Kanonische
> Kurzfassung: Project Bible Kap. 4/5/6.5/9/10/12. Dieses Dokument ist als eigenständiger,
> zitierfähiger Abschnitt für die Bachelorarbeit formuliert.

## 1. Problemstellung

Nach Implementierung von M5 Teil C2 (BLE-Telemetrie auf Basis von NimBLE-Arduino) zeigte
die Firmware auf dem bis dahin verwendeten Development-Board (AZ-Delivery ESP32 NodeMCU
DevKit C V2) einen reproduzierbaren Bootloop: Beim Aufruf von `NimBLEDevice::init()`
löste der Brownout-Detektor (BOD) des ESP32 aus (Log-Meldung „E BOD: Brownout detector
was triggered"), gefolgt von einem Software-Reset (`SW_RESET`) und einem erneuten
Boot-Versuch — in einer Endlosschleife. Der Log-Marker „`[BLE] nach init()`" wurde zu
keinem Zeitpunkt erreicht; ein BLE-Advertising fand nicht statt.

Die Firmware von M5 Teil C2 ist vollständig host-getestet (75/75 Testfälle grün, `pio
test -e native`) und baut fehlerfrei für die Zielplattform (`pio run -e esp32dev`). Ein
Code-Defekt der neu hinzugefügten Logik (Telemetrie-Frame, Ringpuffer, BLE-Treiber)
konnte damit von Beginn an als alleinige Ursache ausgeschlossen werden. Als
Arbeitshypothese wurde ein Spannungseinbruch durch die Stromspitze der BLE-Funk-
Kalibrierung beim Hochlauf des BLE-Controllers angenommen.

## 2. Vorgehen / Methodik

Die Fehlersuche folgte einer systematischen Trennung zwischen Firmware- und
Hardware-Ursachen sowie einem hypothesengetriebenen, messtechnisch verifizierten
Vorgehen:

1. **Lokalisierung vor Behebung:** Mittels eines Bracket-Loggings (`Serial.flush()`
   unmittelbar vor und nach `NimBLEDevice::init()`) wurde der exakte Fehlerort auf den
   Aufruf von `init()` selbst eingegrenzt, bevor irgendeine Gegenmaßnahme implementiert
   wurde.
2. **Eine Variable pro Test:** Jede Gegenmaßnahme wurde einzeln, isoliert und mit
   sofortiger Verifikation am realen Gerät getestet — nie mehrere Änderungen gleichzeitig.
3. **Negativergebnisse wurden dokumentiert, nicht verworfen:** Jede wirkungslose
   Maßnahme wurde mit Ergebnis und Schlussfolgerung festgehalten (Tabelle in Kap. 3),
   um die Eliminationslogik nachvollziehbar zu halten.
4. **Messtechnische Verifikation vor Schlussfolgerung:** Annahmen über den Zustand der
   Stromversorgung wurden nicht unterstellt, sondern mit dem Multimeter nachgemessen
   (Spannungen, Übergangswiderstände), bevor sie als Ursache ausgeschlossen wurden.
5. **Entscheidendes Unterscheidungskriterium:** Als letzter diagnostischer Schritt wurde
   der Brownout-Detektor testweise deaktiviert, um zwischen einer Fehlauslösung des
   Detektors und einem echten Spannungskollaps zu unterscheiden (s. Kap. 7).

## 3. Testtabelle

| Nr. | Maßnahme | Ergebnis | Schlussfolgerung |
|---|---|---|---|
| 1 | Bracket-Logging `[BLE] vor/nach init()` | Fehler exakt IN `NimBLEDevice::init()`, vor jedem Telemetrie-Senden | Telemetrie/Senden als Ursache ausgeschlossen |
| 2 | Isolationstest: nacktes BLE, ohne Sensoren/I²C/PWM/RF/Watchdog | Brownout identisch | „Zu viele gleichzeitige Verbraucher"/Lastverteilung ausgeschlossen; reine RF-Kalibrierungsspitze |
| 3 | Nur-USB (Akku/MT3608 getrennt) | Brownout bleibt | Zwei-Quellen-Konflikt ausgeschlossen; MT3608 unter USB nicht im Versorgungspfad |
| 4 | BLE-TX-Leistung gesenkt (`NimBLEDevice::setPower()`) | Keine Wirkung | `setPower()` greift erst nach `init()`, nicht auf die Controller-RF-Kalibrierung |
| 5 | CPU-Takt auf 80 MHz während `init()` | Keine Wirkung | Spitze ist das Funkmodul, nicht die CPU |
| 6 | 1000-µF-Elko an 3V3↔GND (verlötet; < 1 Ω zu den Modul-Pins; misst 3,4 V) | Keine Wirkung | — |
| 7 | 1000-µF-Elko an Vin/5V↔GND (Reglereingang) | Keine Wirkung | — |
| 8 | MT3608-Ausgangsspannung erhöht (Akkupfad) | Keine Wirkung | — |
| 9 | Multimeter-Leerlaufmessung: 5 V = 5,1 V (gesund), 3V3 = 3,24 V (~0,44 V über der BOD-Schwelle); Widerstände im 3,3-V-/GND-Pfad < 1 Ω | Versorgung UND Verdrahtung elektrisch gesund | Kontaktwiderstand/kalte Lötstelle ausgeschlossen (fest verlöteter Lochraster-Aufbau) |
| 10 | Brownout-Detektor per Register deaktiviert (`WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0)`), verifiziert | „E BOD" verschwindet, aber das Board hängt nun bei `init()` und wird erst später vom RTC-/Task-Watchdog zurückgesetzt | Beweist einen ECHTEN Spannungskollaps am Silizium, keine Fehlauslösung — der BOD hatte den realen Fehler bisher kontrolliert/schnell abgefangen |

## 4. Eliminationslogik

Aus der Testreihe ergibt sich eine schrittweise Ausschlussargumentation:

- **Firmware als Ursache ausgeschlossen:** vollständig host-getestet (75/75), fehlerfreier
  Build, Fehler durch Bracket-Logging exakt auf die BLE-Controller-Initialisierung
  lokalisiert (Test 1).
- **Lastverteilung/gleichzeitige Verbraucher als Ursache ausgeschlossen:** Der Fehler tritt
  auch bei einem vollständig isolierten, „nackten" BLE-Start ohne jede weitere Peripherie
  auf (Test 2).
- **Zwei-Quellen-Konflikt (USB + Akku/MT3608) ausgeschlossen:** Der Fehler tritt auch im
  Nur-USB-Betrieb auf, bei physisch getrenntem Akkupfad (Test 3).
- **Software-seitige Leistungs-/Taktsteuerung als Abhilfe ausgeschlossen:** Weder eine
  reduzierte BLE-Sendeleistung noch ein reduzierter CPU-Takt während der Init-Phase zeigen
  eine Wirkung (Tests 4–5).
- **Versorgungspfad und Verdrahtung als Ursache ausgeschlossen:** Messtechnisch bestätigt
  gesunde Spannungen (5,1 V / 3,24 V) und Übergangswiderstände < 1 Ω schließen eine defekte
  oder mangelhafte Verkabelung sowie Kontaktprobleme aus (Test 9).
- **Reine Entkopplung/Pufferung als alleinige Abhilfe ausgeschlossen:** Zusätzliche
  Kapazität an beiden potenziell relevanten Schienen (3V3 und Vin) sowie eine erhöhte
  MT3608-Ausgangsspannung zeigen keine Wirkung (Tests 6–8) — ein Kondensator kann einen
  Spannungseinbruch nur zeitlich verzögern/abfedern, nicht dessen Ursache beheben, wenn die
  Ursache in der Nachregelgeschwindigkeit oder Belastbarkeit des Reglers selbst liegt.
- **Fehlauslösung des Brownout-Detektors ausgeschlossen:** Die testweise Deaktivierung des
  BOD führt nicht zu stabilem Betrieb, sondern zu einem Hängenbleiben an derselben Stelle,
  das erst später vom Watchdog aufgefangen wird (Test 10) — ein für den Betrieb
  unschädlicher, lediglich überempfindlich erkannter Transient hätte stattdessen zu einem
  unauffälligen Weiterlaufen führen müssen.

## 5. Root Cause

Der Onboard-3,3-V-Spannungsregler (AMS1117) des ursprünglich verwendeten Development-Boards
(AZ-Delivery ESP32 NodeMCU DevKit C V2) kann den durch die BLE-RF-Kalibrierung beim
Controller-Hochlauf verursachten Stromtransienten nicht liefern. Es handelt sich um eine
Hardware-Grenze bzw. einen Hardware-Defekt dieses konkreten Boards — wahrscheinlich verursacht
durch umfangreiches Löten/Rework am Board im Projektverlauf —, nicht um eine generische
Eigenschaft der ESP32-Plattform oder der BLE-Funktion, da Standard-ESP32-Boards BLE im
Batteriebuch beziehungsweise am USB-Anschluss regulär problemlos ausführen können.

## 6. Entscheidung und Begründung

**Board-Austausch:** Das Development-Board wird durch ein Espressif ESP32-DevKitC-32E
(Modul ESP32-WROOM-32E) ersetzt. Dieses Board ist das Referenz-Design von Espressif mit
entsprechend robusterer Spannungsregelung und ist pin-kompatibel zum bestehenden 38-Pin-
DevKitC-Layout, sodass ein reiner Board-Tausch ohne Neuverkabelung möglich ist. Die Wahl
ist über den ESP32-DevKitC Getting-Started-Guide (Board, Pinbelegung/Maße) und das
ESP32-WROOM-32E-Datasheet v2.0 (Modul, BLE, elektrische Daten) vollständig dokumentiert
und zitierfähig. Leitprinzip: „Leistung über Kosten".

**Entkopplungskondensatoren bleiben verbaut:** Die bereits verlöteten 1000-µF-Kondensatoren
an 3V3 und an Vin werden trotz nachgewiesener Wirkungslosigkeit gegen dieses spezifische
Problem nicht wieder entfernt. Sie verbessern die allgemeine Versorgungsstabilität und
Transienten-Robustheit (EMV, Lastspitzen) und sind Teil eines robusten
Stromversorgungsdesigns; zudem reduziert ein Belassen das Risiko von weiterem
Rework-bedingtem Schaden.

**WiFi als Alternative verworfen:** WiFi teilt sich denselben 2,4-GHz-Funkpfad und dieselbe
RF-Kalibrierung wie BLE und zieht zusätzlich mehr Strom — ein Wechsel auf WiFi würde das
Brownout-Risiko also gleich stark oder stärker reproduzieren. Außerdem widerspricht WiFi der
bestehenden BLE-App-Architektur und NFR-PWR-01 (WiFi bleibt aus).

**Firmware unverändert:** M5 Teil C2 (Telemetrie-Frame, Ringpuffer, BLE-Treiber) bleibt
inhaltlich unverändert, da die Fehlersuche sie als Ursache ausgeschlossen hat und sie
bereits vollständig host-getestet ist. Die BLE-Verifikation am realen Gerät (Advertising,
Verbindungsaufbau, MTU-Verhandlung, Reconnect-Backfill) wird bis zum Eintreffen des neuen
Boards vertagt.

## 7. Wissenschaftliche Einordnung

Der entscheidende diagnostische Schritt dieser Fallstudie ist Test 10 (BOD-Abschaltung):
Er liefert ein sauberes Unterscheidungskriterium zwischen zwei ansonsten schwer
trennbaren Erklärungen für ein wiederholtes „E BOD" — einem **überempfindlichen, falsch
auslösenden Detektor** auf einem für den Betrieb unschädlichen Spannungstransienten
einerseits, und einem **realen Spannungskollaps**, der die Ausführung tatsächlich stört,
andererseits. Beide Erklärungen erzeugen dasselbe oberflächliche Symptom (wiederholtes
„E BOD" und Reset); erst das gezielte Entfernen des Detektors selbst macht sichtbar, ob das
zugrunde liegende System bei fortgesetzter Ausführung tatsächlich versagt (Hängenbleiben,
Watchdog-Reset) oder unauffällig weiterläuft. Damit fungiert die BOD-Abschaltung als
gezielter Kontrollversuch, der eine ansonsten nicht beobachtbare Eigenschaft des Systems
(reale versus scheinbare Instabilität) sichtbar macht.

Ebenso methodisch relevant ist die konsequente Dokumentation aller neun vorangegangenen,
wirkungslosen Gegenmaßnahmen (Tests 1–9): Reproduzierbare Negativergebnisse grenzen den
Hypothesenraum ein und sind damit ein integraler, valider Bestandteil des
Diagnoseprozesses — nicht lediglich Nebenprodukt einer letztlich erfolgreichen Maßnahme.
Erst die Kombination aus vollständiger Ausschlussreihe (Firmware, Lastverteilung,
Zwei-Quellen-Konflikt, Software-Gegenmaßnahmen, Verdrahtung/Versorgungspfad, reine
Pufferung) und dem gezielten Kontrollversuch (BOD-Abschaltung) erlaubt die belastbare
Eingrenzung der Ursache auf eine Hardware-Grenze des konkreten Boards.

## 8. Auflösung: Board-Tausch am realen System validiert

Nach Eintreffen und Einlöten des Ersatzboards (Espressif ESP32-DevKitC-32E,
ESP32-WROOM-32E) wurde die in Kap. 6 getroffene Entscheidung stufenweise am realen
Gerät verifiziert:

1. **Pin-/Logiktest ohne BLE:** Voller Normalbetrieb (alle Sensoren, Schluss-/
   Bremslicht, Blinker/RF, Task-Watchdog) auf dem neuen Board ohne BLE — stabil, kein
   Brownout, alle Boot-Meldungen (`IMU ready=1`, `BMP280 ready=1`, `GNSS ready=1`)
   und laufenden Statusausgaben (`[R1/R2]`, `[Baro]`, `[GNSS]`) wie erwartet.
2. **BLE-Isolationstest:** `NimBLEDevice::init()` als alleinige Hardware-Aktion (bei
   weiterhin aktivem Brownout-Detektor, kein Register-Bypass) — der Log-Marker
   „`[BLE] nach init()`" wird erstmals erreicht, kein „E BOD", Advertising startet.
3. **Vollbetriebstest (Sensoren/Aktoren + BLE gleichzeitig):** identisches Ergebnis
   unter voller Last — `[BLE] nach init()` ohne Brownout, Advertising läuft parallel
   zu `[R1/R2]`/`[Baro]`/`[GNSS]` ohne Beeinträchtigung, kein Reset über die gesamte
   Beobachtungsdauer.
4. **Reale BLE-Verbindung (nRF Connect):** Verbindungsaufbau und MTU-Verhandlung auf
   **185 Byte** (Nutzlast 182 Byte, deutlich über dem für ein 81-Byte-Frame in einer
   Notification benötigten Mindestwert von 84 Byte), Notify-Subscribe erfolgreich.

Bei der ersten Verbindungsprüfung fielen zwei von der Brownout-Ursache unabhängige
Firmware-Bugs im Advertising auf und wurden behoben: (a) `NimBLEDevice::init(name)`
trägt den Gerätenamen nur intern im GAP ein, nicht ins Advertising-Paket — ergänzt um
einen expliziten `pAdvertising->setName()`-Aufruf; (b) der Name überschritt zusammen
mit der 128-Bit-Service-UUID das 31-Byte-Limit des primären Advertising-Pakets
(„Data length exceeded") — behoben durch Auslagerung des Namens in die
Scan-Response-Payload (`enableScanResponse(true)`, eigenes 31-Byte-Budget, Standard-
BLE-Praxis).

**Ergebnis:** Der in Kap. 5 identifizierte Root Cause (Spannungsregler des Altboards)
ist damit bestätigt — auf dem Ersatzboard tritt der Brownout unter keiner der
getesteten Lastkonfigurationen mehr auf. M5 Teil C2 (BLE-Telemetrietransport) gilt als
am realen System validiert.

## Quellen

- Espressif Systems: *ESP32-DevKitC Getting Started Guide* (Board, Pinbelegung, Maße).
- Espressif Systems: *ESP32-WROOM-32E Datasheet*, Version 2.0 (Modul, BLE, elektrische
  Daten).
