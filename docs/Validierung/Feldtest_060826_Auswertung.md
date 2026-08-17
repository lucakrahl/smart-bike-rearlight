# Feldtest 06.08.2026 — Auswertung Bremslichterkennung und GNSS-Eignung

**Bachelorarbeit Krahl · Smart Bike Rear Light**
**Dokumenttyp:** Validierungsbericht (Zuarbeit zur Project Bible, Kap. 9.4)
**Prüfling:** Firmware-Stand vor Feldtest (Frame-Schema v2, 81 Byte), Board Espressif ESP32-DevKitC-32E
**Datum Versuchsdurchführung:** 06.08.2026, 22:20–22:43 Uhr MESZ
**Erstellt:** 06.08.2026 · **Status:** abgeschlossen, Befunde freigegeben · **Nachtrag 17.08.2026: zwei Befunde zurückgezogen, siehe Kasten unten**

> ## Nachtrag vom 17.08.2026 — zurückgezogene Befunde
>
> **(1) Der Integritätsbefund aus Fahrt 5 ist vollständig zurückgezogen.** Abschnitt 6.4 führte
> die gemeldeten 73 km/h bei gesetztem Fix-Flag als Beleg dafür, dass eine falsche
> Navigationslösung unerkannt bleibt. Fahrt 5 wurde nach Klarstellung des Verfassers **aus dem
> fahrenden Auto** durchgeführt. Die gemeldete Geschwindigkeit war damit real, die Abdeckung hat
> das Signal nicht ausreichend gedämpft. Der Versuch ist ergebnislos und weder Beleg für noch
> gegen die Integrität des Empfängers. Eine belastbare Integritätsprüfung liegt nicht vor.
>
> **(2) Die Latenzangabe von 200 bis 400 ms ist zurückgezogen.** Sie war eine eigene Abschätzung
> und im Datenblatt nicht belegt. Maßgeblich ist der an der Messfahrt vom 08.08.2026 bestimmte
> Versatz von **1,6 bis 2,0 s** zwischen Inertialsignal und Satellitenreferenz.
>
> **(3) Die Korrelation r = −0,132 ist als Gütemaß nicht belastbar.** Sie wurde ohne
> Berücksichtigung der Latenz der Referenzkette gerechnet. Bei einheitlich herausgerechnetem
> Versatz von 2,0 s ergibt sich für die Vergleichsfahrten im Median +0,15 gegenüber +0,85 für die
> Messfahrt vom 08.08.2026.
>
> **Unberührt bleibt die Falsifikation der Bremserkennung.** Sie ist analytisch aus dem Quelltext
> hergeleitet, am Prüfstand gemessen und über die Verteilung der Lichtstärke unabhängig belegt.
> Ebenso unberührt bleibt die Entscheidung gegen eine satellitengestützte Primärarchitektur. Sie
> steht auf der Ratengrenze, der Latenz, der Verfügbarkeit nach dem Einschalten und dem Rauschen.

> Dieses Dokument ist die ausführliche Fassung der Feldtest-Auswertung. Die Project Bible führt in Kap. 9.4 nur die verdichteten Befunde; die Herleitung, die Rohdatenauszüge und die Variantendiskussion stehen hier. Für die Thesis ist dieses Dokument die Quelle für das Validierungskapitel „Feldtest Bremslichterkennung“.

---

## 1. Zweck und Einordnung

Die Serial-Bench-Validierung (Project Bible Kap. 9.3) hatte nachgewiesen, dass die **Steuerlogik** des Bremslichts spezifikationskonform arbeitet: Ansprechschwelle exakt bei 2,0 m/s², linearer Anstieg bis 100 % bei 5,0 m/s² (R² = 0,99984), 300 ms Mindesthaltezeit, Fail-Safe bei IMU-Ausfall. Diese Bench speiste jedoch ein **bereits fertiges Verzögerungssignal** ein. Die Frage, ob die vorgelagerte Kette *Beschleunigungssensor → Komplementärfilter → Schwerkraftkompensation* im realen Fahrbetrieb überhaupt ein brauchbares Verzögerungssignal liefert, war damit ausdrücklich **nicht** beantwortet (Kap. 9.3, Abschnitt „Einordnung/Grenzen“).

Genau diese Lücke schließt der Feldtest. Er ist damit kein Nachweis, sondern ein **Falsifikationsversuch** — und er ist erfolgreich verlaufen: Die Kette wurde falsifiziert. Das ist wissenschaftlich der wertvollere Ausgang, weil er einen konkreten, messbar belegten Konstruktionsfehler offenlegt und dessen Behebung überprüfbar macht (Vorher-Nachher-Vergleich).

---

## 2. Versuchsaufbau und Methodik

### 2.1 Prüfling und Messkette

| Element | Ausführung |
|---|---|
| Prüfling | Smart Bike Rear Light, Aufbau auf Lochraster/Breadboard, am Fahrrad montiert |
| Primäre Messkette | ESP32 → BLE-Notify (81-Byte-Frame, Schema v2, 10 Hz) → iOS-App → SwiftData-Persistenz (1 Hz) → CSV-Export (23 Spalten, `;`-getrennt, deutsche Dezimalkommata) |
| Referenzmesskette | Strava (Smartphone-GNSS, unabhängige Aufzeichnung derselben Fahrt) |
| Protokollierung | Feldtest-Leitfaden mit vorab definierten Prüfpunkten und Protokollbogen je Fahrt |
| Umgebung | Öffentlicher Verkehrsraum, Nachtfahrt, freie Himmelssicht |

Der wesentliche methodische Punkt: Es existieren **zwei unabhängige Messketten** für dieselbe physikalische Größe (Geschwindigkeit über Grund). Damit lässt sich die eigene Messkette gegen eine externe Referenz prüfen, statt sie nur gegen sich selbst zu vergleichen. Das ist die Voraussetzung dafür, dass die anschließende Fehleranalyse überhaupt belastbar ist.

### 2.2 Versuchsplan (aus dem Feldtest-Leitfaden)

Der Leitfaden definierte sechs Fahrten mit jeweils eigenem Prüfzweck. Entscheidend für die Auswertung: Die Prüfzwecke wurden **vorab** festgelegt, nicht nachträglich aus den Daten abgeleitet.

| # | Uhrzeit | Prüfzweck laut Leitfaden |
|---|---|---|
| 1 | 22:20 | Versuch einer konstanten Fahrt bei 30 km/h (Referenzfahrt, Messkettenvergleich) |
| 2 | 22:26 | 5 × Abbremsen von 20 km/h bis Stillstand, jeweils 5 s stehen (**Kern-Bremsversuch**) |
| 3 | 22:29 | Langzeittest (Stabilität, Drift, Dauerbetrieb) |
| 4 | 22:37 | Versuchsaufbau während der Fahrt in alle vier Richtungen neigen (**Störgrößentest Neigung**) |
| 5 | 22:40 | GNSS-Antenne **nach** erlangtem Fix verdecken (Integritätstest) |
| 6 | 22:42 | GNSS-Antenne **vor** dem Fix verdecken (Kaltstart-Test) |

### 2.3 Vorab definierte Akzeptanzkriterien

| Kriterium | Sollwert |
|---|---|
| K1 Distanzabweichung App ↔ Strava | ≤ ± 5 % |
| K2 Abweichung Höchstgeschwindigkeit | ≤ 1–2 km/h |
| K3 Abtastung | lückenlos ~1 Hz, keine Aussetzer |
| K4 Fix-Gating | GNSS-Werte nur bei gültigem Fix verwertet |
| K5 **Bremslichtreaktion** | **bei `brake_decel_ms2` ≥ 2,0 m/s² steigt `brake_light_pct` über 20 %** |

