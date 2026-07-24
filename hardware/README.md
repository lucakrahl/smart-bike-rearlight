# hardware/

Schaltplan, Stückliste (BOM) und Hardware-Doku.

**Zu übernehmen / zu korrigieren (Bible Kap. 11.2):**
- Schaltplan v2 korrigieren: RF GPIO34→**GPIO4**; **GPIO25↔GPIO26** tauschen;
  3× 10-kΩ-Gate-Pull-Down ergänzen; Einschalter **SW1** ergänzen;
  Entkopplungskondensatoren explizit darstellen.
- BOM ergänzen: 10-kΩ-Pull-Down (3×), Drucktaster IP65 8 mm, Akku **LP103454**.
- RF-Empfänger einheitlich als **SRX882S** benennen (nicht „PT2262").

Pinbelegung: siehe `firmware/include/pins.h` und Bible Kap. 4.2.
