# Wiederholungs-Messfahrt 08.08.2026 — Auswertung

**Smart Bike Rear Light · Bachelorarbeit Krahl**
**Datengrundlage:** `SmartBikeRearLightFahrt202608082245.csv` (Frame-Schema v3, 10 Hz)
**Stand der Auswertung:** 09.08.2026
**Einbaulage:** unverändert gegenüber dem Feldtest 06.08.2026 (vor der Drehung der Lochrasterplatine)

> **Einordnung.** Dieses Dokument wertet die erste Fahrt nach der Stufe-1-Überarbeitung
> des Bewegungsfilters aus. Es prüft, ob der am Prüfstand nachgewiesene Effekt
> (Normbetrags-Gate gegen Fehlermechanismus A) im Fahrbetrieb trägt, und dokumentiert
> drei bis dahin unbekannte Befunde, die erst durch die v3-Telemetrie sichtbar geworden
> sind. Verbindlicher Projektstand bleibt die Project Bible; dieses Dokument liefert den
> Ergebnisteil.

---

## 1. Zusammenfassung der Befunde

| # | Befund | Bewertung |
|---|---|---|
| B1 | Die Bremserkennung funktioniert. Alle **9** aus der GNSS-Referenz identifizierten Bremsvorgänge wurden angezeigt; von 11 Anzeigevorgängen lassen sich 10 einer realen Verzögerung zuordnen. | Kernergebnis, **gesichert** |
| B2 | Bei identischer Auswertung und identischem Zeitversatz (−2,0 s) beträgt die Korrelation zwischen GNSS-Referenz und `brake_decel_ms2` am 08.08. **r = +0,85**, an den Vergleichsfahrten vom 06.08. im Median **+0,15**. | Kernergebnis, **gesichert** |
| B3 | Der Ruhesockel von ~3,0 m/s² aus dem Vorzustand ist im Stillstand nicht mehr nachweisbar (Median 0,00 m/s², P99 0,08, kein Bremslicht) — belegt über 11,8 s Stillstand. | **gesichert**, schmale Datenbasis |
| B4 | Keine Fehlauslösung in Beschleunigungsphasen (0 von 25 Epochen mit ≥ 1,0 m/s² Beschleunigung, robust über alle geprüften Fensterbreiten). | **gesichert** |
| B5 | **Neu:** Die in FR-TL-06 geforderte Mindesthaltezeit von 300 ms ist im Fahrbetrieb in **keinem** der 14 Fälle wirksam geworden. Ursache im Quelltext lokalisiert, durch Simulation reproduziert, Testlücke benannt. | **Mangel**, gesichert — **behoben am 10.08.2026, s. Nachtrag** |
| B6 | **Neu:** `brake_decel_ms2` trägt eine geschwindigkeitsabhängige Grundlinie (r = +0,80; Partialkorrelation unter Kontrolle der Fahrbahnneigung +0,75). Bei 25–35 km/h verbleiben im 99-%-Quantil nur **0,32 m/s²** Reserve zur Ansprechschwelle. | **Grenze**, gesichert |
| B7 | **Neu:** Die Worst-Case-Schleifenzeit liegt im Fahrbetrieb bei **6,7 ms** statt der am Prüfstand gemessenen 0,651 ms. NFR-RT-04 (< 10 ms) bleibt erfüllt, die Reserve beträgt 33 % statt scheinbar 93 %. | Messwert **gesichert**; die ursprüngliche Ursachenzuordnung zum GNSS-Slot ist **zurückgenommen**, s. Nachtrag |
| B8 | Die Aufzeichnung ist formal einwandfrei: 35 Spalten nach Vertrag, 1773 von 1776 Frames, kein Fenster mit fehlenden IMU-Abtastungen, keine Bereichsverletzung. | **gesichert** |
| B9 | **Methodisch:** Die am 06.08. berichtete Korrelation r = −0,132 wurde ohne Berücksichtigung der Latenz der GNSS-Referenzkette gerechnet und ist als Gütemaß nicht belastbar. | **Korrektur der Erstauswertung** |

---

## 2. Versuchsbeschreibung

### 2.1 Randbedingungen

| Größe | Wert |
|---|---|
| Datum, Uhrzeit | 08.08.2026, 22:45:04 – 22:48:03 (Ortszeit) |
| Dauer | 177,86 s |
| Distanz (App, GNSS-integriert) | 1,139 km |
| Höchstgeschwindigkeit | 44,4 km/h |
| mittlere Geschwindigkeit (fahrend, v > 1 km/h) | 24,7 km/h |
| Stillstandsanteil (v < 1 km/h) | 6,7 % der Abtastungen (118 Zeilen, 11,8 s) |
| Satelliten | 6 … 10 (Median 10) |
| HDOP | 0,9 … 1,8 (Median 1,0) |
| Höhenspanne (barometrisch) | 3,9 m |
| Aufzeichnungsrate | 10 Hz (Validierungsmodus) |
| Frame-Version | durchgängig 3, `frame_version_gemischt = nein` |

**Zur Höhe.** Die barometrische Höhe spannt 3,9 m, die GNSS-Höhe dagegen 20,6 m. Die
Abweichung ist erwartungsgemäß — die vertikale GNSS-Komponente ist die schwächste
Größe einer Navigationslösung, weshalb `gnss_altitude_m` laut Schnittstellenvertrag
ohnehin nur als Rückfallebene geführt wird. Für alle Neigungsbetrachtungen wird die
barometrische Höhe verwendet.

**Zur Fahrbahnneigung.** Aus der barometrischen Höhe über gleitende 10-s-Fenster ergeben
sich lokale Steigungen zwischen −8,3 % und +6,7 %, entsprechend Längsanteilen der
Schwerkraft von −0,81 bis +0,65 m/s² (Median des Betrags 0,06, 95-%-Quantil 0,28 m/s²).
Die Extremwerte sind allerdings überwiegend Rauschen der barometrischen Höhenschätzung
und nicht als reale Rampen zu lesen. Neigung ist damit **keine** vernachlässigbare
Größe, wie eine Betrachtung der Gesamt-Höhenspanne nahelegen würde; ihr Beitrag wird in
Abschnitt 8 gesondert und quantitativ gegen die Grundlinie geprüft.

### 2.2 Zuordnung zu den geplanten Manövern

Die Fahrt enthält **neun** aus der GNSS-Referenz eindeutig identifizierbare
Bremsvorgänge (zusammenhängende Epochen mit einer Referenzverzögerung ≥ 1,5 m/s²).
Acht davon setzen bei 23,2 … 28,0 km/h ein, einer bei 44,4 km/h — gemessen an der
GNSS-Epoche unmittelbar vor dem Ereignis. Das deckt sich mit Teil B des Messprotokolls
(Manöver 1–3, je dreimal, aus ca. 25 km/h).

| Gruppe | Anzeigevorgang (t_s) | `brake_decel_ms2` Spitze | erreichte Duty |
|---|---|---|---|
| vermutlich „sanft" | 27,9 · 37,0 · 45,2 | 3,07 · 3,50 · 4,68 | 48 % · 60 % · 92 % |
| vermutlich „mittel" | 56,5 · 66,9 · 79,6 | 5,59 · 5,69 · 7,43 | 100 % · 100 % · 100 % |
| vermutlich „kräftig" | 91,6 · 125,4 · 161,6 | 8,31 · 6,39 · 4,86 | 100 % · 100 % · 96 % |