K5 ist das eigentliche Prüfkriterium dieses Feldtests.

---

## 3. Ergebnis Teil 1 — Validierung der Messkette (K1–K4)

| Fahrt | Distanz App / Strava | Abw. | Fahrzeit App / Strava | Ø App / Strava | Max App / Strava | Hm App / Strava |
|---|---|---|---|---|---|---|
| 1 (22:20) | 1,40 / 1,43 km | −2,1 % | 03:08 / 03:17 | 27,0 / 26,2 | 30,4 / 33,4 | 3 / 3 |
| 2 (22:26) | 0,64 / 0,64 km | 0,0 % | 01:38 / 02:01 | 23,7 / 19,1 | 38,0 / 38,2 | 3 / 0 |
| 3 (22:29) | 2,51 / 2,50 km | +0,4 % | 05:54 / 06:02 | 25,5 / 24,9 | 33,2 / 33,7 | 7 / 3 |
| 4 (22:37) | 0,53 / 0,57 km | −7,0 % | 01:17 / 01:32 | 25,0 / 22,3 | 33,8 / 34,6 | 3 / 0 |

**Bewertung K1:** Für die Fahrten 1–3 wird das ±5-%-Kriterium eingehalten (−2,1 %, 0,0 %, +0,4 %). Fahrt 4 liegt mit −7,0 % darüber; die Fahrt ist mit 0,53 km jedoch die kürzeste und wurde absichtlich durch Neigen des Aufbaus gestört, außerdem unterscheiden sich die aufgezeichneten Fahrzeiten um 15 s (unterschiedliche Auto-Pause-Logik). Die Abweichung ist damit erklärbar und nicht als Messkettenfehler zu werten.

**Bewertung K2:** Die Maximalgeschwindigkeiten stimmen bei drei von vier Fahrten auf 0,2–0,8 km/h überein (38,0/38,2 · 33,2/33,7 · 33,8/34,6). Bei Fahrt 1 weicht sie mit 30,4 gegen 33,4 km/h um 3,0 km/h ab — Strava glättet und interpoliert Geschwindigkeitsspitzen anders. Das Kriterium ist damit im Wesentlichen, aber nicht durchgängig erfüllt.

**Bewertung K3:** Die Abtastung ist in allen CSV-Exporten lückenlos; die Zeitstempel `t_s` steigen in ~1,00-s-Schritten, `device_timestamp_ms` läuft monoton. Kein Frame-Verlust erkennbar.

**Bewertung K4:** Das Fix-Gating funktioniert formal — in Fahrt 6 werden ~20 s korrekt als `NO_FIX` mit `sats = 0` geführt. Eine Aussage darüber, ob ein gesetztes Fix-Flag eine korrekte Navigationslösung belegt, lässt sich aus diesem Feldtest nicht ableiten (Nachtrag 17.08.2026).

**Zwischenfazit:** Die eigene Messkette ist tragfähig. Kumulative Größen (Distanz, Höhenmeter, Durchschnitts- und Höchstgeschwindigkeit) stimmen mit einer unabhängigen Referenz überein. Damit sind alle folgenden Aussagen zum Bremslicht **nicht** durch eine defekte Messkette erklärbar.

---

## 4. Ergebnis Teil 2 — Bremslichtverhalten (K5)

Datengrundlage: alle sechs CSV-Exporte, insgesamt **939 auswertbare Zeilen** mit gültigem Fix und gültigem Vorgängerwert.

### 4.1 Korrelationsanalyse

Als unabhängige Referenz für die tatsächliche Längsverzögerung wurde die GNSS-Geschwindigkeit numerisch differenziert:

$$a_{\text{GNSS}}(t_k) = -\frac{v(t_k) - v(t_{k-1})}{t_k - t_{k-1}}$$

Verglichen wurde diese Referenz mit der von der Firmware ausgegebenen Größe `brake_decel_ms2` (dem Ausgang des Komplementärfilters, also dem Eingang der Bremskennlinie).

| Kenngröße | Wert |
|---|---|
| Pearson-Korrelation r über alle Fahrten | **−0,132** |
| Stichprobenumfang n | 939 |
| Spannweite r je Einzelfahrt | −0,09 … −0,20 |
| Erwartungswert bei funktionierender Erkennung | r ≥ +0,7 |

**Interpretation:** Der IMU-Verzögerungswert korreliert nicht positiv mit der tatsächlichen Verzögerung. Er korreliert sogar schwach **negativ**, d. h. wenn das Fahrrad tatsächlich verzögert, ist der IMU-Wert tendenziell eher kleiner. Das Vorzeichen ist konsistent mit dem in Abschnitt 5 hergeleiteten Fehlermechanismus und über alle sechs Fahrten stabil — es ist kein Zufallsbefund einer einzelnen Fahrt.

### 4.2 Verteilung der ausgegebenen Bremslicht-Duty

| Fahrt | Anteil Zeilen mit `brake_light_pct` = 20 % |
|---|---|
| 1 (22:20) | 99 % |
| 2 (22:26, Kern-Bremsversuch) | 95 % |
| 3 (22:29) | 93 % |
| 4 (22:37) | 98 % |
| 5 (22:40) | 95 % |
| 6 (22:42) | 100 % |

Auch in Fahrt 2 — der Fahrt, die aus **fünf vollständigen Bremsungen von 20 km/h bis zum Stillstand** besteht — bleibt das Licht in 95 % der Abtastungen auf Schlusslichtniveau. Die verbleibenden 5 % liegen, wie Abschnitt 4.4 zeigt, überwiegend **nicht** bei den Bremsungen.

### 4.3 Nicht erkannte Bremsvorgänge

Ausgewählte Ereignisse, bei denen die GNSS-Referenz eine eindeutige Bremsung belegt:

| Fahrt | t [s] | v-Verlauf [km/h] | a_GNSS [m/s²] | `brake_decel_ms2` | `brake_light_pct` |
|---|---|---|---|---|---|
| 2 | 71,0 | 27,9 → 7,2 | **−5,75** | 0,18 | 20 % |
| 2 | 27,1 | — | −2,97 | 0,57 | 20 % |
| 2 | 119,1 | 38,0 → 28,2 | −2,72 | 0,00 | 20 % |
| 1 | 183,0 | — | −2,58 | 0,26 | 20 % |
| 3 | 354,0 | — | −2,17 | 0,19 | 20 % |
| 4 | 20,1 / 49,1 | — | −1,86 | — | 20 % |

Das erste Ereignis ist das aussagekräftigste: eine Vollbremsung mit **5,75 m/s²** — deutlich über der Sättigungsschwelle von 5,0 m/s², das Licht müsste auf 100 % gehen — erzeugt einen Filterausgang von 0,18 m/s², also **3 % des tatsächlichen Wertes**. Das Licht bleibt auf 20 %.

**K5 ist damit nicht erfüllt.**

### 4.4 Fehlauslösungen ohne Verzögerung

| Fahrt | t [s] | v [km/h] | a_GNSS [m/s²] | `brake_decel_ms2` | `brake_light_pct` |
|---|---|---|---|---|---|
| 2 | 51,1 | 34,9 → 34,2 | ≈ 0 | **5,71** | **100 %** |
| 2 | 68,0 | — | ≈ 0 | 3,70 | 65 % |
| 2 | 117,1 | beschleunigend | < 0 | 3,13 | 50 % |
| 4 | 85,1 | 30,4 | ≈ 0 | 3,64 | 64 % |
| 3 | 192,1 | — | ≈ 0 | — | 60 % |
| 5 | 6,0 | — | ≈ 0 | 0,00 | 57 % |

