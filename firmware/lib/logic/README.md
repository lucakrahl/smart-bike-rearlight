# lib/logic — reine, hardwarefreie Logik (NFR-TST-01)

Hier liegt ausschließlich **hardwareunabhängige** Logik: Zustandsmaschinen,
Bremskennlinie (FR-TL-06), RF-Tastenerkennung (FR-RF-03/04), Plausibilitäts-
prüfung (FR-SNS-05), Telemetrie-Serialisierung (FR-TEL).

**Regeln:**
- **Kein `#include <Arduino.h>`**, keine direkten Hardwarezugriffe.
- Eingaben kommen als einfache Werte/Strukturen herein, Ergebnisse gehen als
  Werte heraus. Zeit wird als Parameter (`now_ms`) übergeben, nicht via `millis()`.
- Jedes Modul bekommt einen Host-Unit-Test unter `firmware/test/` (`pio test -e native`).

Beispiel bereits vorhanden: `brake_curve.{h,cpp}` mit Test
`test/test_logic/test_brake_curve.cpp`.
