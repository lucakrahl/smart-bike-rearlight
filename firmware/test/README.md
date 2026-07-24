# firmware/test — Host-Unit-Tests (NFR-TST-03)

Tests der reinen Logik, ausgeführt auf dem PC (kein ESP32 nötig):

```bash
pio test -e native
```

Konvention: pro Logik-Modul eine Testdatei unter `test_logic/`. Aufgezeichnete
Sensordaten für datengetriebene Tests (NFR-TST-02) liegen in `../../testdata/`.
On-Target-Tests (Hardware) laufen separat gemäß Validierungsplan (Bible Kap. 9).