Das erste Ereignis ist das Spiegelbild von 4.3: bei nahezu konstanten 34,5 km/h meldet der Filter **5,71 m/s²** und das Bremslicht geht auf **100 %** — die maximale Warnwirkung wird genau dann ausgelöst, wenn nicht gebremst wird. Ereignis 3 ist besonders kritisch, weil das Fahrzeug dort **beschleunigt** und trotzdem 50 % Bremslicht sendet; die Anforderung FR-TL-02 („Bremslicht-Helligkeit steigt mit der Bremsintensität“) und die Entscheidung „Bremslicht nur bei Verzögerung in Fahrtrichtung“ (Project Bible Kap. 10) werden hier beide verletzt.

Sicherheitstechnisch ist dieser Befund gravierender als das Nichterkennen: Ein Bremslicht, das ohne Bremsvorgang aufleuchtet, entwertet das Signal für den nachfolgenden Verkehr (Gewöhnungseffekt, „cry wolf“).

### 4.5 Zuordnung der Störgrößen (Neigungstest Fahrt 4)

Der Leitfaden weist Fahrt 4 als gezielten Neigungstest aus. Die relevanten Zeilen:

```
t =  0,0 s   v =  0,0 km/h   decel = 0,53   pct = 20
t =  3,0 s   v =  0,0 km/h   decel = 0,00   pct = 42
t =  8,0 s   v =  2,4 km/h
t = 11,0 s   v = 20,7 km/h
...
t = 84,1 s   v = 30,5 km/h   decel = 0,36   pct = 20
t = 85,1 s   v = 30,4 km/h   decel = 3,64   pct = 64
t = 86,1 s   v = 31,1 km/h   decel = 0,46   pct = 20
```

Zwei getrennte Befunde:

- **t = 3,0 s, v = 0 km/h → 42 % Bremslicht im Stand.** Das Fahrrad steht. Es kann keine Verzögerung geben. Dies ist mit hoher Wahrscheinlichkeit das gezielte Neigen des Aufbaus, also der **direkte experimentelle Nachweis, dass statische Neigung als Bremsung fehlinterpretiert wird**.
- **t = 85,1 s, v = 30,4 km/h → 64 % Bremslicht bei konstanter Fahrt.** Hier fährt das Rad mit voller Geschwindigkeit; das ist kein Neigungsereignis, sondern ein **Stoßereignis** (Fahrbahnunebenheit, Bordstein, Schlagloch). Es dauert genau eine Abtastperiode.

Die beiden Ereignisse belegen unabhängig voneinander die **zwei verschiedenen Fehlermechanismen** aus Abschnitt 5. Die saubere Trennung ist wichtig — sie darf in der Thesis nicht zu „alles Schlaglöcher“ verkürzt werden.

### 4.6 Nebenbefund: Aliasing durch 1-Hz-Persistenz

In einzelnen Zeilen stehen `brake_decel_ms2 = 0,00` und ein erhöhter `brake_light_pct` nebeneinander (Fahrt 4, t = 3,0 s: 0,00/42 %; Fahrt 5, t = 6,0 s: 0,00/57 %; Fahrt 1, t = 188,0 s: 0,00/27 %). Das ist **kein** Logikfehler: Firmware und Frame arbeiten mit 10 Hz, die App persistiert jedoch nur 1 Hz. Eingang und Ausgang der Kennlinie stammen dann aus unterschiedlichen Momentaufnahmen innerhalb derselben Sekunde. Die 300-ms-Mindesthaltezeit überlebt einen kurzen Verzögerungspeak, der beim Abtastzeitpunkt bereits wieder auf 0 gefallen ist.

**Konsequenz:** Die exportierte 1-Hz-CSV eignet sich für kumulative Fahrdaten, ist aber zu grob, um Bremsdynamik aufzulösen. Für den Nachweis der Verbesserung nach dem Umbau wird ein höher aufgelöster Validierungspfad benötigt (s. Abschnitt 10).

---

## 5. Ursachenanalyse

### 5.1 Der implementierte Signalpfad

Der Filter (`firmware/lib/logic/motion_filter.cpp`) arbeitet in vier Schritten:

```cpp
const float pitch_from_accel = atan2f(in.accel_y_ms2, in.accel_z_ms2);
pitch_rad_ = alpha * (pitch_rad_ + in.gyro_x_rads * in.dt_s)
           + (1.0f - alpha) * pitch_from_accel;
const float gravity_y      = gravity_ms2 * sinf(pitch_rad_);
const float linear_accel_y = in.accel_y_ms2 - gravity_y;
return brake_sign * linear_accel_y > 0 ? ... : 0.0f;
```

Er schätzt aus dem Beschleunigungsvektor einen Nickwinkel, glättet ihn mit dem Gyroskop über einen Komplementärfilter, rechnet daraus den auf die Fahrtrichtungsachse entfallenden Schwerkraftanteil aus und zieht diesen von der gemessenen Beschleunigung ab. Was übrig bleibt, gilt als Längsbeschleunigung.

### 5.2 Fehlermechanismus A — die Scheinneigung (systematisch)

Der Komplementärfilter hat bei α = 0,98 und 100 Hz Abtastung die Zeitkonstante

$$\tau = \frac{\alpha \cdot \Delta t}{1-\alpha} = \frac{0{,}98 \cdot 0{,}01\,\text{s}}{0{,}02} = \mathbf{0{,}49\ s}$$

Das heißt: Alles, was länger als etwa eine halbe Sekunde konstant anliegt, wird vom Filter als **Neigung** interpretiert und in den Schwerkraftanteil eingerechnet.

Der Beschleunigungssensor kann prinzipbedingt nicht zwischen einer Neigung und einer Längsbeschleunigung unterscheiden — beides erzeugt dieselbe Komponente auf der Fahrtrichtungsachse (Äquivalenzprinzip). Der Komplementärfilter löst diese Mehrdeutigkeit, indem er dem Gyroskop kurzfristig und dem Beschleunigungssensor langfristig vertraut. Für ein stehendes oder gleichförmig fahrendes System ist das korrekt. Für eine **anhaltende Bremsung** ist es fatal:

1. Die Bremsung beginnt, `accel_y` steigt.
2. Das Gyroskop meldet keine Drehung (das Rad neigt sich nicht wirklich).
3. Der Filter „zieht“ mit τ = 0,49 s den geschätzten Nickwinkel in Richtung des vermeintlichen Neigungswinkels.
4. `gravity_y` wächst mit — und wird von `accel_y` abgezogen.
5. Nach etwa 1–1,5 s ist die reale Bremsbeschleunigung **vollständig als Schwerkraft weggerechnet**. Der Ausgang geht gegen null.

Eine typische Bremsung von 20 km/h bis Stillstand mit 3 m/s² dauert 1,85 s — also länger als die Filterzeitkonstante. Genau deshalb erscheint die 5,75-m/s²-Vollbremsung in den Daten als 0,18 m/s².

Und weil beim **Lösen** der Bremse der Filter mit derselben Trägheit zurückläuft, entsteht ein negativer Überschwinger — der Ausgang wird kurzzeitig negativ (also als „Beschleunigung“ gedeutet) und die Restverzögerung wird unterdrückt. Das erklärt das **negative Vorzeichen** der Korrelation aus Abschnitt 4.1.

### 5.3 Fehlermechanismus B — Stoßdurchgriff (impulsiv)

