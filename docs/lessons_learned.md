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

### iOS/SwiftData: Aufzeichnung nach App-Neustart lückenlos fortsetzen (Zeitstempel-Kontinuität)
**Problem:** Beim „Weiter fahren" einer nach Absturz wiederhergestellten Fahrt
(AR-DATA-04) beginnt die (simulierte) Frame-Uhr nach dem Neustart wieder bei 0.
Neue Punkte kollidierten dadurch mit den bereits persistierten 1-Hz-Zeitstempeln
(t = 0, 1, 2 …) → der idempotente Append (Schutz gegen Doppel-Zeitstempel beim
Backfill) verwarf sie stillschweigend.
**Lösung:** Die relative Punktzeit `t` über einen `tOffset` fortschreiben, der beim
Fortsetzen auf das letzte gespeicherte `t` gesetzt wird; die erste neue Frame-Zeit
wird zur neuen Basis. Fortgesetzte Punkte laufen so lückenlos hinter den bestehenden
weiter — unabhängig von der nach Neustart zurückgesetzten Geräte-/Mock-Uhr.
**Nebenbefund (SwiftUI):** `.alert` fügt automatisch einen „Cancel"-Button ein,
solange kein Button die `.cancel`-Rolle trägt. Für eine exakt definierte Aktionsmenge
(hier: Abschließen / Verwerfen / Weiter fahren) einem Button bewusst die `.cancel`-
Rolle geben (hier „Weiter fahren"), dann bleibt es bei genau diesen Aktionen.

### App/Aufzeichnung: Der Zeitstempel einer entfernten (Firmware-)Quelle ist keine verlässliche Zeitbasis
**Problem:** Wurde das Rücklicht **mitten in einer laufenden Aufzeichnung** kurz
stromlos geschaltet, sprang nach dem Reconnect die Fahrtdauer auf **1191:44:03**
(≈ 4,29 Mrd. ms) und die Distanz auf **~784 km**; zusätzlich zerlegte es das
Höhenprofil (senkrechte Artefakte, rücklaufende Distanz-Achse). Am realen Gerät
reproduziert.
**Ursache:** Die App leitete Dauer/Distanz aus dem Geräte-Zeitstempel
`timestamp_ms` (uint32 = `millis()` seit Boot) ab. Ein Stromausfall **startet den
ESP32 neu → `millis()` fällt auf ~0 zurück**. Der neue Zeitstempel ist kleiner als
der letzte; die als **vorzeichenlose 32-Bit-Differenz** gerechnete verstrichene Zeit
läuft unter (`neu − alt mod 2^32` ≈ 4,29·10⁹ ms = 1191:44:03). Genau dieser eine
riesige Zeitschritt fließt in die Distanz-Integration (Σ v·dt) → ~784 km, und über
die daraus kumulierte, nicht-monotone Distanz-Achse bricht auch die Höhenprofil-
Grafik.
**Lösung:** Eine reine, host-getestete **`RecordingClock`** (`SmartBikeCore`) führt
eine **monotone** Aufzeichnungszeit: sie erkennt einen Reset (`rawMs < last`) und
re-basiert über einen fortlaufenden Offset, **kappt jeden Schritt auf
`maxSampleDt = 1,5 s`** und verwirft Duplikate. Die Sample-Zeit `t` ist damit immer
monoton und pro Frame ≤ 1,5 s; Dauer (`last.t − first.t`) und Distanz (Σ v·dt)
werden dadurch **automatisch robust**, ohne die `StatisticsEngine` anzufassen
(Fix an der Quelle). 7 neue Swift-Testing-Tests (Reset, Lücke, Duplikat, Backfill,
Monotonie über Reset-Sequenz).
**Erkenntnis:** In einem verteilten System (App + eingebettetes Gerät) darf die Uhr
der *entfernten* Quelle **nicht** als monotone Zeitbasis behandelt werden. Zwei
generische Konsequenzen: (1) vorzeichenlose Zeit-Differenzen gegen Unterlauf
absichern; (2) numerische Integration gegen Ausreißer-`dt` **kappen** — die Kappung
unterschätzt einen langen Ausfall bewusst leicht (sichere Richtung) statt zu
explodieren. Wichtige Unterscheidung: **Funkabriss** (Gerät bleibt an → Backfill aus
dem RAM-Ringpuffer möglich) vs. **Stromausfall** (RAM-Puffer verloren → Lücke
endgültig). Vorher/Nachher am Gerät belegbar (Screenshot 1191:44:03/784 km vs.
korrekte Werte) — geeignet als Validierungs-Abbildung.
### Ein grüner Testlauf beweist nur das, was die Testeingabe abdeckt (Mangel M-01)
**Problem:** Die in FR-TL-06 geforderte Mindesthaltezeit von 300 ms war im
Fahrbetrieb in **allen 14** aufgezeichneten Aktivierungen wirkungslos — obwohl
drei eigens dafür geschriebene Host-Tests grün liefen und die Funktion am
Serial-Bench als validiert galt.
**Ursache:** Der `else`-Zweig der Bremslicht-Zustandsmaschine lief bereits ab
`BRAKE_OFF_MS2` = 1,5 m/s² und überschrieb den für die Haltezeit eingefrorenen
Wert mit `brakeDutyPercent()` — und diese Funktion gibt unterhalb der
Einschaltschwelle von 2,0 m/s² den reinen Schlusslichtwert zurück. Der
„gehaltene" Wert war damit identisch mit dem Wert, gegen den er halten sollte.
Jedes reale Bremssignal durchläuft das Band 1,5…2,0 beim Abklingen.
**Warum die Tests es nicht fanden:** Die drei Haltezeit-Tests speisen
idealisierte Sprünge (5,0 → 0,0) ein und überspringen das Hystereseband
vollständig. Ein vierter Test fährt den Wert 2,0 zwar an, prüft danach aber nur
den *Zustand*, nicht die ausgegebene *Duty*. Die Tests bildeten damit exakt den
Signalverlauf ab, den der Entwickler beim Schreiben der Logik im Kopf hatte —
nicht den, den die Physik erzeugt.
**Erkenntnis:** Grüne Host-Tests belegen die Übereinstimmung der Logik mit den
*modellierten* Eingangssignalen, nicht mit den realen. Der Defekt entstand nicht
durch fehlende Abdeckung (die Zeilen waren abgedeckt), sondern durch eine
Testeingabe, die den kritischen Betriebsbereich systematisch ausließ. Für
Zustandsmaschinen mit Hysterese heißt das konkret: **mindestens ein Testfall muss
das Hystereseband monoton durchlaufen**, nicht darüber hinwegspringen. Gefunden
wurde der Fehler erst durch den Vergleich der aufgezeichneten Ausgangsgröße mit
einer Nachbildung der Zustandsmaschine, gespeist mit den realen Felddaten
(Übereinstimmung 98,53 %) — also durch eine Messung, nicht durch einen Test.

### Diagnoseausgaben innerhalb eines Messfensters verfälschen die gemessene Größe
**Problem:** Die Worst-Case-Schleifenzeit im Fahrbetrieb lag bei 6,7 ms gegenüber
0,651 ms am Prüfstand. Die Spitzen traten exakt periodisch mit 1,00 s auf und
wurden deshalb dem 1-Hz-GNSS-Slot zugeschrieben.
**Ursache der Fehlzuordnung:** Im Aufzeichnungsstand liefen **drei**
Debug-`printf` mit ebenfalls exakt 1 Hz — und alle drei lagen *innerhalb* des
Zeitfensters, aus dem `loop_max_us` gebildet wird. Zusammen rund 173 Byte; bei
115 200 Bd sind das etwa 87 µs je Byte, und sobald der UART-Sendepuffer voll ist,
blockiert `Serial.printf`. Zwei solcher Ausgaben in einem `loop()`-Durchlauf
liegen in derselben Größenordnung wie der gemessene Ausreißer.
**Erkenntnis:** Zwei Vorgänge mit identischer Periode lassen sich aus einer
Zeitreihe grundsätzlich nicht auseinanderhalten — die Zuordnung war eine
plausible Vermutung, kein Befund. Allgemeiner: Eine Messinstrumentierung, die im
Messfenster der Größe liegt, die sie beobachten soll, ist Teil des Messobjekts.
Das ist ein Beobachtereffekt, der in den Daten selbst nicht sichtbar wird. Zwei
Konsequenzen: (1) Instrumentierung entweder aus dem Messfenster herausnehmen oder
ihren Beitrag getrennt beziffern; (2) bei periodischen Störungen immer prüfen, ob
mehr als ein Vorgang dieselbe Periode hat. Der einzige belastbare Weg zur
Trennung wäre ein Kontrollversuch mit und ohne Instrumentierung — dieselbe
Vorgehensweise, die in der BLE-Brownout-Fallstudie zum Ziel führte.

### Mechanische Eigenschaften gehören an die Hardware-Abstraktionsgrenze
**Problem:** Die Lochrasterplatine musste um 180° in ihrer eigenen Ebene gedreht
eingebaut werden. Damit kehren sich a_x, a_y, ω_x und ω_y um; die Bremserkennung
hätte sich invertiert — Bremsen erzeugt kein Bremslicht, Beschleunigen dagegen
schon. Am Schreibtisch wäre das nicht aufgefallen, weil Boot, I²C-Scan,
Telemetrie und Regime-Klassifikation unverändert korrekt erscheinen.
**Lösung:** Die Einbaulage wird als Rotation an der Treibergrenze zurückgerechnet
(`lib/logic/imu_mount_orientation.h`, aufgerufen in `imuRead()`), nicht als
zusätzliches Vorzeichen in der Auswertelogik. Dadurch blieb die gesamte
nachgelagerte Kette unverändert gültig: 126 Host-Tests, der Golden-Vektor, das
Telemetrie-Frame und die komplette App-Seite mussten nicht angefasst werden.
**Zwei Fallstricke, die dabei sichtbar wurden:** (1) Accelerometer **und**
Gyroskop müssen dieselbe Transformation erfahren. Würde nur a_y negiert, gälte
dθ/dt = ω_x nicht mehr und der Komplementärfilter integrierte die Nickschätzung
in die falsche Richtung — unsichtbar im Stand, weil ω_x dort null ist, und erst
während echter Fahrt wirksam. Formal: Die Transformation muss eine echte Rotation
mit Determinante +1 sein, damit das Koordinatensystem rechtshändig bleibt.
(2) Das bereits am Board verifizierte `MOTION_BRAKE_SIGN` darf **nicht**
zusätzlich umgekehrt werden. Die Verifikation erfolgte an den Rohdaten vor dem
Umbau; die Transformation stellt genau diese Konvention wieder her. Ein zweites
Vorzeichen wäre eine Doppelanwendung.
**Erkenntnis:** Eine Eigenschaft des mechanischen Aufbaus ist keine Eigenschaft
des Auswerteverfahrens. Wird sie dort korrigiert, wo sie entsteht — am Übergang
Hardware → Logik —, bleibt alles Nachgelagerte unverändert validiert. Wird sie
weiter hinten korrigiert, repariert man einen Pfad und lässt die übrigen
gespiegelt zurück.