**Einschränkung [Annahme].** Die Zuordnung ist aus den Daten rekonstruiert, nicht
protokolliert. Ohne die im Messprotokoll vorgesehenen sekundengenauen Zeitmarken lässt
sich nicht belegen, welcher Ausschlag zu welchem geplanten Manöver gehört. Für die
Bewertung der Bremserkennung ist das unerheblich — die Referenz kommt aus dem GNSS,
nicht aus dem Protokoll —, für die Aussage „drei Intensitätsstufen sind unterscheidbar"
dagegen schon.

**Nicht belegbar:** Die Manöver 5 (Bordstein), 6 (bergauf) und 7 (bergab) sind in der
Aufzeichnung nicht als abgegrenzte Ereignisse identifizierbar. Manöver 4 (Sprint) und 8
(Stillstand) sind dagegen datenseitig eindeutig auswertbar (Abschnitt 6).

---

## 3. Datenintegrität und Formatkonformität

| Prüfung | Ergebnis |
|---|---|
| Kodierung UTF-8 mit BOM, CRLF, Semikolon, Dezimalkomma | erfüllt |
| Präambel vollständig, `schema_version = 3` | erfüllt |
| Header byte-identisch zur eingefrorenen 35-Spalten-Spezifikation | erfüllt |
| Wertebereiche (`duty` 0–100, `brake_decel ≥ 0`, `dt_max ≤ 255`, `loop_max < 65535`) | keine Verletzung |
| `gnss_accel_ms2 = 0`, wenn `gnss_accel_valid = 0` | erfüllt (218 Zeilen) |
| Gerätezeitstempel monoton | erfüllt |
| Summe der Regime-Zähler je Fenster | **1773 von 1773 Fenstern exakt 10** — kein Sensorausfall |

**Frame-Vollständigkeit.** Von 1772 Intervallen des Gerätezeitstempels betragen 1412
exakt 100 ms, 357 liegen zwischen 97 und 105 ms (ohne die exakten 100 ms) und **drei**
bei 198 ms. Es sind also genau **drei Frames verloren gegangen** (0,17 %). Der Jitter
ist mit −3/+5 ms leicht unsymmetrisch und stammt aus der BLE-Zustellung, nicht aus der
Firmware: Das firmwareinterne `dt_max_ms` liegt in 89,9 % der Fenster bei exakt 10 ms.

Die drei verlorenen Frames bedeuten, dass rund 30 der etwa 17 790 IMU-Abtastungen nicht
in die Fensteraggregate der Auswertung eingehen. Die Aussage „kein Fenster mit
fehlenden Abtastungen" bezieht sich also auf die **übertragenen** Fenster.

**Zeitbasis.** Aufzeichnungsuhr und Gerätezeit laufen über 177,86 s um 3 ms auseinander
(+0,002 %). Eine Drift der Zeitbasis scheidet damit als Erklärung für den in
Abschnitt 4 behandelten Versatz aus.

> **Bewertung K3 (lückenlose Abtastung):** erfüllt.

---

## 4. Methodik der Referenzbildung — und eine Korrektur der Erstauswertung

### 4.1 Bildung der Referenz

**Zentraldifferenz statt Rückwärtsdifferenz.** Die Erstauswertung bildete
a(t_k) = −(v_k − v_{k−1})/Δt und ordnete den Wert dem Zeitpunkt t_k zu. Physikalisch
beschreibt dieser Wert aber die *mittlere* Beschleunigung im Intervall, also den
Zeitpunkt t_k − Δt/2; die Rückwärtsdifferenz enthält damit einen konstruktionsbedingten
Versatz von einer halben Sekunde. Verwendet wird stattdessen

$$a_{\text{ref}}(t_k) = -\frac{v(t_{k+1}) - v(t_{k-1})}{t_{k+1} - t_{k-1}}$$

was um t_k zeitlich unverzerrt ist. Gültig nur für 1,2 s < t_{k+1} − t_{k−1} < 3,0 s;
von 172 erkannten GNSS-Epochen erfüllen 168 diese Bedingung. Eine GNSS-Epoche ist als
Zeile definiert, in der sich `lat`, `lon`, `speed_kmph` oder `course_deg` gegenüber der
Vorzeile ändert.

**Fensterbildung statt Zeilenvergleich.** Die Referenz existiert mit 1 Hz, das
IMU-Signal mit 10 Hz. Ein zeilenweiser Vergleich koppelt jede Referenzstützstelle an
zehn hochkorrelierte IMU-Zeilen und täuscht eine zehnfach größere Stichprobe vor
(n = 1555 statt n = 168), ohne den Informationsgehalt zu erhöhen. Verglichen wird
deshalb je Stützstelle das Mittel des IMU-Signals über ein Fenster um t_k, dessen
Halbbreite in jeder Angabe mitgeführt wird.

### 4.2 Der Zeitversatz zwischen IMU-Signal und GNSS-Referenz

Zwei voneinander unabhängige Schätzungen:

*Kreuzkorrelation.* Das Maximum über Versätze von −8 bis +2 s liegt bei **−2,0 s**
(r = +0,852, Fenster ±1 s). Das IMU-Signal eilt der Referenz also vor.

*Ereignisweise Bestimmung.* Zeitpunkt des IMU-Maximums gegen Zeitpunkt des
Referenzmaximums je Bremsvorgang: Median **−1,60 s**, Spanne −0,20 … −2,91 s (n = 9).
Diese Schätzung ist **nicht** auf ein Korrelationsmaximum optimiert und damit die
konservativere; sie wird im Folgenden als Standardversatz verwendet.

Zwei Anteile tragen bei, die sich mit den vorliegenden Daten nicht trennen lassen
[beide Annahme]:

*Transportlatenz der GNSS-Kette.* Der Quectel L86 liefert eine Navigationslösung mit
eigener Latenz; die Firmware liest den UART-Puffer mit `PERIOD_GNSS_MS = 1000`, wodurch
ein fertiger NMEA-Satz bis zu einer Sekunde liegen bleibt, bevor er in ein Telemetrie-
Frame gelangt. Größenordnung nach Datenblatt und Quelltext: 0,6 … 1,3 s. Eine Messung
dieser Latenz liegt nicht vor.

*Mittelungsbreite der Referenz.* Die Zentraldifferenz mittelt über zwei Sekunden. Das
IMU-Maximum liegt am Beginn einer Bremsung, das Referenzmaximum in ihrer Mitte; bei
Bremsdauern von 1,5 … 3 s erklärt das weitere 0,5 … 1,5 s.

**Der Versatz ist ein Freiheitsgrad der Auswertung.** Er wird aus denselben Daten
geschätzt, für die anschließend r berichtet wird. Das Korrelationsmaximum über rund
40 Verschiebungen ist deshalb nach oben verzerrt. Deshalb werden in Abschnitt 5
durchgehend **beide** Zahlen genannt — mit und ohne Versatzkorrektur — und der
Vorher-Nachher-Vergleich wird zusätzlich bei einem für beide Datensätze **fest
vorgegebenen** Versatz gerechnet.

### 4.3 Konsequenz für die Erstauswertung vom 06.08.

Die damals berichtete Korrelation von r = −0,132 wurde ohne Versatzkorrektur gerechnet.
Wendet man dieselbe Rechnung mit Versatzsuche auf die Fahrten 1–4 an, steigt r von im
Median −0,18 auf im Median +0,29. Ein Teil des damals gemessenen negativen
Zusammenhangs war also ein **Artefakt der Auswertemethode**.