Zwischen `accel_y` und der Kennlinie liegt **keinerlei Tiefpass, Median oder Stoßunterdrückung**. Ein Schlagloch erzeugt an einem starr am Rahmen montierten Sensor Beschleunigungsspitzen im Bereich mehrerer g mit Dauern von 10–30 ms. Bei 100 Hz Abtastung liegt eine solche Spitze auf 1–3 Samples — sie ist zu kurz, als dass der Komplementärfilter sie in den Nickwinkel einarbeiten könnte, geht also **ungedämpft** durch die Schwerkraftkompensation hindurch in die Kennlinie.

Die Kennlinie ist auf schnellen Anstieg ausgelegt (NFR-RT-01, Bench: ≤ 10 ms) und die Mindesthaltezeit von 300 ms hält den Ausschlag anschließend fest. Ein 20-ms-Stoß erzeugt damit ein **300 ms sichtbares Bremslicht**. Das ist der Grund, warum die Fehlauslösungen für den Fahrer *auffälliger* waren als die echten Bremsungen — sie sind es tatsächlich, weil das System die kurzen Störimpulse zeitlich streckt und die langen Nutzsignale wegfiltert. Es verhält sich damit exakt invers zur Anforderung.

### 5.4 Abgrenzung — die IMU ist nicht das Problem

Diese Abgrenzung ist für die wissenschaftliche Sauberkeit der Arbeit zentral, weil die naheliegende Schlussfolgerung („die IMU taugt nicht, das GNSS muss übernehmen“) durch die Daten **nicht** gedeckt ist.

- Ein MPU-6050 mit ±16 g Messbereich löst 0,5 m/s² problemlos auf; das Rauschband liegt weit unter den geforderten 2,0 m/s².
- Der Sensor liefert die Information mit 100 Hz und ohne nennenswerte Latenz — die einzige Sensorik im System, die NFR-RT-01 (≤ 50 ms) überhaupt erfüllen kann.
- Kommerzielle Fahrradrücklichter mit Bremslichtfunktion (u. a. Busch & Müller, Trelock, Garmin Varia RTL) arbeiten **ausschließlich** beschleunigungsbasiert und funktionieren. Das Verfahren ist also nachweislich tragfähig.

Nicht der Sensor ist ungeeignet, sondern die **Auswertevorschrift**. Der Fehler liegt in einer Zeile Signalverarbeitung, nicht in der Hardwarearchitektur.

---

## 6. Prüfung der Hypothese „GNSS übernimmt die Bremserkennung“

Geprüft wurde die Frage, ob der Quectel L86 — ggf. mit hoher Abtastrate von 100 Hz — die Rolle des primären Bremssensors übernehmen kann und die IMU nur noch als Notfall-Rückfallebene bei Fix-Verlust dient.

### 6.1 Datenblattgrenze der Abtastrate

Aus dem Herstellerdatenblatt (Quectel L86 Hardware Design, Tabelle 1, S. 9):

| Parameter | Wert laut Datenblatt |
|---|---|
| Maximale Update-Rate | **bis 10 Hz** (Standard 1 Hz) |
| Geschwindigkeitsgenauigkeit | 0,1 m/s |
| Beschleunigungsgenauigkeit | 0,1 m/s² |
| Positionsgenauigkeit | < 2,5 m CEP |
| TTFF mit EASY | 15 s kalt / 5 s warm |
| TTFF ohne EASY | 35 s kalt / 30 s warm |
| Stromaufnahme | 26 mA Tracking / 30 mA Acquisition (GPS+GLONASS) |

**100 Hz sind hardwareseitig nicht darstellbar.** Der Empfänger endet bei 10 Hz — Faktor 10 unter der Anforderung. Die Frage ist damit auf der Ebene des Datenblatts abschließend beantwortet und benötigt keinen Versuch.

Ergänzend: Das Datenblatt vermerkt ausdrücklich, dass **EASY (Self-Assisted-AGNSS) automatisch deaktiviert wird, sobald die Update-Rate 1 Hz überschreitet**. Eine Erhöhung der Rate verlängert also die Erstfixzeit von 15 s auf 35 s (kalt) bzw. von 5 s auf 30 s (warm). Das ist ein realer Zielkonflikt, der bei der Auslegung der Stufe 2 (Abschnitt 9.2) berücksichtigt werden muss.

### 6.2 Bandbreitengrenze der Schnittstelle

Der L86 ist mit 9600 Baud angebunden. Das entspricht rund 960 Byte/s Nutzdaten. Ein vollständiger NMEA-Satz je Epoche (RMC, GGA, GSA, GSV …) umfasst je nach Satellitenzahl 400–600 Byte. Bei 1 Hz ist das unkritisch; bereits bei 5 Hz wäre die Leitung überlastet. Eine Ratenerhöhung erfordert daher zwingend **gleichzeitig** eine Erhöhung der Baudrate und eine Reduktion des NMEA-Satzumfangs. Das ist machbar (s. Abschnitt 9.2), zeigt aber, dass „einfach die Rate hochdrehen“ nicht funktioniert.

### 6.3 Latenzgrenze gegenüber NFR-RT-01

| Beitrag | Größenordnung |
|---|---|
| Interne Lösungslatenz des Empfängers (Signalverarbeitung, Ausgabe) | 100–300 ms |
| Zusätzliche Epoche für die Differentiation v → a | + 1 Abtastperiode |
| **Summe bis zur nutzbaren Verzögerungsinformation** | ~~200–400 ms~~ · gemessen 1,6 bis 2,0 s (Nachtrag 17.08.2026) |
| Anforderung NFR-RT-01 | **≤ 50 ms** |

Die Überschreitung beträgt Faktor 4 bis 8. Sicherheitstechnisch übersetzt sich das direkt in Weg: Bei einem mit 50 km/h (13,9 m/s) folgenden Fahrzeug entsprechen 300 ms zusätzlicher Verzug **4,2 m** später einsetzender Warnwirkung. Eine Architektur, die die schnellste Sicherheitsfunktion des Systems an den langsamsten Sensor bindet, ist nicht begründbar.

### 6.4 Verfügbarkeitsgrenze  ~~und Integritätsgrenze~~

> **Zurückgezogen am 17.08.2026.** Der folgende Abschnitt bis einschließlich des Absatzes zur
> Unsichtbarkeit des Fehlers beruht auf Fahrt 5. Diese Fahrt fand im fahrenden Auto statt, die
> gemeldete Geschwindigkeit war real. Der Abschnitt ist **kein Beleg** und darf nicht verwendet
> werden. Er bleibt aus Gründen der Nachvollziehbarkeit stehen. Gültig bleibt allein der letzte
> Absatz zu Fahrt 6 und der Verfügbarkeit nach dem Einschalten.

Fahrt 5 (22:40, „GNSS nach Fix verdecken“). Auszug aus den Rohdaten:

| t [s] | v [km/h] | Fix | Sats | HDOP | lat | lon |
|---|---|---|---|---|---|---|
| 0,0 | **73,0** | FIX_OK | 15 | 0,7 | 51,178215 | 6,806918 |
| 1,0 | **73,6** | FIX_OK | 14 | 0,8 | 51,178074 | 6,807088 |
| 2,0 | 71,0 | FIX_OK | 14 | 0,8 | 51,177933 | 6,807261 |
| 3,0 | 67,5 | FIX_OK | 13 | 0,8 | 51,177792 | 6,807422 |
| 4,0 | 69,0 | FIX_OK | 13 | 0,8 | 51,177662 | 6,807579 |
| 5,0 | 64,5 | FIX_OK | 12 | 0,8 | 51,177517 | 6,807727 |
| 6,0 | 58,6 | FIX_OK | 12 | 0,8 | 51,177433 | 6,807805 |

