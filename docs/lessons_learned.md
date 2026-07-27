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