Das entwertet die damalige Schlussfolgerung nicht: Fehlermechanismus A ist analytisch
aus dem Quelltext hergeleitet, am Prüfstand mit Experiment D gemessen
(3,924 → 0,000 gegenüber 3,9 → 3,836) und über die Duty-Verteilung unabhängig belegt.
Die *Zahl* r = −0,132 ist aber als Gütemaß der Bremserkennung nicht belastbar und
sollte in der Arbeit entsprechend eingeordnet werden.

Hinzu kommt ein zweiter Vorbehalt: Die optimalen Versätze der vier Vergleichsfahrten
streuen mit −3,00, −3,00, −3,75 und −6,00 s erheblich und liegen sämtlich deutlich
über dem Wert vom 08.08. Bei einem Signal, das der Referenz nicht folgt, ist der
„optimale" Versatz jedoch nicht identifizierbar — die Kreuzkorrelation passt dann
Rauschen an. Die Streuung ist damit selbst ein Indiz dafür, dass am 06.08. kein
kohärenter Zusammenhang bestand, und zugleich der Grund, den Hauptvergleich bei festem
Versatz zu führen.

Fahrten 5 und 6 vom 06.08. bleiben durchgehend ausgeschlossen — dort war die GNSS-Lösung
nachweislich grob falsch (Abschattungstest), die Referenz also ungültig.

---

## 5. Ergebnis 1 — Bremserkennung

### 5.1 Korrelation

| Auswertung | n | r | ρ (Spearman) |
|---|---|---|---|
| ohne Versatzkorrektur, Fenster ±1,0 s | 168 | **+0,231** | +0,308 |
| Versatz −1,6 s, Fenster ±0,5 s | 167 | +0,761 | +0,764 |
| Versatz −1,6 s, Fenster ±0,75 s | 168 | +0,784 | +0,792 |
| Versatz −1,6 s, Fenster ±1,0 s | 168 | **+0,808** (95-%-KI +0,748 … +0,855) | +0,827 |
| Maximum der Kreuzkorrelation (−2,0 s, ±1,0 s) | 167 | +0,852 | — |

**Vergleich bei festem Versatz.** Um den in Abschnitt 4.2 benannten Freiheitsgrad
auszuschließen, wurde derselbe Versatz von −2,0 s auf beide Datensätze angewandt:

| Datensatz | n | r bei −2,0 s |
|---|---|---|
| 06.08., Fahrt 1 (22:20) | 137 | +0,192 |
| 06.08., Fahrt 2 (22:26) | 79 | +0,531 |
| 06.08., Fahrt 3 (22:29) | 304 | +0,111 |
| 06.08., Fahrt 4 (22:37) | 58 | −0,043 |
| *06.08., Median* | — | *+0,151* |
| **08.08.** | 151 | **+0,852** |

Die im Lösungskonzept genannte Erwartung „r ≥ +0,7 bei funktionierender Erkennung"
ist damit erfüllt, und zwar unabhängig davon, welcher der beiden Versatzschätzer
verwendet wird.

**Übertragungsverhalten.** Regression über die Verzögerungsphasen (a_ref > 0),
Versatz −1,6 s, Fenster ±0,75 s:

`brake_decel = 1,153 · a_ref + 0,688` (n = 67, r² = 0,474, SE der Steigung 0,151).

Mit Fenster ±1,0 s ergibt sich `1,052 · a_ref + 0,779` (r² = 0,490, SE 0,133). Die
Steigung ist in beiden Fällen innerhalb ihrer Unsicherheit mit 1 verträglich — das
IMU-Signal bildet die Verzögerung im Mittel maßstabsgetreu ab. Der Achsenabschnitt von
+0,69 bis +0,78 m/s² ist die in Abschnitt 8 behandelte Grundlinie.

### 5.2 Ereignisbasierte Auswertung

Sie beantwortet die Frage, die das Produkt stellt: *Geht das Licht an, wenn gebremst
wird — und nur dann?* Verwendet wird durchgehend folgende Taxonomie:

- **Referenz-Bremsvorgang:** zusammenhängende GNSS-Epochen mit a_ref ≥ 1,5 m/s². → **9**
- **Aktivierung:** zusammenhängende Zeilen mit `brake_light_pct` > 20 %. → **14**
- **Anzeigevorgang:** Aktivierungen mit einem Abstand ≤ 1 s zusammengefasst. → **11**

| Anzeigevorgang | t_s | Teil&shy;akti&shy;vie&shy;rungen | Duty max | a_ref lokal | Einordnung |
|---|---|---|---|---|---|
| 1 | 27,86 – 30,16 | 1 | 48 % | 1,78 | Referenz-Bremsvorgang |
| 2 | 36,98 – 38,28 | 1 | 60 % | 1,54 | Referenz-Bremsvorgang |
| 3 | 45,19 – 47,40 | 1 | 92 % | 1,64 | Referenz-Bremsvorgang |
| 4 | 56,52 – 58,72 | 1 | 100 % | 2,17 | Referenz-Bremsvorgang |
| 5 | 66,94 – 69,15 | 1 | 100 % | 1,91 | Referenz-Bremsvorgang |
| 6 | 79,57 – 81,47 | 1 | 100 % | 2,36 | Referenz-Bremsvorgang |
| 7 | 91,59 – 94,00 | **3** | 100 % | 2,97 | Referenz-Bremsvorgang, zerfallen |
| 8 | 102,71 – 103,41 | **2** | 33 % | 0,17 | **Fehlauslösung** |
| 9 | 125,36 – 126,76 | 1 | 100 % | 1,91 | Referenz-Bremsvorgang |
| 10 | 149,51 – 150,91 | 1 | 43 % | 1,33 | reale, schwächere Verzögerung |
| 11 | 161,63 – 163,13 | 1 | 96 % | 2,99 | Referenz-Bremsvorgang |

| Größe | Wert |
|---|---|
| Referenz-Bremsvorgänge erkannt | **9 von 9** |
| Anzeigevorgänge einer realen Verzögerung zuzuordnen | 10 von 11 |
| Fehlauslösungen | **1** (Anzeigevorgang 8, 0,3 s Leuchtdauer, Duty max 33 %) |
| Gesamtleuchtdauer oberhalb Schlusslicht | 19,3 s = 10,9 % der Fahrzeit (± 1,4 s Quantisierung) |
| Anteil Zeilen auf Schlusslicht-Grundwert | 89,1 % |

Zum Vergleich: am 06.08. verharrte der Ausgang in 93–98 % der Zeilen je Fahrt auf dem
Schlusslicht-Grundwert, obwohl Fahrt 2 aus fünf vollständigen Bremsungen bis zum
Stillstand bestand.

### 5.3 Empfindlichkeit gegenüber der Fensterwahl

Eine epochenweise Auswertung hängt sichtbar davon ab, wie breit das Zuordnungsfenster
gewählt wird und ob der Versatz korrigiert ist. Die Werte werden deshalb vollständig
offengelegt statt eine günstige Kombination zu berichten:

| Fenster | Versatz | erkannte Epochen mit a_ref ≥ 1,5 | Auslösungen in Epochen mit \|a_ref\| < 0,5 |
|---|---|---|---|
| ±0,5 s | 0 | 2 von 14 (14 %) | 13 von 81 (16,0 %) |
| ±0,5 s | −1,6 s | 11 von 14 (79 %) | 3 von 81 (3,7 %) |
| ±1,0 s | 0 | 7 von 14 (50 %) | 17 von 81 (21,0 %) |
| **±1,0 s** | **−1,6 s** | **14 von 14 (100 %)** | **9 von 81 (11,1 %)** |
| ±1,5 s | 0 | 10 von 14 (71 %) | 18 von 81 (22,2 %) |
| ±1,5 s | −1,6 s | 14 von 14 (100 %) | 16 von 81 (19,8 %) |

Die rechte Spalte ist **keine** Fehlalarmrate: Epochen mit kleiner Referenzverzögerung
liegen häufig unmittelbar neben einem Bremsvorgang, sodass das Zuordnungsfenster in
das benachbarte echte Ereignis hineinreicht. Beschränkt man die Auswertung auf Epochen,
die mindestens 3 s von jeder Verzögerung ≥ 1,0 m/s² entfernt liegen (58 Epochen), ergibt
sich:

| Fenster (Versatz −1,6 s) | Auslösungen in eindeutig bremsfreien Epochen |
|---|---|
| ±0,5 s | 1 von 58 (1,7 %) |
| ±1,0 s | 2 von 58 (3,4 %) |
| ±1,5 s | 3 von 58 (5,2 %) |

Dieses Ergebnis ist mit der ereignisbasierten Auswertung konsistent: **eine**
Fehlauslösung von 0,3 s Dauer in knapp drei Minuten Fahrt.

### 5.4 Die Fehlauslösung

| t_s | Dauer | Duty max | `jerk_max` | SHOCK je Fenster | a_ref lokal |
|---|---|---|---|---|---|
| 102,71 – 103,41 | 0,3 s (zwei Teile) | 33 % | 7,99 | 7 von 10 | 0,17 |

Sie liegt im rauesten Abschnitt der Strecke, mit Jerk-Werten um 8 m/s² je 10 ms und
sieben von zehn als SHOCK klassifizierten Abtastungen. Es handelt sich also um
**Restdurchgriff eines Fahrbahnstoßes** (Fehlermechanismus B), nicht um eine
Fehlklassifikation ruhiger Fahrt.

Bemerkenswert ist die Dauer. Am 06.08. war die Kritik, dass die 300-ms-Mindesthaltezeit
einen 20-ms-Stoß auf ein 300 ms sichtbares Bremslicht streckt. Hier dauert die
Fehlauslösung 0,3 s in zwei Teilen von 0,1 und 0,2 s — weil die Mindesthaltezeit, wie
Abschnitt 9 zeigt, gar nicht wirkt. Der eine Mangel mildert also den anderen; beide sind
unabhängig voneinander zu bewerten.

---

## 6. Ergebnis 2 — die geprüften Fehlauslösungsszenarien

| Szenario | Datengrundlage | Ergebnis |
|---|---|---|
| **Beschleunigung** (Manöver 4) | 25 GNSS-Epochen mit ≥ 1,0 m/s² Beschleunigung | **0 Auslösungen**, robust über alle Fensterbreiten und mit wie ohne Versatzkorrektur |
| **Stillstand** (Manöver 8) | 118 Abtastungen (11,8 s) mit v < 1 km/h | Duty **konstant 20 %**; `brake_decel` Median 0,00 · P99 0,08 m/s² |
| **Fahrbahnstöße** | 295 Fenster mit mindestens einem SHOCK-Sample (16,6 %) | 1 Fehlauslösung von 0,3 s |
| **Neigung** | lokale Steigungen bis ±8 %, aber nicht als abgegrenzte Manöver protokolliert | keine belastbare Aussage; indirekt über Abschnitt 8 |

Der Stillstandsbefund ist der deutlichste Einzelbeleg für die Wirksamkeit der
Reparaturrunde. Am 06.08. lag der Ausgang im Stillstand auf einem Sockel von etwa
3,0 m/s², zurückgeführt auf den nie kompensierten Gyro-Nullpunktfehler von −4,61 °/s
über die Beziehung ε = b · τ. Nach der entkoppelten Bias-Kalibrierung ist er im
Stillstand nicht mehr nachweisbar. Die Datenbasis sind 11,8 s einer einzelnen Fahrt —
für eine Größenordnungsaussage ausreichend, für eine Aussage über die Streuung nicht.

---

## 7. Ergebnis 3 — Innensicht des Filters

### 7.1 Bias-Kalibrierung

`bias_calibrated` steht bereits in der ersten aufgezeichneten Zeile auf 1; die im
Messprotokoll vorgeschriebenen zehn Sekunden Stillstand vor der Abfahrt haben gewirkt.
Der geschätzte Nullpunktfehler liegt bei **−4,08 °/s** (Bereich −4,16 … −3,98) und
driftet über die gesamte Fahrt um lediglich **+0,18 °/s**. Der Prüfstandswert vom
07.08. betrug −4,61 °/s; die Differenz von 0,53 °/s ist mit der Temperaturabhängigkeit
des MPU-6050-Nullpunkts verträglich (Prüfstand im Innenraum, Fahrt bei Nacht im
Freien), aber nicht belegt — die Temperaturspalte ist ab Schema v3 entfallen
(Entscheidung E-4). **[Annahme]**

### 7.2 Wirksamkeit des Kalibrierfensters — der eigentliche Beleg

Der STATIC-Anteil beträgt im Stillstand **auf dem Rad** nur **39,2 %**, in Fahrt
**25,2 %**. Am Schreibtisch waren es am 07.08. nach Aktivierung des DLPF-Filters
90–100 %.

Damit lässt sich die Reparatur B-FW.11 R6 erstmals am Feldfall begründen. Die
ursprüngliche Auslegung verlangte 100 **zusammenhängende** STATIC-Abtastungen. Bei der
real gemessenen Rate von 0,392 beträgt die Wahrscheinlichkeit dafür

0,392¹⁰⁰ ≈ 10⁻⁴¹.

Die Kalibrierung wäre im Fahrbetrieb also nicht selten, sondern **nie** zustande
gekommen. Die Entkopplung in ein kurzes Verankerungsfenster (0,3 s, Toleranz für zwei
Ausreißer) und eine kumulative Bias-Mittelung über 200 STATIC-Abtastungen ohne
Zusammenhangsforderung war damit keine Optimierung, sondern die Voraussetzung dafür,
dass das System überhaupt in seinen ausgelegten Betriebspunkt kommt. Der
Schreibtischtest allein hätte diesen Nachweis nicht liefern können, weil er mit
90–100 % STATIC eine Umgebung abbildet, die es am Fahrrad nicht gibt.

### 7.3 Nickschätzung

| Größe | Wert |
|---|---|
| Wertebereich über die Fahrt | −13,45° … +3,98° |
| Mittel im Stillstand | −2,11° (Std 2,03°) |
| Mittel in Fahrt | −7,96° (Std 3,28°) |
| Mittel über die ersten 3 s (Stillstand vor der Abfahrt) | **−4,42°** |
| Hub (max − min) je Anzeigevorgang länger als 0,5 s, n = 11 | Median **+2,92°**, Spanne +1,00° … +5,29° |

Zum Vergleich die analytische Erwartung für den Legacy-Filter: Bei einer Verzögerung
von 3 m/s² konvergiert die Scheinneigung gegen arcsin(3/9,81) = 17,8° und erreicht
diesen Wert mit τ = 0,49 s innerhalb von etwa 1,5 s. Gemessen werden 2,92° im Median.