Ein Fahrrad fährt keine 73,6 km/h. Die Positionsdifferenz zwischen t = 0 und t = 1 s beträgt rechnerisch rund **19,7 m** — die Navigationslösung springt tatsächlich, es handelt sich nicht nur um einen fehlerhaften Geschwindigkeitswert. Der Grund ist Mehrwegeausbreitung und Signalverlust durch die Abdeckung: Der Empfänger rechnet weiter, mit reflektierten und teilweise fehlenden Signalen.

**Das Entscheidende ist nicht der Fehler selbst, sondern seine Unsichtbarkeit:** Während der gesamten Fehlmessung meldet der Empfänger `FIX_OK`, **15 bzw. 14 Satelliten** und einen **HDOP von 0,7–0,8**. Nach jedem gängigen Qualitätskriterium — und nach dem in FR-TEL-05 implementierten Gating (`isValid` & Alter < 3 s & Sats ≥ 4) — ist das ein exzellenter Fix.

~~Damit ist die vorgeschlagene Architektur „GNSS primär, IMU als Notfallebene bei Fix-Verlust“ widerlegt: **Der Umschaltauslöser („Fix verloren“) tritt in genau dem Fehlerfall nicht ein, gegen den er schützen soll.** Das System hätte in dieser Situation aus einer Scheinverzögerung von 73 → 58 km/h eine Bremsung von rund 0,7 m/s² errechnet und in den anderen Sekunden aus den Sprüngen beliebige Werte — bei voller Vertrauenswürdigkeitsanzeige.~~ *(zurückgezogen 17.08.2026)*

Fahrt 6 (22:42, „vor Fix verdecken“) ergänzt die Verfügbarkeitsseite: Nach dem Geräteneustart (`device_timestamp_ms` beginnt bei 3291 ms) vergehen rund **20 s mit `NO_FIX` und `sats = 0`**; anschließend wird ein Fix mit nur 5 Satelliten und HDOP 1,5–1,8 erreicht. In einer GNSS-primären Architektur wäre das Bremslicht für 20 s nach jedem Einschalten funktionslos — bei einer Sicherheitsfunktion nicht vertretbar.

### 6.5 Rauschen der Differentiation

Selbst bei einwandfreiem Empfang ist die aus 1-Hz-Geschwindigkeit differenzierte Beschleunigung verrauscht:

| Kenngröße | Wertebereich über die Fahrten |
|---|---|
| Median \|Δa\| zwischen aufeinanderfolgenden Epochen | 0,17 … 0,33 m/s² |
| 90-%-Perzentil \|Δa\| | 0,53 … 1,97 m/s² |

Bei einer Ansprechschwelle von 2,0 m/s² liegt das 90-%-Perzentil des reinen Differentiationsrauschens im ungünstigen Fall bereits fast auf Schwellenniveau. Ohne zusätzliche Glättung — die wiederum Latenz kostet — wäre eine GNSS-primäre Schwellwertentscheidung nicht flackerfrei.

### 6.6 Zwischenfazit GNSS

| Prüfkriterium | Ergebnis |
|---|---|
| 100 Hz Abtastrate erreichbar? | **Nein** — Datenblattgrenze 10 Hz |
| Latenz ≤ 50 ms (NFR-RT-01)? | **Nein** — 200–400 ms, Faktor 4–8 zu langsam |
| Verfügbarkeit ab Systemstart? | **Nein** — 20 s bis Erstfix gemessen |
| Fehler durch Fix-Gating erkennbar? | **ungeprüft** — der Abschattungsversuch ist ergebnislos, siehe Nachtrag |
| Rauscharm genug für 2,0-m/s²-Schwelle? | **Grenzwertig** — p90 bis 1,97 m/s² |
| Als *langsame* Referenz-/Korrekturgröße geeignet? | **Ja** — 0,1 m/s Geschwindigkeitsgenauigkeit, driftfrei |

Das GNSS ist als **Stütz- und Korrekturgröße** hervorragend geeignet, weil es die eine Eigenschaft besitzt, die der IMU fehlt: Es driftet nicht. Es ist als **primärer Bremssensor** ungeeignet, weil ihm die drei Eigenschaften fehlen, die eine Sicherheitsfunktion braucht: Schnelligkeit, garantierte Verfügbarkeit und erkennbare Fehlerzustände.

---

## 7. Lösungsvarianten

### V-A — GNSS primär, IMU als Rückfallebene

Bremsintensität wird aus der GNSS-Geschwindigkeit abgeleitet; bei Fix-Verlust übernimmt die IMU.

*Vorteile:* physikalisch direkte Messung der Fahrzeugverzögerung, driftfrei, keine Neigungsproblematik.
*Nachteile:* verletzt NFR-RT-01 um Faktor 4–8; 20 s funktionsloses Bremslicht nach dem Start; Rauschen grenzwertig; die IMU-Rückfallebene enthielte weiterhin denselben ungefixten Filter, wäre also kein sicherer Rückfall.

### V-B — IMU als schneller Regelpfad, GNSS als langsame Stützgröße *(empfohlen)*

Die IMU bleibt der Echtzeitpfad, ihr Filter wird jedoch grundlegend überarbeitet. Das GNSS liefert eine langsame, driftfreie Korrekturgröße, die den systematischen Fehler des IMU-Pfads laufend nachführt.

*Vorteile:* NFR-RT-01 bleibt erfüllt (Reaktion im 10-ms-Bereich); volle Funktion ab Sekunde 1 ohne Fix; das GNSS trägt nur dort bei, wo es stark ist (Langzeitgenauigkeit); die Architektur ist die in der Fahrzeug- und Luftfahrttechnik übliche (schneller Inertialpfad + langsame Stützung), also gut belegbar; erlaubt einen sauberen Vorher-Nachher-Nachweis in der Thesis.
*Nachteile:* höherer Implementierungsaufwand als V-C; die Fusionsstufe braucht eigene Tests und eigene Validierung.

### V-C — nur den Filter reparieren, GNSS unverändert bei 1 Hz belassen

*Vorteile:* geringster Aufwand, behebt beide nachgewiesenen Fehlermechanismen vollständig; keine Änderung an GNSS-Konfiguration oder Telemetrie.
*Nachteile:* verschenkt die vorhandene, driftfreie Referenz; die Thesis kann keine Sensorfusion zeigen, obwohl drei Sensoren verbaut sind; keine Absicherung gegen langsam driftende Gyroskop-Nullpunktfehler.

### Vergleich

| Kriterium | V-A | V-B | V-C |
|---|---|---|---|
| NFR-RT-01 (≤ 50 ms) | ✗ | ✓ | ✓ |
| Verfügbarkeit ab Start | ✗ | ✓ | ✓ |
| Robustheit gegen Neigung | ✓ | ✓ | ✓ |
| Robustheit gegen Stöße | ✓ | ✓ | ✓ |
| Robustheit gegen GNSS-Fehlmessung | ✗ | ✓ | ✓ |
| Langzeitgenauigkeit / Driftfreiheit | ✓ | ✓ | ○ |
| Nutzung der vorhandenen Sensorik | ○ | ✓ | ○ |
| Implementierungsaufwand | hoch | mittel–hoch | gering |
| Wissenschaftlicher Gehalt für die Thesis | gering | **hoch** | mittel |

---

## 8. Entscheidung

**Gewählt wird V-B, umgesetzt in zwei getrennt validierbaren Stufen.**

