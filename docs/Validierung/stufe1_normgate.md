# Stufe 1 — Normbetrags-Gate (`motion_filter`)

> Kurzbericht zur Neufassung von `motion_filter` nach dem Feldtest vom
> 06.08.2026 (r=-0,132, n=939 — s. `Feldtest_060826_Auswertung.md`). Dieser
> Abschnitt behandelt einen einzelnen, quantifizierten Teilbefund
> ("Totzone des STATIC-Bands"); der vollständige Bericht (Verfahren,
> Schwellwerte, Testabdeckung, Experimente D/E/F) folgt in einem separaten
> Schritt.

## Totzone des STATIC-Bands

Jede Bremsung muss beim Anstieg den Bereich `0 .. 1,539 m/s²` (=
`MOTION_NORM_STATIC_BAND=0,12` in die Pythagoras-Umrechnung
`a = sqrt((g+delta)² - g²)` eingesetzt) durchlaufen und wird dabei noch als
`STATIC` klassifiziert — das Regime-Gate selbst kann eine Bremsung erst
erkennen, sobald ‖a‖ das Band verlässt. Bei realistischen Anstiegsraten
(150–250 ms bis zur vollen Bremsverzögerung) nimmt das schnelle
STATIC-Filter (`MOTION_COMPL_TAU_S`) in dieser kurzen Phase bereits einen
falschen Pitch auf, weil `atan2f(ay, az)` die anliegende Bremsbeschleunigung
zu diesem Zeitpunkt noch nicht von echter Neigung unterscheiden kann. Dieser
Fehlanteil bleibt danach als Offset im Ausgang bestehen, auch wenn das
Regime korrekt auf `DYNAMIC` wechselt.

Das ist ein **quantifizierter Restanteil von Fehlermechanismus A**
("Scheinneigung", s. Feldtest-Befund) — die Regime-Klassifikation
verhindert die *unbegrenzte* Kontamination aus dem ursprünglichen Bug,
beseitigt aber nicht die kurze, strukturell unvermeidbare Anlaufphase
selbst. Stufe 2 (GNSS-Bias-Stützung, `tau_b=10 s`, s. Teil B der
Auftragsplanung) reduziert diesen Restanteil weiter, indem sie eine von der
IMU unabhängige Referenz für genau diese Anlaufphase liefert.

### Zahlenbeispiel T11 (Rampe 0→2,2 m/s² in 300 ms, danach 3 s halten)

`MOTION_COMPL_TAU_S` bestimmt, wie stark der falsche Pitch während der
~200 ms im STATIC-Band aufgebaut wird, bevor bei ‖a‖-g>0,12 auf `DYNAMIC`
(mit `MOTION_COMPL_TAU_SLOW_S`/`_UNCAL_S`) gewechselt wird:

| Größe | vorher (TAU_S=0,49 s) | nachher (TAU_S=3,0 s) |
|---|---|---|
| Ausgang am Ende der Rampe (t=300 ms) | 1,846 m/s² | 2,070 m/s² |
| Verlust ggü. Sollwert (2,2 m/s²) an dieser Stelle | ≈0,354 m/s² | ≈0,130 m/s² |
| Verlust nach Abklingen der Glättung (Spitzenwert kurz danach) | — | ≈0,054–0,060 m/s² |
| Minimum über die volle 3-s-Haltephase | 1,745 m/s² | 2,070 m/s² |
| Erreicht `BRAKE_ON_MS2=2,0`? | **Nein** (max. 1,919 m/s²) | **Ja**, durchgehend (T11/T16 grün) |

Mit dem alten `TAU_S=0,49 s` blieb der Filterausgang für dieses Profil
durchgehend unter der Ansprechschwelle — `tail_light_fsm` wäre nie in
`Brakelight` eingetreten (s. T16, volle Kette). Mit `TAU_S=3,0 s` sinkt der
STATIC-Phasen-Anteil des Verlusts von ≈0,35 auf ≈0,05–0,13 m/s² (je nach
Messpunkt, s. Tabelle), der Ausgang bleibt durchgehend über der Schwelle.

Werte per Host-Simulation ermittelt (nicht am realen Board), Herleitung
und Gegenrechnung s. Konversation zu T11/T8/T14 (Korrekturrunde
"T11 — Entscheidung und Nachbesserung").

## Voraussetzung der Verankerung

**Annahme, keine gesicherte Aussage:** Die Verankerung und die
Stufe-1-Bias-Kalibrierung (s. `MOTION_ANCHOR_WINDOW_S`) benötigen nach dem
Einschalten mindestens 1,0 s zusammenhängendes `STATIC`. Wird das nicht
erreicht (z. B. wenn das Rad beim Einschalten bereits bewegt oder erschüttert
wird), greift nach `MOTION_ANCHOR_TIMEOUT_S=2,0 s` der Fallback (ebene Lage
annehmen, Bias bleibt unkalibriert) — das System läuft dann dauerhaft im
konservativen Rückfall mit `MOTION_COMPL_TAU_SLOW_UNCAL_S=30 s` statt der
kalibrierten `MOTION_COMPL_TAU_SLOW_S=90 s`, mit entsprechend höherer
Restdämpfung anhaltender Bremsungen (s. Bench-Nachweis in
`bench_run_notes.md`, Schritt A: `bias_calibrated=0` bei Profilen ohne
ausreichende STATIC-Vorlaufzeit).

Im normalen Gebrauch ist die Bedingung erfüllt, weil das Rad beim
Einschalten typischerweise steht — das ist jedoch eine **Annahme und keine
gesicherte Aussage**. Sie wird in der realen Messfahrt überprüft, nicht
durch synthetische Bench-Profile ersetzt.