**Abgrenzung [Annahme].** Ein Teil dieses Hubs ist reale Nickbewegung des Fahrrads
(Lastverlagerung, Federweg der Gabel) und kein Filterfehler. Ohne unabhängige
Lagereferenz — etwa eine zweite, nicht am Rahmen befestigte IMU — lässt sich der Anteil
nicht trennen. Belastbar ist nur die Aussage: Die Kontamination der Nickschätzung liegt
eine Größenordnung unter dem Wert, der aus dem Legacy-Verhalten folgt.

### 7.4 Regimeverteilung

| Regime | Anteil der übertragenen 100-Hz-Abtastungen |
|---|---|
| STATIC | 26,2 % |
| DYNAMIC | 69,2 % |
| SHOCK | 4,6 % |

295 der 1773 Fenster (16,6 %) enthalten mindestens ein SHOCK-Sample. Das Gate greift
also in jedem sechsten Fenster ein — bei einem einzigen Durchgriff über die gesamte
Fahrt.

---

## 8. Befund B6 — geschwindigkeitsabhängige Grundlinie

Außerhalb der Bremsvorgänge (1137 Abtastungen; definiert als alle Zeilen mit mehr als
2 s Abstand zu jeder Aktivierung) trägt `brake_decel_ms2` eine von null verschiedene
Grundlinie, die mit der Geschwindigkeit wächst:

| Geschwindigkeit | n | Median | 99-%-Quantil | Reserve zu 2,0 m/s² | STATIC-Anteil | Median Spanne ‖a‖−g |
|---|---|---|---|---|---|---|
| 0–5 km/h | 178 | 0,00 | 0,12 | 1,88 | 39 % | 0,58 |
| 5–15 km/h | 229 | 0,00 | 0,72 | 1,28 | 36 % | 0,66 |
| 15–25 km/h | 218 | 0,07 | 1,09 | 0,91 | 29 % | 0,88 |
| **25–35 km/h** | 361 | 0,75 | **1,68** | **0,32** | 18 % | 1,69 |
| 35–50 km/h | 151 | 0,95 | 1,65 | 0,35 | 23 % | 1,19 |

Korrelation Grundlinie ↔ Geschwindigkeit: **r = +0,80** (n = 1137).
Korrelation Grundlinie ↔ Anregungsniveau (Spanne von ‖a‖−g je Fenster): r = +0,40.
Maximalwert der Grundlinie über die gesamte Fahrt: 1,93 m/s².

### 8.1 Prüfung der Alternativerklärung Fahrbahnneigung

Da lokale Steigungen bis ±8 % auftreten (Abschnitt 2.1), wurde geprüft, ob die
Grundlinie schlicht die Hangabtriebskomponente abbildet:

| Prüfung | Ergebnis |
|---|---|
| Korrelation Grundlinie ↔ Betrag des Neigungsanteils | **r = −0,38** |
| Korrelation Grundlinie ↔ vorzeichenbehafteter Neigungsanteil | r = −0,14 |
| Korrelation Geschwindigkeit ↔ Betrag des Neigungsanteils | r = −0,48 |
| **Partialkorrelation Grundlinie ↔ Geschwindigkeit, Neigung kontrolliert** | **r = +0,75** |

Die Grundlinie ist mit dem Neigungsanteil **negativ** korreliert; der Zusammenhang mit
der Geschwindigkeit bleibt nach Herausrechnen der Neigung mit +0,75 nahezu unverändert.
Fahrbahnneigung scheidet damit als Ursache aus. Die negative Korrelation erklärt sich
plausibel dadurch, dass die steileren Abschnitte in dieser Fahrt zugleich die langsamer
gefahrenen sind (r = −0,48).

### 8.2 Ursachendiskussion

Drei Mechanismen wirken in dieselbe Richtung:

*Einseitige Begrenzung.* `motion_filter` gibt nur den positiven Anteil weiter
(`brake_sign · linear_accel_y > 0 ? … : 0`). Ein mittelwertfreies Rauschsignal mit der
Standardabweichung σ hat nach dieser Gleichrichtung den Erwartungswert σ/√(2π) ≈ 0,40 σ.
Steigt die Anregung mit der Geschwindigkeit, steigt damit zwangsläufig auch die
Grundlinie — ohne jede physikalische Verzögerung. Der Anteil dieses Mechanismus am
Gesamteffekt ist aus den vorliegenden Fensteraggregaten nicht quantifizierbar, weil das
Signal vor der Begrenzung nicht übertragen wird. **[Annahme]**

*Seltenere Verankerung.* Der STATIC-Anteil fällt von 39 % im Stillstand auf 18 % bei
25–35 km/h. Der Beschleunigungssensor darf die Nickschätzung damit seltener korrigieren,
der Restfehler der Lageschätzung wächst und erscheint als Längsanteil.

*Höheres Anregungsniveau.* Die Spanne von ‖a‖−g je Fenster verdreifacht sich von 0,58
auf 1,69 m/s².

### 8.3 Bewertung

Der Effekt führt in dieser Fahrt zu keiner Fehlauslösung durch die Grundlinie allein —
ihr Maximum bleibt mit 1,93 m/s² unter der Ansprechschwelle. Er verringert aber die
Reserve bei Reisegeschwindigkeit auf 0,32 m/s². Die im Vorzustand dokumentierte
„effektive Ansprechschwelle von 2,13 m/s²" ist damit um eine zweite, gegenläufige
Größe zu ergänzen: Bei 25–35 km/h liegt der Arbeitspunkt bereits um bis zu 1,7 m/s²
angehoben, sodass eine reale Verzögerung von nur noch etwa 0,3–0,5 m/s² zum Ansprechen
genügen kann. Erkennungsschwelle und Störfestigkeit sind also geschwindigkeitsabhängig,
und zwar gegenläufig.

Der Umfangsschnitt vom 07.08. sieht keine weitere Schwellenwertiteration vor. Der
Befund gehört als quantifizierte Grenze in die Arbeit und als Empfehlung in den Ausblick
(Abschnitt 13).

---

## 9. Befund B5 — die Mindesthaltezeit ist unwirksam

### 9.1 Messbefund

FR-TL-06 fordert eine Mindesthaltezeit von 300 ms, damit das Bremslicht nach dem
Bremsende nicht abrupt abfällt und nicht flackert. In der Aufzeichnung fällt der
Ausgang in **allen 14 Aktivierungen** innerhalb eines Abtastschritts (0,10 s,
auflösungsbegrenzt) auf den Schlusslicht-Grundwert zurück, sobald `brake_decel_ms2` die
Einschaltschwelle von 2,0 m/s² unterschreitet. Der Wert in der jeweils folgenden Zeile
liegt in 13 von 14 Fällen zwischen 1,45 und 1,97 m/s², also im Hystereseband zwischen
`BRAKE_OFF_MS2` = 1,5 und `BRAKE_ON_MS2` = 2,0.

Vier Aktivierungen sind kürzer als die geforderte Mindesthaltezeit. Zwei der elf
Anzeigevorgänge zerfallen in Teilaktivierungen: Nr. 7 (91,6–94,0 s, ein realer
Bremsvorgang aus 44 km/h) in drei Teile und Nr. 8 (die Fehlauslösung) in zwei. Bei
Nr. 7 schaltet das Bremslicht während eines einzigen Bremsvorgangs zweimal
zwischendurch ab.

### 9.2 Ursache