Begründung: V-B ist die einzige Variante, die alle nichtfunktionalen Anforderungen einhält und gleichzeitig beide vorhandenen Geschwindigkeitsinformationen nutzt. Die Zweiteilung folgt dem im Engineering Charter festgelegten Hardware-/Validierungsablauf: Erst wird die Kernfunktion repariert und im Feld nachgewiesen, danach wird die Stützung ergänzt. Damit ist jederzeit ein funktionsfähiger, abgabefähiger Stand vorhanden — auch dann, wenn Stufe 2 aus Zeitgründen entfallen müsste. Die Zweiteilung ist zugleich die Absicherung gegen das Projektrisiko „Fusion funktioniert nicht rechtzeitig“.

---

## 9. Lösungskonzept im Detail

### 9.1 Stufe 1 — Umbau des Bewegungsfilters

Kerngedanke: Neigung, Bremsung und Stoß sind **am Betrag des Beschleunigungsvektors unterscheidbar**, auch wenn sie auf einer einzelnen Achse identisch aussehen.

| Fahrzustand | Betrag ‖a‖ | Physikalische Begründung |
|---|---|---|
| Reine Neigung (statisch oder quasistatisch) | ‖a‖ = g ≈ 9,81 m/s² | Es wirkt ausschließlich die Schwerkraft, nur ihre Verteilung auf die Achsen ändert sich |
| Längsbeschleunigung/Bremsung | ‖a‖ = √(g² + a²) > g | Zur Schwerkraft tritt eine dazu senkrechte Trägheitskomponente |
| Stoß | ‖a‖ ≫ g | Impulsartige Anregung mit Vielfachen von g |

Zahlenbeispiele: Eine Bremsung mit 6 m/s² ergibt ‖a‖ = √(9,81² + 6²) = **11,5 m/s²**, also g + 1,7. Eine Kurvenfahrt mit 20° Schräglage ergibt ‖a‖ = g/cos(20°) = **10,4 m/s²**. Ein Schlagloch erzeugt typischerweise 20–50 m/s².

Daraus folgt ein **Drei-Zustands-Gate** im Filter:

| Zustand | Bedingung | Verhalten |
|---|---|---|
| STATIC | \|‖a‖ − g\| ≤ ~0,35 m/s² | Normale Komplementärfilter-Aktualisierung; der Beschleunigungssensor darf den Nickwinkel korrigieren |
| DYNAMIC | ‖a‖ ≤ g + ~2,5 m/s² | **Nur Gyroskop-Propagation** des Nickwinkels; der Beschleunigungssensor wird von der Winkelschätzung ausgeschlossen, die Längsbeschleunigung wird also nicht mehr „weggeneigt“ |
| SHOCK | ‖a‖ > g + ~2,5 m/s² | Letzte gültige Bremslichtausgabe halten, maximal ~200 ms; der Impuls erreicht die Kennlinie nicht |

Das Verfahren ist als *accelerometer rejection* in der Lageschätzung etabliert (Madgwick-, Mahony- und Fusion-AHRS-Filter verwenden dasselbe Prinzip). Es ist damit für die Thesis literaturgestützt begründbar und nicht als Ad-hoc-Heuristik zu rechtfertigen.

**Entscheidender Vorteil gegenüber den naheliegenden Alternativen:** Das Gate kostet **keine Latenz**. Es ist eine Zustandsentscheidung pro Abtastschritt, keine Filterung des Nutzsignals.

| Alternative zur Stoßunterdrückung | Wirkung | Latenzkosten |
|---|---|---|
| Tiefpass 1. Ordnung, f_c = 8 Hz | τ = 20 ms; ein 25-ms-Impuls passiert noch zu ~71 % | 20 ms, dauerhaft auf dem Nutzsignal |
| Median über 5 Abtastwerte | unterdrückt Einzelspitzen zuverlässig | ~30 ms Gruppenlaufzeit |
| **Norm-Gate** | unterdrückt Impulse vollständig | **0 ms** |

Ergänzende Maßnahmen in Stufe 1:

- **Initialisierung des Nickwinkels** aus `atan2f(a_y, a_z)` beim ersten Abtastwert statt mit 0. Bisher startet der Filter bei 0 rad und braucht mehrere τ, bis er die tatsächliche Einbaulage erreicht — in dieser Zeit ist die Bremserkennung systematisch falsch.
- **Drift-Wächter:** Bleibt das System länger als etwa 5 s im Zustand DYNAMIC, wird eine STATIC-Korrektur erzwungen. Damit kann sich der reine Gyroskop-Pfad nicht unbegrenzt aufintegrieren.
- **Langsame Nullpunktschätzung des Gyroskops** (Zero-Rate-Offset) während STATIC-Phasen, um den Integrationsfehler weiter zu reduzieren.
- **Schwache Nachglättung** (3-Punkt-Median plus Tiefpass ~15 Hz) *nach* dem Gate als zweite Verteidigungslinie gegen Restrauschen — bewusst so schwach ausgelegt, dass NFR-RT-01 unberührt bleibt.

### 9.2 Stufe 2 — GNSS als langsame Stützgröße

**Konfiguration des Empfängers.** Erhöhung auf 5 Hz (`$PMTK220,200*2C`), Anhebung der Baudrate auf 115200 (`PMTK251`) und Reduktion des NMEA-Satzumfangs auf das Benötigte (`PMTK314`). 5 Hz statt der maximal möglichen 10 Hz, weil der Nutzen einer Stützgröße mit der Rate nur schwach steigt, der Bandbreiten- und Strombedarf aber linear. Zu beachten: Die PMTK-Konfiguration ist beim L86 **nicht dauerhaft gespeichert** — sie übersteht einen reinen ESP32-Reset, aber keinen Spannungsausfall. Die Firmware benötigt daher eine Baudraten-Erkennung beim Start (erst 115200 versuchen, bei ausbleibenden gültigen Sätzen auf 9600 zurückfallen und neu konfigurieren).

**Gültigkeitsprüfung.** Ein neues, host-testbares Logikmodul `gnss_speed_ref` gibt eine Referenzbeschleunigung nur dann als gültig aus, wenn mindestens zwei aufeinanderfolgende Fixes vorliegen, `sats ≥ 5`, `HDOP ≤ 2,5` und die Geschwindigkeit über etwa 1,5 m/s liegt (darunter ist die GNSS-Geschwindigkeit selbst verrauscht). Diese Kriterien sind bewusst schärfer als das bestehende FR-TEL-05-Gating, weil die Fahrt 5 gezeigt hat, dass das bestehende Gating zu großzügig ist.

**Fusionsansatz.** Bewusst **kein** Kalman-Filter, sondern eine langsame Biaskorrektur:

$$b_k = b_{k-1} + \frac{\Delta t}{\tau_b}\left[\left(a_{\text{IMU}} - a_{\text{GNSS}}\right) - b_{k-1}\right], \qquad \tau_b \approx 10\ \text{s}$$

$$a_{\text{korrigiert}} = a_{\text{IMU}} - b_k$$

Mit τ_b ≈ 10 s wirkt die Korrektur ausschließlich auf langsame, systematische Abweichungen (Einbaulage, Gyroskop-Nullpunktdrift, Steigungseinfluss). Sie kann die schnelle Bremsdynamik **prinzipbedingt nicht verzögern**, weil sie viel zu träge ist, um ihr zu folgen. Damit bleibt NFR-RT-01 auch mit aktiver Fusion unangetastet — das ist der Grund, warum diese Konstruktion gegenüber einem Kalman-Filter bevorzugt wird: Sie ist analytisch begründbar, mit zwei Zeilen dokumentierbar und ihre Latenzneutralität ist offensichtlich statt simulationsbedürftig.

Die Fusion wird hinter einem Konfigurationsflag geführt und ist zunächst deaktiviert, bis Stufe 1 im Feld nachgewiesen ist. So bleibt der Vorher-Nachher-Vergleich der Stufe 1 unverfälscht.

