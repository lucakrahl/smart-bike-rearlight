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