Die Ursache liegt in `tail_light_fsm.cpp`. Der Haltemechanismus greift erst, wenn der
Eingang unter `BRAKE_OFF_MS2` = 1,5 fällt (Zeile 52). Im Band zwischen 1,5 und 2,0
läuft der `else`-Zweig (Zeile 63–66): Er setzt `below_off_pending_` zurück **und
überschreibt** `held_brake_duty_pct_` mit `brakeDutyPercent(decel)`. Diese Funktion
gibt für jeden Wert ≤ 2,0 den Schlusslichtwert zurück. Fällt der Eingang später unter
1,5 und startet die Haltezeit tatsächlich, hält sie deshalb bereits 20 % — die
Mindesthaltezeit friert den falschen Wert ein.

Da jedes reale Bremssignal das Band zwischen 2,0 und 1,5 durchläuft, ist der
Mechanismus im Feldbetrieb systematisch unwirksam.

### 9.3 Absicherung des Befunds

Der Befund stützt sich nicht allein auf das Lesen des Quelltextes. Eine Nachbildung der
Zustandsmaschine, gespeist mit dem aufgezeichneten `brake_decel_ms2`, reproduziert die
aufgezeichnete `brake_light_pct` in **98,53 %** der Zeilen (1747 von 1773; die
Abweichungen entfallen auf Fenster, in denen das 100-Hz-Signal zwischen zwei
Telemetrie-Abtastungen etwas anderes getan hat als die 10-Hz-Stichprobe zeigt).

Unabhängig davon stimmt der Ausgang bei `brake_decel ≥ 2,0` in **194 von 194** Zeilen
auf ±1 Prozentpunkt (maximal 0,60) mit der in FR-TL-06 spezifizierten Kennlinie
überein — die Proportionalstufe selbst arbeitet also korrekt.

### 9.4 Warum die Unit-Tests das nicht gefunden haben

`test_tail_light_fsm.cpp` enthält drei Tests zur Mindesthaltezeit, alle grün. Sie
speisen den Filter mit idealisierten Sprüngen, etwa 5,0 → 0,0, und überspringen das
Hystereseband dadurch vollständig. Der dritte Test führt zwar den Wert 2,0 im Band ein,
prüft danach aber nur den *Zustand* der Maschine, nicht die ausgegebene Duty.

Das ist ein verwertbarer methodischer Befund: 120 grüne Host-Tests belegen die
Korrektheit der Logik gegenüber den *modellierten* Eingangssignalen. Sie belegen nicht,
dass die modellierten Signale den realen entsprechen. Der Nachweis, dass ein
Bremssignal jedes Schwellenband kontinuierlich durchläuft, konnte nur aus Felddaten
kommen.

---

## 10. Befund B7 — Zeitverhalten im Fahrbetrieb

| Größe | Prüfstand 07.08. | Fahrt 08.08. | Anforderung |
|---|---|---|---|
| `loop_max_us` Median | 651 µs (Worst Case) | 97 µs | — |
| `loop_max_us` 95-%-Quantil | — | 6015 µs | — |
| `loop_max_us` Maximum | 651 µs | **6713 µs** | NFR-RT-04 < 10 000 µs ✅ |
| Anteil Fenster > 5 ms | — | 8,40 % | — |
| Anteil Fenster > 10 ms | — | **0,00 %** | ✅ |
| `dt_max_ms` > 10 ms | — | 10,15 % der Fenster (max. 13 ms) | — |

Die Spitzen treten **periodisch mit 1,00 s** auf (Median des Abstands; 10-%- und
95-%-Quantil 1,00 bzw. 1,01 s) und betreffen 9,9 % der Fenster — das deckt sich eins zu
eins mit dem GNSS-Slot (`PERIOD_GNSS_MS = 1000`). Dieselben Fenster tragen auch die
dt-Ausreißer: 9,9 % der Fenster haben gleichzeitig `loop_max > 4 ms` und
`dt_max > 10 ms`. Ohne diese Fenster liegt die Schleifenzeit im Median bei 92 µs und im
99-%-Quantil bei 249 µs.

**Bewertung.** NFR-RT-04 ist erfüllt, aber die am Prüfstand ermittelte Zahl von
0,651 ms unterschätzt den Fahrbetrieb um den Faktor 10. Die im Zwischenstand vom
07.08. festgehaltene Einschränkung — die Bench-Zeitstatistik beschreibe den Harness und
nicht den produktiven Scheduler — bestätigt sich damit quantitativ. Die verbleibende
Reserve zu NFR-RT-04 beträgt 33 %, nicht die scheinbaren 93 %. Der 1-Hz-NMEA-Parselauf
ist die dominierende Einzellast der Hauptschleife.

Auf NFR-RT-01 (Bremslicht-Reaktionszeit ≤ 50 ms) wirkt sich das nicht kritisch aus:
Selbst wenn ein Bremsbeginn genau in einen 6,7-ms-Slot fällt, bleibt die
Gesamtreaktionszeit deutlich unter der Vorgabe.

---

## 11. Gesamtvergleich 06.08. gegen 08.08.

| Kriterium | 06.08.2026 (Fahrten 1–4) | 08.08.2026 | Bewertung |
|---|---|---|---|
| r Referenz ↔ `brake_decel`, ohne Versatz, ±1 s | −0,14 … −0,25 (Median −0,18) | +0,23 | verbessert |
| r bei festem Versatz −2,0 s, ±1 s | −0,04 … +0,53 (Median +0,15) | **+0,85** | deutlich verbessert |
| r bei individuell optimiertem Versatz | +0,20 … +0,64 | +0,85 | verbessert |
| Anteil Zeilen auf Schlusslicht-Grundwert | 93 … 98 % | 89,1 % | plausibel |
| Erkennung realer Starkbremsungen | 5,75 m/s² → 0,18 m/s², kein Licht | 9 von 9 erkannt | behoben |
| Ruhesockel im Stillstand | ≈ 3,0 m/s² | 0,00 m/s² (P99 0,08) | behoben |
| Fehlauslösungen bei konstanter Fahrt | bis 100 % Duty bei 34,9 → 34,2 km/h | 1 × 0,3 s | weitgehend behoben |
| Fehlauslösungen beim Beschleunigen | vorhanden | 0 von 25 | behoben |
| `bias_calibrated` | nie erreicht | ab dem ersten Frame | behoben |
| Zeitverhalten im Fahrbetrieb | nicht messbar | 6,7 ms Worst Case | erstmals belegt |

---

## 12. Grenzen der Aussagekraft

**Stichprobenumfang.** Diese Auswertung stützt sich auf **eine** Fahrt von knapp drei
Minuten mit neun Bremsvorgängen. Das reicht für einen Vorher-Nachher-Nachweis der
Größenordnung, nicht für belastbare Kennzahlen zur Fehlauslösungsrate: Eine
Fehlauslösung in 178 s erlaubt keine Hochrechnung auf eine Rate pro Kilometer. Teil A
des Messprotokolls (sechs Fahrten auf der Vergleichsstrecke) ist nicht gefahren worden;
der direkte Streckenvergleich zum 06.08. steht weiterhin aus.

