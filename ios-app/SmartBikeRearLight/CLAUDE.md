# CLAUDE.md — iOS-App-Kontext (SmartBikeRearLight)

Diese Datei ist der Einstiegspunkt für die iOS-Toolchain (Claude in Xcode)
innerhalb des gemeinsamen Repos `smart-bike-rearlight`. Ein Repo, eine
Historie — die Firmware-Toolchain (Claude Code/VS Code) pflegt
`../../CLAUDE.md` und `../../docs/`.

## Verbindlicher BLE-Schnittstellenvertrag

**Einzige gemeinsame Wahrheitsquelle** für Firmware und App:
[`../../docs/BLE_Frame_v3_Schnittstelle.md`](../../docs/BLE_Frame_v3_Schnittstelle.md).
Bei jedem Widerspruch zwischen App-Code, Kommentaren hier oder der App
Bible gilt dieser Vertrag. Abweichungen sind Fehler, auch wenn sie
„funktionieren".

**Eckdaten (Schema v3, Stand 07.08.2026):**

| Eigenschaft | Wert |
|---|---|
| Framelänge | **113 Byte** |
| Schema-Version (Offset 0) | **3** (App liest `version >= 3` **und** Länge `>= 113` für v3-Felder; `version >= 2` **und** Länge `>= 81` reicht für die v2-Felder — keine Gleichheitsprüfung, s. Vertrag Kap. 4) |
| Erforderliche MTU | **≥ 116** (Frame + 3 Byte ATT-Overhead); ausgehandelt werden 185 |
| Service-UUID | `587bb505-9f9d-4ae0-96fd-0b29adfc4b03` |
| Characteristic-UUID (NOTIFY) | `8c604d09-743f-4850-9109-19604a17f358` |
| Richtung | unidirektional ESP32 → App, kein Write |
| Byte-Reihenfolge | Little-Endian, gepackt, keine Alignment-Annahmen (`withUnsafeBytes` + `loadUnaligned`, kein Pointer-Cast) |
| Senderate | 10 Hz |

Referenz-Bytefolge für den Schnittstellen-Kreuztest (Firmware und App
gegen dieselbe eingefrorene Bytefolge, nicht nur gegen die jeweils eigene
Symmetrie Encoder↔Decoder):
[`../../testdata/frame_v3_golden.hex`](../../testdata/frame_v3_golden.hex) +
[`../../testdata/frame_v3_golden.md`](../../testdata/frame_v3_golden.md)
(Wertetabelle: Feld, Offset, Typ, gesetzter Wert).

## Nicht verhandelbare Randbedingungen

- **`SmartBikeCore` bleibt UI-frei:** kein SwiftUI, kein CoreBluetooth,
  kein SwiftData im Package. Nur Foundation-Werttypen/reine Logik.
- **Reine Datensenke, unidirektional** (FR-SYS-04): kein Write zum Gerät.
- **Keine modalen Alerts während der Fahrt** (AR-UX-01). Diagnosegrößen
  dürfen das Cockpit nicht überladen und kein Scrollen erzwingen.
- **Alles lokal, kein Netzwerk.**

## Weiterführend

- App Bible (SSOT der App-Architektur/Anforderungen): `../../docs/app_bible.md`.
- Umsetzungsplan Schema v3 (paketweise Abarbeitung AP0–AP8):
  `../../docs/Umsetzungsplan_Schema_v3_iOS.md`.
