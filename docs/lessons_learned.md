# Lessons Learned

> Von Claude Code pflegbar. Kurze Einträge: Problem → Ursache → Lösung/Erkenntnis.
> Dient als Wissensspeicher für die Thesis (z. B. Kapitel „Herausforderungen").

### macOS: `liblzma`-Fehler beim Toolchain-Build (pioarduino)
**Problem:** PlatformIO bricht beim Laden/Entpacken der pioarduino-Toolchain mit
„Python's lzma module is unavailable/broken" ab.
**Ursache:** pioarduino-Toolchains sind `.tar.xz`-gepackt; PlatformIOs Python
braucht dafür `liblzma`, das auf macOS (insb. Apple Silicon) oft fehlt.
**Lösung:** Homebrew installieren, dann `brew install xz` (liefert
`/opt/homebrew/opt/xz/lib/liblzma.5.dylib`). Danach baut die Toolchain durch.

### Akkubetrieb-Freeze bei angestecktem, aber host-seitig getrenntem USB-Kabel
**Problem:** Bremslicht friert ein (bleibt auf einem Wert stehen), wenn das
USB-Kabel im ESP32 steckt, aber am Host-Ende getrennt ist.
**Ursache:** Floatende VBUS-Leitung → unruhige 3,3-V-Schiene → I²C-/IMU-
Aussetzer. Blinker (eigener Task, nicht von der IMU abhängig) bleibt
unbeeinträchtigt — das Symptom war also regionsspezifisch, nicht global.
**Erkenntnis:** Im echten Akkubetrieb (Kabel komplett ab) nicht reproduzierbar.
„Debug-Setup ≠ Feldbedingung — Validierung stets im realen Betriebszustand."
(s. auch Project Bible Kap. 9.2.)

### ESP32-Core: `Wire.end()` scheitert bei blockiertem I²C-Bus still (No-Recovery-Falle)
**Problem:** Im SDA-Kurzschluss-Fehlerinjektionstest wirkte die I²C-Recovery
(Soft-Reinit, SCL-Clock-Release) zunächst nicht — Log zeigte wiederholt
„Bus already started in Master Mode".
**Ursache:** Im installierten Core (`Wire.cpp`/`esp32-hal-i2c-ng.c`)
verifiziert: `Wire.end()` → `i2cDeinit()` → ESP-IDFs
`i2c_del_master_bus()`; nur bei Erfolg wird der Bus als deinitialisiert
markiert und die Pins per `perimanClearPinBus()` freigegeben. Bei
elektrisch blockiertem Bus kann `i2c_del_master_bus()` fehlschlagen — der
nächste `Wire.begin()` sieht den Bus fälschlich als „schon gestartet" und
wird zum wirkungslosen No-op. Zusätzlich bleiben die Pins dabei
PeriMan-seitig I²C-besessen, wodurch `pinMode()`/`digitalWrite()` in der
SCL-Release-Routine ebenfalls kommentarlos nichts bewirken (PeriMan ruft
vor der Neuzuweisung selbst den — am gleichen Fehler scheiternden —
I²C-Deinit-Callback auf).
**Lösung:** Bus-Recovery mit rohen ESP-IDF-`gpio_*`-Calls (umgehen PeriMan
vollständig) **vor** dem `Wire.end()`-Versuch physisch freimachen; danach
gelingt die eigentliche Deinitialisierung zuverlässig.

### Systematische Hardware-vs-Firmware-Eingrenzung: der BOD-Abschalt-Test als Unterscheidungskriterium
**Problem:** BLE-Start (M5 Teil C2) löste reproduzierbar den Brownout-Detektor
aus (Bootloop bei `NimBLEDevice::init()`). Unklar war lange, ob es sich um
eine Fehlauslösung des Detektors auf einem harmlosen Transienten handelt
oder um einen echten Spannungskollaps — beide Erklärungen erzeugen dasselbe
Symptom (wiederholtes „E BOD" + Reset).
**Vorgehen:** Erst messen, dann schließen — Firmware wurde durch Host-Tests
(75/75) und Bracket-Logging als Ursache ausgeschlossen, bevor an der
Hardware getestet wurde; jede Gegenmaßnahme (Sendeleistung, CPU-Takt,
Kondensatoren an zwei Schienen, geänderte Spannungsquelle) wurde einzeln
getestet und das Ergebnis dokumentiert, auch wenn wirkungslos. Reproduzierbare
Negativergebnisse waren dabei genauso wertvoll wie ein positiver Befund, weil
sie den Hypothesenraum eingrenzen.
**Entscheidendes Unterscheidungskriterium:** Die testweise Deaktivierung des
Brownout-Detektors selbst. Eine reine Fehlauslösung hätte nach dem Wegfall
des Detektors zu unauffälligem Weiterlaufen führen müssen; stattdessen hing
sich das Board an derselben Stelle auf und wurde erst später vom
RTC-/Task-Watchdog zurückgesetzt — ein Beweis für einen echten
Spannungskollaps, nicht für eine überempfindliche Detektion.
**Erkenntnis:** Wenn zwei Erklärungen (Fehlauslösung vs. reales Versagen)
dasselbe Symptom erzeugen, hilft kein weiteres Beobachten des Symptoms
selbst — sondern ein gezielter Kontrollversuch, der die scheinbar
schützende Instanz (hier: den Detektor) selbst entfernt, um zu sehen, ob
das dahinterliegende System tatsächlich versagt. Details:
`docs/ble_brownout_fallstudie.md`.

### Host-Tests allein finden keine Fehlerfall-Bugs am realen Bus
**Problem:** Alle 61 Host-Unit-Tests liefen grün, trotzdem zeigte der reale
SDA-Kurzschluss-Fehlerinjektionstest zwei Bugs (Fehl-Bremslicht durch ein
einzelnes Müll-Sample; wirkungslose I²C-Recovery), die kein Test vorher
gefangen hatte.
**Erkenntnis:** Host-Tests prüfen nur die modellierte Fehlerannahme mit
synthetischen Eingaben, nicht das tatsächliche Verhalten der Hardware-/
Treiberschicht unter einem physischen Fehler. „Debug-Setup ≠ Feldbedingung"
gilt also nicht nur für die Stromversorgung (s. o.), sondern ebenso für
Bus-Fehlerfälle — echte Fehlerinjektion am Gerät bleibt für
sicherheitskritische Pfade unverzichtbar.