**Freiheitsgrade der Auswertung.** Der Zeitversatz, die Fensterbreite, die Schwelle für
ein Referenzereignis (1,5 m/s²), die Schwelle für eine Aktivierung (> 20 %), das
Zusammenfassungsintervall für Anzeigevorgänge (1 s) und die Gültigkeitsgrenzen der
Zentraldifferenz (1,2 … 3,0 s) sind allesamt Wahlentscheidungen. Sie sind im Text
jeweils angegeben; die Empfindlichkeit gegenüber Versatz und Fensterbreite ist in
Abschnitt 5.3 vollständig tabelliert. Das berichtete Maximum r = +0,852 ist ein Maximum
über rund 40 Verschiebungen und damit nach oben verzerrt; der belastbarere Wert ist
r = +0,808 (95-%-KI +0,748 … +0,855) beim unabhängig, ereignisweise geschätzten Versatz.

**Referenzgüte.** Die GNSS-Referenz ist eine 1-Hz-Näherung mit ein bis zwei Sekunden
Transportlatenz und zwei Sekunden Mittelungsbreite. Sie eignet sich zur Bestätigung,
dass Bremsungen erkannt werden, nicht zur Vermessung der Reaktionszeit; deren Nachweis
bleibt beim Prüfstandsversuch.

**Vergleichbarkeit mit dem 06.08.** Die damaligen Exporte liegen mit 1 Hz vor. Das
±1-s-Fenster enthält dort drei statt einundzwanzig Werte; die Auswerteschritte sind
nominell gleich, die Auflösung ist es nicht. Zusätzlich ist der optimale Versatz dort
nicht identifizierbar (Abschnitt 4.3), weshalb der Hauptvergleich bei festem Versatz
geführt wird.

**Zuordnung und nicht abgedeckte Manöver.** Die Zuordnung der Bremsvorgänge zu den
geplanten Manöverklassen ist rekonstruiert, nicht protokolliert. Die Manöver 5 bis 7
sind nicht auswertbar; zur Neigungsfestigkeit im Fahrbetrieb erlaubt die Streckenwahl
keine direkte Aussage.

**Nicht trennbare Anteile.** Reale Nickbewegung des Fahrrads gegenüber
Restkontamination der Nickschätzung (7.3); Gleichrichtungsanteil gegenüber
Lagefehleranteil an der Grundlinie (8.2); Transportlatenz gegenüber Mittelungsbreite
beim Zeitversatz (4.2).

**Quantisierung.** Alle Zeitangaben aus der Telemetrie sind auf 0,1 s quantisiert. Bei
14 Aktivierungen ergibt das für die Gesamtleuchtdauer eine Unsicherheit von etwa ±1,4 s.
Drei verlorene Frames entsprechen rund 30 nicht ausgewerteten IMU-Abtastungen.

---

## 13. Schlussfolgerungen und Empfehlungen

**Zur Kernfrage.** Die Stufe-1-Überarbeitung wirkt im Fahrbetrieb. Der am Prüfstand
gezeigte Effekt überträgt sich auf die Straße: Alle neun realen Bremsvorgänge werden
angezeigt, der Ruhesockel ist verschwunden, Beschleunigungsphasen lösen nicht aus, und
die Korrelation zur unabhängigen Referenz steigt bei identischer Auswertung von im
Median +0,15 auf +0,85. Die Architekturentscheidung V-B (IMU als schneller Regelpfad,
GNSS als langsame Stützreferenz) ist damit feldseitig bestätigt.

**Blockierend vor der Abgabe.** Nichts. Beide neuen Befunde (B5, B6) sind
dokumentierbar, ohne dass Code geändert werden muss.

**Zu B5 (Mindesthaltezeit).** Die Entscheidung zwischen Korrektur und Dokumentation
hängt am Zeitbudget. Eine Korrektur wäre klein — der Haltewert darf nur oberhalb der
Einschaltschwelle aktualisiert werden —, zieht aber eine erneute Verifikation der
Ausgangsstufe und mindestens einen zusätzlichen Test nach sich, der das Hystereseband
durchläuft. Wird nicht korrigiert, gehört der Befund als Anforderungsabweichung in die
Validierungstabelle, nicht in den Ausblick: Eine spezifizierte und getestete Funktion,
die nachweislich nicht wirkt, ist ein Mangel und als solcher zu führen.

**Zu B6 (Grundlinie).** Als quantifizierte Grenze dokumentieren. Für den Ausblick bietet
sich der Hinweis an, dass eine geschwindigkeitsabhängige Ansprechschwelle oder die
Stufe-2-Fusion (GNSS-Bias-Term, τ_b ≈ 10 s) genau diesen Offset adressiert — die Fahrt
liefert damit erstmals eine gemessene statt einer nur konzeptionellen Begründung für
Stufe 2.

**Zur Testmethodik.** Der Befund aus Abschnitt 9.4 gehört in Lessons Learned:
Schwellenwertlogik ist mit Rampen zu testen, die alle Bänder durchlaufen, nicht mit
Sprüngen zwischen den Extremen.

**Zur Auswertemethodik.** Der Befund aus Abschnitt 4.3 gehört ebenfalls dorthin: Bei
einem Vergleich zweier Signalketten mit unterschiedlicher Latenz ist die
Nullversatz-Korrelation kein Gütemaß.

**Was noch fehlt.** Teil A des Messprotokolls. Sechs Fahrten auf der Strecke vom 06.08.
würden aus dieser Einzelfahrt einen echten Streckenvergleich machen. Wenn dafür Zeit
bleibt, ist es die wertvollste verbleibende Messung — und sie muss nach dem Umbau der
Einbaulage ohnehin wiederholt werden.

**Hinweis zur Einbaulage.** Diese Fahrt entstand vor der 180°-Drehung der
Lochrasterplatine. Sie ist damit zugleich die Referenz, gegen die der Umbau zu prüfen
ist: Der Nickwinkel im Stillstand auf dem Rad betrug hier **−4,42°** (Mittel über die
ersten 3 s), der Gyro-Nullpunkt **−4,08 °/s** bei einer Drift von 0,18 °/s über drei
Minuten. Nach dem Umbau mit korrekt abgebildeter Einbaulage müssen beide Werte
innerhalb ihrer Streuung reproduziert werden; andernfalls ist die Achsentransformation
fehlerhaft.

---

## 14. Abbildungen

| Nr | Datei | Inhalt |
|---|---|---|
| 1 | `abb1_fahrtverlauf` | Geschwindigkeit, Referenz- und IMU-Verzögerung, Rücklicht-Duty über der Fahrzeit |
| 2 | `abb2_streudiagramm` | Referenz gegen IMU-Signal, ohne und mit Versatzkorrektur |
| 3 | `abb3_kreuzkorrelation` | Korrelation über dem Zeitversatz, 08.08. gegen 06.08. Fahrten 1–4 |
| 4 | `abb4_detail_bremsvorgang` | Detailausschnitt 78–106 s: Eingang, Duty, SHOCK-Zähler, Nickschätzung |
| 5 | `abb5_zeitverhalten` | `loop_max_us` über der Zeit und als Histogramm, mit NFR-RT-04-Grenze |
| 6 | `abb6_mindesthaltezeit` | Ende eines Bremsvorgangs: Ausgang fällt ohne Haltezeit ab |
| 7 | `abb7_grundlinie` | Grundlinie, STATIC-Anteil und Anregungsniveau über der Geschwindigkeit |

Jede Abbildung liegt als PNG (300 dpi) und als PDF (vektoriell, für den Word-Satz) vor.

---

## 15. Reproduzierbarkeit und Kennzeichnung