---

## 10. Auswirkungen auf das Gesamtsystem

| Teilsystem | Auswirkung |
|---|---|
| **Hardware / Elektronik** | Keine. Kein Bauteil, keine Verdrahtung, kein Leiterplattenlayout ändert sich. Der Umbau ist rein softwareseitig. |
| **Stromversorgung** | Bei aktiver Stufe 2 steigt die GNSS-Stromaufnahme leicht (höhere Update-Rate, höhere UART-Aktivität); Größenordnung wenige mA gegenüber den 26 mA Tracking. Für die Energiebilanz (Kap. 5.2 der Bible) unkritisch, aber in der Messung NFR-PWR-02 mitzuführen. |
| **Firmware-Architektur** | `motion_filter` wird umgebaut, bleibt aber hardwarefrei und host-testbar (NFR-TST-01). Neues Logikmodul `gnss_speed_ref` ebenfalls in `lib/logic/`. Die Modultrennung bleibt gewahrt. |
| **Echtzeitverhalten** | Rechenaufwand steigt um eine Wurzel- und wenige Vergleichsoperationen je Abtastschritt — vernachlässigbar gegenüber dem 10-ms-Budget (NFR-RT-04). |
| **Bremskennlinie / FSM** | Unverändert. Der Umbau betrifft ausschließlich den *Eingang* der Kennlinie. Die Bench-Validierung aus Kap. 9.3 bleibt damit gültig. |
| **BLE / Telemetrie** | Unverändert, **solange** kein neues Feld aufgenommen wird. Wird `gnss_accel_ms2` als Vergleichsgröße in den Frame gelegt, steigt die Framegröße und die Schemaversion muss von 2 auf 3 erhöht werden — mit entsprechender Anpassung des iOS-Dekoders und des CSV-Exports. (Offene Entscheidung, s. Abschnitt 12.) |
| **iOS-App** | Nur betroffen, falls der Frame erweitert wird oder ein höher aufgelöster Validierungspfad ergänzt wird (Abschnitt 4.6). |
| **Validierung** | Der Eintrag „Komplementärfilter/Bremserkennung ✅ HW-validiert“ in Kap. 9 der Bible ist durch diesen Feldtest **widerlegt** und auf „❌ Fehlfunktion nachgewiesen, Umbau beschlossen“ zurückzustufen. |
| **Thesis** | Deutlicher Gewinn: aus einem unbelegten „funktioniert“ wird ein belegter Fehlerbefund mit hergeleiteter Ursache, Variantenvergleich, begründeter Entscheidung und quantifizierbarem Wirksamkeitsnachweis. |

---

## 11. Grenzen der Aussagekraft (Ehrlichkeitsabschnitt)

Für die wissenschaftliche Redlichkeit sind folgende Einschränkungen der Auswertung zu nennen:

1. **Die Referenzgröße ist selbst GNSS-basiert.** Die Vergleichsbeschleunigung stammt aus der differenzierten GNSS-Geschwindigkeit desselben Empfängers und ist mit 1 Hz zeitlich grob und verrauscht (Abschnitt 6.5). Sie eignet sich, um grobe Fehlfunktionen im Bereich mehrerer m/s² nachzuweisen — nicht, um die Genauigkeit einer korrekt arbeitenden Erkennung zu bewerten. Für den Wirksamkeitsnachweis nach dem Umbau ist eine höher aufgelöste Referenz vorzusehen.
2. **Die Korrelation ist über alle Zustände gerechnet.** Sie enthält Standzeiten, Beschleunigungsphasen und Konstantfahrt. Ein zustandsselektiver Vergleich (nur Bremsphasen) wäre aussagekräftiger, ändert aber am Befund nichts, da die Einzelereignistabellen (4.3/4.4) denselben Schluss unabhängig stützen.
3. **Die 1-Hz-Persistenz der App unterabtastet den 10-Hz-Datenstrom** (Abschnitt 4.6). Einzelne Zeilenpaare aus `brake_decel_ms2` und `brake_light_pct` stammen aus verschiedenen Momenten. Die Aussagen dieses Berichts stützen sich deshalb nicht auf einzelne Zeilen, sondern auf Verteilungen und mehrfach belegte Ereignisse.
4. **Die Zuordnung einzelner Fehlauslösungen zu Neigung oder Stoß ist teilweise eine begründete Vermutung.** Nur für das Ereignis im Stand (Fahrt 4, t = 3,0 s) ist die Neigungsursache zwingend; bei den Ereignissen während der Fahrt ist die Stoßursache die plausibelste, aber nicht die einzig mögliche Erklärung.
5. **Es wurde nur ein Prüfling, ein Fahrer, eine Strecke und eine Nacht getestet.** Aussagen zur Reproduzierbarkeit über Fahrzeuge, Montagepositionen und Fahrbahnzustände hinweg sind daraus nicht ableitbar.
6. **Die Fahrten 5 und 6 sind keine kalibrierten GNSS-Störversuche**, sondern manuelles Abdecken der Antenne. Sie belegen, dass der beschriebene Fehlerfall real auftreten kann — sie quantifizieren nicht, wie häufig er im normalen Betrieb auftritt.

---

## 12. Noch offene Entscheidungen aus diesem Bericht

| # | Frage | Empfehlung |
|---|---|---|
| E1 | Wird `gnss_accel_ms2` als Vergleichsgröße in den Telemetrie-Frame aufgenommen (Schema v2 → v3)? | Ja — es ist die Voraussetzung für einen quantitativen Vorher-Nachher-Nachweis in der Thesis |
| E2 | Wird ein höher aufgelöster Validierungspfad (10 Hz statt 1 Hz Persistenz) in der App ergänzt? | Ja, als abschaltbarer Validierungsmodus — nur für Messfahrten |
| E3 | Bleibt NFR-RT-01 bei ≤ 50 ms oder wird der Wert nach der neuen Messung dokumentiert revidiert? | Beibehalten; der Umbau kostet keine Latenz |
| E4 | Wird die leere Spalte `temperature_c` aus dem CSV-Export entfernt (23 → 22 Spalten)? | Ja — die Auslassung ist eine bewusste Entscheidung, dann sollte auch die Spalte entfallen |
| E5 | Wird die Stufe-2-Fusion aktiviert ausgeliefert oder hinter einem Flag deaktiviert? | Deaktiviert, bis Stufe 1 feldvalidiert ist |

---

## 13. Vorschläge für Abbildungen und Tabellen in der Thesis

| Nr. | Vorschlag | Aussage |
|---|---|---|
| Abb. F1 | Streudiagramm `brake_decel_ms2` über a_GNSS, alle 939 Punkte, mit Regressionsgerade | Nur mit herausgerechnetem Zeitversatz aussagekräftig, siehe Nachtrag |
| Abb. F2 | Zeitverlauf Fahrt 2, drei Kurven übereinander: v(t), a_GNSS(t), `brake_light_pct`(t) | Zeigt die fünf Bremsungen und dass das Licht bei keiner reagiert |
| Abb. F3 | Ausschnitt um Fahrt 2, t = 51 s: Konstantfahrt mit 100 % Bremslicht | Fehlauslösung, das Spiegelbild zu F2 |
| Abb. F4 | Simulierter Filterverlauf: Sprungantwort des Komplementärfilters mit τ = 0,49 s auf eine 2-s-Bremsung | Belegt den Fehlermechanismus rechnerisch, unabhängig von Messdaten |
| Abb. F5 | Vektordiagramm der drei Fahrzustände mit ‖a‖ = g, > g, ≫ g | Erklärt das Lösungsprinzip anschaulich |
| ~~Abb. F6~~ | ~~Rohdatenauszug Fahrt 5~~ | **entfällt, Befund zurückgezogen (Nachtrag 17.08.2026)** |
| Tab. F1 | App ↔ Strava Soll-Ist-Vergleich (Abschnitt 3) | Nachweis der Messkettengültigkeit |
| Tab. F2 | Nicht erkannte Bremsungen (Abschnitt 4.3) | Quantifizierung Fehler 1. Art |
| Tab. F3 | Fehlauslösungen (Abschnitt 4.4) | Quantifizierung Fehler 2. Art |
| Tab. F4 | Variantenvergleich V-A/V-B/V-C (Abschnitt 7) | Entscheidungsdokumentation nach Engineering Charter |
| Abb. F7 | Wiederholung von F1/F2 nach dem Umbau | Wirksamkeitsnachweis — das Kernstück des Validierungskapitels |

