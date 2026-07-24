# Smart Bike Rear Light

Prototyp eines intelligenten IoT-Fahrrad-Rücklichts (Bachelorarbeit, Maschinenbau
& Produktentwicklung B.Eng.). ESP32-Firmware mit IMU-gesteuertem Schluss-/Brems-
licht, 433-MHz-Funk-Blinkern und BLE-Telemetrie an eine Web-App.

## Struktur (Monorepo)
| Ordner | Inhalt |
|---|---|
| `firmware/` | PlatformIO-Projekt (ESP32, Arduino-Framework) |
| `webapp/` | PWA / Web-Bluetooth-App (später) |
| `docs/` | **Project Bible** (SSOT) + Wissensdatenbank |
| `hardware/` | Schaltplan, BOM |
| `cad/` | Gehäuse (später) |
| `testdata/` | Sensordaten für Host-Logik-Tests |

## Quickstart
```bash
# Abhängigkeiten holen & Firmware bauen
cd firmware
pio run -e esp32dev

# Flashen & Serial-Monitor
pio run -e esp32dev -t upload
pio device monitor -b 115200

# Host-Unit-Tests der Logik (ohne ESP32)
pio test -e native
```

## Arbeiten mit Claude Code
`CLAUDE.md` im Repo-Root ist der Projektkontext (wird automatisch geladen).
**Zuerst `docs/project_bible.md` lesen** — das ist die Single Source of Truth.
Implementiert wird gegen die SRS (Bible Kap. 2); jede Umsetzung referenziert die
Anforderungs-ID(s). Reihenfolge der Module: `docs/roadmap.md`.

## Voraussetzungen
VS Code · PlatformIO IDE · Claude Code · Git. ESP32 NodeMCU DevKit C V2
(CP2102-Treiber). Board `esp32dev`, Partition `huge_app` (No-OTA), 115200 Baud.

## Wichtiger Hinweis
Das Notbrems-Blinken (FR-TL-07) ist **standardmäßig deaktiviert** — ein blinkendes
rotes Rücklicht ist nach § 67 Abs. 4 StVZO unzulässig (nur Experimental-/Messzweck).