Alle Kennzahlen sind aus der genannten CSV mit den Skripten `a1_integritaet.py` bis
`a9_nachpruefung.py` sowie `plots.py` reproduzierbar. Die Referenzbildung ist in
Abschnitt 4 vollständig beschrieben; sämtliche Schwellen und Fensterbreiten sind an der
Stelle ihrer Verwendung angegeben. Für die Vergleichswerte vom 06.08. wurden dieselben
Auswerteschritte auf die damaligen Exporte angewandt; die Fahrten 5 und 6 sind wegen
nachweislich fehlerhafter GNSS-Lösung ausgeschlossen.

Die Zahlen dieses Dokuments wurden zusätzlich durch eine unabhängige Nachrechnung
geprüft, die alle Kernwerte bestätigt hat; die dabei gefundenen Beanstandungen
(Neigungsargument, Fensterasymmetrie, Selektionsbias beim Versatz, uneinheitliche
Ereigniszählung) sind in dieser Fassung eingearbeitet.

**Als gesichert** geführt sind alle Größen, die direkt aus den Messdaten folgen.
**Als Annahme** gekennzeichnet sind: die Zuordnung der Bremsvorgänge zu den geplanten
Manöverklassen (2.2), die Aufteilung des Zeitversatzes in Transportlatenz und
Mittelungsbreite (4.2), die Temperaturerklärung der Bias-Differenz (7.1), die
Aufteilung des Nickwinkelhubs in reale Fahrzeugbewegung und Restkontamination (7.3)
sowie der Anteil der Gleichrichtung an der Grundlinie (8.2). Alle Quelltextaussagen
beziehen sich auf den Stand des Commits `1178017`.

---

## Nachtrag vom 10.08.2026 — Firmware-Abschluss

> Dieser Nachtrag hält fest, was sich nach dem Erstellen der Auswertung geändert
> hat. Die Auswertung selbst bleibt unverändert: Sie dokumentiert den Zustand
> der Firmware am 08.08.2026 (Commit `1178017`) und ist die Beweisgrundlage
> beider Befunde. Der korrigierte Stand ist Commit `835c7b3`.

### N.1 Befund B5 ist behoben

Der in Abschnitt 9 lokalisierte Defekt ist am 10.08.2026 korrigiert worden.
`held_brake_duty_pct_` wird im `else`-Zweig der Bremslicht-Zustandsmaschine nur
noch nachgeführt, wenn `decel_ms2 > BRAKE_ON_MS2` gilt — also nur dann, wenn der
Wert tatsächlich einen Bremslichtwert darstellt. Innerhalb des Hysteresebands
bleibt der zuletzt oberhalb der Einschaltschwelle erreichte Wert eingefroren, und
die Mindesthaltezeit nach FR-TL-06 wird wirksam. Hysterese und Zeitverhalten
blieben unverändert; sie waren korrekt.

Die in Abschnitt 9.4 benannte Testlücke ist geschlossen: Ein Regressionstest
speist einen monoton abklingenden Bremsvorgang ein, der das Hystereseband
durchläuft (4,0 → 2,5 → 1,8 → 1,0 m/s²), und sichert zu, dass die Duty während
der Haltezeit oberhalb des Schlusslichtwerts bleibt und erst danach abfällt. Die
Host-Testsuite umfasst damit 126 Tests, alle grün. Am Gerät ist die Wirkung nach
dem Flashen im Normalbetrieb bestätigt: Das Bremslicht fällt nach dem Ende einer
Bremsung nicht mehr abrupt ab, sondern hält sichtbar nach.

**Für die Arbeit maßgeblich:** Die hier ausgewerteten Daten bilden weiterhin den
Zustand **vor** der Korrektur ab. Eine Nachmessung im Feld ist nach dem
Umfangsschnitt vom 10.08.2026 nicht mehr Teil der Arbeit (Project Bible
Kap. 12.2); die Wirksamkeit der Korrektur ist durch Quelltext, Regressionstest
und Beobachtung am Gerät belegt, nicht durch eine Messfahrt quantifiziert. Die
Kette Feldmessung → Quelltextlokalisierung → Simulation → Korrektur →
Regressionstest → Nachweis am Gerät ist damit vollständig dokumentiert.

### N.2 Befund B7 — Ursachenzuordnung wird zurückgenommen

In Abschnitt 10 ist die periodisch mit 1,00 s auftretende Spitze der
Schleifenzeit dem GNSS-Slot (`PERIOD_GNSS_MS` = 1000) zugeschrieben und der
NMEA-Parselauf als dominierende Einzellast bezeichnet worden. **Diese Zuordnung
war nicht belegt und wird zurückgenommen.**

Im aufgezeichneten Firmware-Stand liefen **drei** Debug-Ausgaben mit ebenfalls
exakt 1 Hz, und alle drei lagen innerhalb des Zeitfensters, aus dem `loop_max_us`
gebildet wird: `[R1/R2]` (≈ 72 Byte), `[Baro]` (≈ 41 Byte) und `[GNSS]`
(≈ 60 Byte), zusammen rund 173 Byte. Bei 115 200 Bd entspricht das etwa 87 µs je
Byte; sobald der UART-Sendepuffer gefüllt ist, blockiert `Serial.printf`. Fallen
zwei dieser Ausgaben in denselben `loop()`-Durchlauf, liegt ihr Beitrag in
derselben Größenordnung wie der gemessene Ausreißer von 6,7 ms.

Aus der Zeitreihe allein sind beide Ursachen **nicht trennbar**, weil beide exakt
mit 1,00 s periodisch sind. Der Messwert selbst bleibt gültig, ebenso die daraus
abgeleitete und für die Anforderung maßgebliche Aussage: **NFR-RT-04 (< 10 ms)
ist erfüllt**, und die Prüfstandszahl von 0,651 ms unterschätzt den Fahrbetrieb
um rund den Faktor zehn. Nicht belegbar ist allein die Behauptung über die
dominierende Einzellast.

Im Auslieferungsstand (Commit `835c7b3`) sind alle drei Ausgaben entfernt und
`DEBUG_SERIAL` steht auf `false`; die UART-Last entfällt damit. Der
Diskriminierungsversuch — eine Aufzeichnung mit und ohne Debug-Ausgaben — ist
nicht mehr Teil des Arbeitsumfangs und gehört als konkrete Empfehlung in den
Ausblick.

**Methodischer Befund, der in die Arbeit gehört.** Zwei Vorgänge mit identischer
Periode lassen sich aus einer Zeitreihe grundsätzlich nicht auseinanderhalten;
die ursprüngliche Zuordnung war eine plausible Vermutung, kein Messergebnis.
Allgemeiner: Eine Diagnoseausgabe, die im Messfenster derjenigen Größe liegt, die
sie beobachten soll, wird selbst Teil des Messobjekts. Dieser Beobachtereffekt
wird in den Daten nicht sichtbar.

### N.3 Einbaulage

Der in Abschnitt 13 genannte Umbau (180°-Drehung der Lochrasterplatine) ist
umgesetzt: Die Einbaulage wird als Rotation an der Treibergrenze zurückgerechnet
(`lib/logic/imu_mount_orientation.h`, `IMU_MOUNT_SIGN_X/Y/Z` = −1/−1/+1 auf
Beschleunigung und Drehrate, Determinante +1). Die dort genannten Referenzwerte —
Nickwinkel **−4,42°** und Gyro-Nullpunkt **−4,08 °/s** — bleiben als
dokumentierte Vergleichsgrößen bestehen; eine systematische Nachmessung ist nach
dem Umfangsschnitt nicht mehr vorgesehen.