---

## 14. Quellen

- Quectel L86 Hardware Design (Projektablage), Tabelle 1 S. 9 und Tabelle 5 — Update-Rate, Genauigkeiten, TTFF, EASY-Verhalten, Stromaufnahme.
- InvenSense MPU-6050 Product Specification — Messbereiche, Rauschdichte.
- Feldtest-Leitfaden Smart Bike Rear Light, 06.08.2026 — Versuchsplan, Akzeptanzkriterien, Protokollbögen.
- CSV-Exporte der iOS-App, sechs Fahrten vom 06.08.2026 (Schema v2).
- Strava-Aufzeichnungen derselben Fahrten (Referenzmesskette).
- Madgwick, S. O. H.: *An efficient orientation filter for inertial and inertial/magnetic sensor arrays*, 2010 — Prinzip der beschleunigungsbasierten Korrekturunterdrückung.
- Mahony, R.; Hamel, T.; Pflimlin, J.-M.: *Nonlinear Complementary Filters on the Special Orthogonal Group*, IEEE TAC 53(5), 2008.
- ECE R50, § 67 StVZO — normativer Rahmen der Bremslichtfunktion (bereits in der Project Bible Kap. 2.8 geführt).
---

## Nachtrag vom 10.08.2026 — methodische Korrektur und Stand der Befunde

> Dieser Nachtrag korrigiert eine Aussage dieses Berichts und ordnet die Befunde
> in den inzwischen erreichten Entwicklungsstand ein. Der Bericht selbst bleibt
> unverändert: Er dokumentiert den Versuch vom 06.08.2026 und ist die
> Beweisgrundlage der Falsifikation. Kanonische Fassung des Projektstands:
> Project Bible v0.19, Kap. 9.4 und 9.5.

### N.1 Die Korrelation r = −0,132 ist als Gütemaß nicht belastbar

In Kapitel 4.1 wird die Pearson-Korrelation zwischen der aus der GNSS-Geschwindigkeit
differenzierten Referenzbeschleunigung und `brake_decel_ms2` mit **r = −0,132
(n = 939)** angegeben und als „über alle sechs Fahrten stabil, kein Zufallsbefund"
bewertet. Kapitel 13 führt sie als Abb. F1 („Kernaussage in einem Bild").

**Diese Rechnung wurde ohne Korrektur des Zeitversatzes zwischen den beiden
Signalketten durchgeführt und ist als Gütemaß deshalb nicht belastbar.** Die
GNSS-Referenzkette trägt eine Transportlatenz von etwa ein bis zwei Sekunden
(interne Lösungslatenz des L86 von 100–300 ms, s. Kapitel 6.3, zuzüglich
`PERIOD_GNSS_MS` = 1000 ms Abtastung in der Firmware), und die verwendete
Rückwärtsdifferenz erzeugt einen weiteren konstruktionsbedingten Versatz von einer
halben Abtastperiode. Bei einem Vergleich zweier Ketten mit unterschiedlicher
Latenz ist die Korrelation bei Nullversatz kein Gütemaß.

Rechnet man dieselben Fahrten 1–4 mit Versatzkorrektur, steigt r vom Median −0,18
auf den Median **+0,29**. Kapitel 11 dieses Berichts nennt sechs Einschränkungen der
Aussagekraft; die fehlende Latenzkorrektur ist dort **nicht** aufgeführt und
ergänzt diese Liste als siebte.

**Was unberührt bleibt.** Die Falsifikation selbst steht auf drei voneinander
unabhängigen Beinen und ist durch diesen Fehler nicht betroffen:

1. der analytische Nachweis aus dem Quelltext (Fehlermechanismus A, Kapitel 5.2:
   τ = α·Δt/(1−α) = 0,49 s gegenüber einer typischen Bremsdauer von 1,85 s),
2. die Verteilung der Bremslicht-Duty (93–100 % der Zeilen je Fahrt auf dem
   Schlusslicht-Grundwert, obwohl Fahrt 2 aus fünf vollständigen Bremsungen bis zum
   Stillstand bestand, Kapitel 4.2) und die Einzelereignisse in Kapitel 4.3/4.4
   (eine reale Verzögerung von −5,75 m/s² erzeugt firmwareseitig 0,18 m/s²),
3. die spätere Prüfstandsmessung vom 07.08.2026 (Bench-Experiment D: Legacy-Filter
   3,924 → 0,000 m/s² gegenüber neuem Filter 3,9 → 3,836 m/s²).

**Für die Thesis:** Die Zahl −0,132 darf nicht als Kennzahl der Erkennungsgüte
geführt werden. Verwendbar sind die Duty-Verteilung, die Einzelereignisse und der
Prüfstandsvergleich. Der Fehler selbst ist als methodischer Befund verwertbar —
er gehört in die Methodenkritik.

### N.2 Stand der in Kapitel 12 offenen Entscheidungen E1–E5

Alle fünf sind entschieden und in Project Bible Kap. 10 sowie im Decision Log
kanonisch geführt: **E1** `gnss_accel_ms2` ins Frame (umgesetzt, Schema v3),
**E2** 10-Hz-Aufzeichnung (umgesetzt, App-Validierungsmodus), **E3** NFR-RT-01
bleibt bei 50 ms, **E4** `temperature_c` aus dem CSV-Export entfernt (im
Persistenzmodell erhalten), **E5** Stufe-2-Fusion bleibt deaktiviert.

Die Kürzel E1–E5 bleiben unverändert; die kollidierenden iOS-Nachtragsentscheidungen
wurden in **V3-1 bis V3-4** umbenannt.

### N.3 Wirksamkeitsnachweis (Abb. F7) ist erbracht

Der in Kapitel 13 vorgeschlagene Vorher-Nachher-Nachweis liegt vor: Messfahrt vom
08.08.2026, ausgewertet in `docs/Messfahrt_2026-08-08_Auswertung.md`. Kernwerte:
alle **9 von 9** aus der GNSS-Referenz identifizierten Bremsvorgänge angezeigt;
r = **+0,85** bei identischem Auswerteverfahren und festem Versatz von −2,0 s
gegenüber einem Median von **+0,15** für die Fahrten 1–4 dieses Berichts; keine
Fehlauslösung in 25 Beschleunigungsepochen; der Ruhesockel von rund 3,0 m/s² ist
nicht mehr nachweisbar.

Einschränkung: Es handelt sich um **eine** Fahrt von knapp drei Minuten, nicht um
die in Kapitel 13 vorgesehene Wiederholung des vollständigen Sechs-Fahrten-Protokolls.
Diese ist nach dem Umfangsschnitt vom 10.08.2026 nicht mehr Teil der Arbeit
(Project Bible Kap. 12.2); die Fehlauslösungsrate ist daher nicht belastbar
hochrechenbar.