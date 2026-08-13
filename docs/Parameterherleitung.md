# Herleitung der Parameterwerte

**Smart Bike Rear Light · Bachelorarbeit Krahl · erstellt 10.08.2026**
**Zweck: Für jeden festgelegten Zahlenwert der Firmware ist angegeben, wie er zustande kam und womit er heute belegt ist.**

---

## Wie dieses Dokument zu lesen ist

Jeder Wert trägt **zwei** Angaben, weil das zwei verschiedene Fragen sind:

**Herkunft** — wie der Wert tatsächlich entstanden ist, zum Zeitpunkt der Festlegung:

- **P — physikalisch hergeleitet:** folgt zwingend aus einer Rechnung oder einer anderen Festlegung.
- **N — aus einer Norm übernommen:** die Zahl stammt aus einem Regelwerk.
- **V — aus einem Versuch bestimmt:** die Zahl wurde gemessen.
- **A — plausibel angenommen:** eine begründete Festlegung ohne Herleitung.

**Absicherung** — was den Wert heute stützt. Ein Wert kann als **A** entstanden und später durch Messung oder Norm bestätigt worden sein. Das ist etwas anderes, als ihn von Anfang an hergeleitet zu haben, und wird hier auch so ausgewiesen.

> **Grundsatz.** Elf der sechzehn Werte sind bei der Erstellung der Anforderungsspezifikation am 21.07.2026 als **Annahme (A)** festgelegt worden. Das ist für eine Erstauslegung normal und kein Mangel — es wird hier nur nicht länger verschwiegen. Was sich seither geändert hat, ist die Absicherung: Sieben Werte sind inzwischen durch die Messfahrt vom 08.08.2026 oder durch eine nachträgliche Rechnung gestützt, vier durch eine normative Analogie plausibilisiert. Drei Werte bleiben reine Annahmen ohne jeden Beleg; sie sind unten ausdrücklich als solche geführt.

**Normative Einordnung vorab.** Für Fahrräder existiert in Deutschland **keine** Regelung, die Verzögerungsschwellen, Haltezeiten oder Reaktionszeiten für eine Bremslichtfunktion vorgibt. § 67 Abs. 4 StVZO **erlaubt** die Bremslichtfunktion ausdrücklich („Schlussleuchten dürfen zusätzlich mit einer Bremslichtfunktion ausgerüstet sein"), macht aber keine quantitativen Vorgaben; blinkende Schlussleuchten sind dort **untersagt**. Jede unten genannte UN/ECE-Regelung gilt für Kraftfahrzeuge und ist deshalb **ausschließlich als ingenieurtechnische Analogie** verwendbar, nicht als Rechtsgrundlage. Das ist in der Arbeit an jeder Stelle so zu kennzeichnen.

> **Verifikationsvorbehalt.** Die normativen Fundstellen wurden über EUR-Lex recherchiert. Vor der wörtlichen Übernahme in die Arbeit ist jede Fundstelle einmal an der amtlichen Fassung gegenzuprüfen (§ 67 StVZO über `gesetze-im-internet.de`, UN-Regelungen über die konsolidierte EUR-Lex-Fassung). Zwei Fundstellen sind unten ausdrücklich als **unverifiziert** markiert.

---

## Übersicht

| # | Parameter | Wert | Herkunft | Absicherung heute |
|---|---|---|---|---|
| 1 | `BRAKE_ON_MS2` | 2,0 m/s² | **A** | V (Feld) + N (Analogie R13-H) — Reserve nur 0,32 m/s² |
| 2 | `BRAKE_FULL_MS2` | 5,0 m/s² | **A** | P (Überschlagsgrenze) + Literatur + V |
| 3 | `BRAKE_OFF_MS2` | 1,5 m/s² | **A** | N fordert *eine* Hysterese, nicht diesen Wert · **V widerspricht teilweise** |
| 4 | `BRAKE_MIN_HOLD_MS` | 300 ms | **A** | P (Blondel-Rey) + V (Feld) |
| 5 | `BLINKER_TIMEOUT_MS` | 60 s | **A** | **keine** — Annahme, im Betrieb unauffällig |
| 6 | `LONGPRESS_MS` | 5 s | **A** | **keine** — Annahme, im Betrieb unauffällig |
| 7 | `INIT_TIMEOUT_MS` | 5 s | **A** | P (Größenordnungsvergleich) |
| 8 | NFR-RT-01 | 50 ms | **A** | Literatur (Green 2000) + N (Analogie) + V |
| 9 | NFR-RT-04 | 10 ms | **P** | folgt zwingend aus 100 Hz · V bestätigt |
| 10 | `PERIOD_IMU_MS` | 10 ms (100 Hz) | **A** | P (Stoßdauer + Antialiasing) |
| 11 | `RINGBUFFER_FRAMES` | 600 (60 s) | **A** | P (Speicherbudget) — Obergrenze, nicht Bedarf |
| 12 | Laufzeit | ~8 h | **P** | Rechnung mit zwei Annahmen · **nicht gemessen** |
| 13 | `BLINK_FREQ_HZ` | 1,5 Hz | **N** | R53 § 6.3.8.1 (90 ± 30/min) — Analogie |
| 14 | `ESS_BLINK_FREQ_HZ` | 4,0 Hz | **N** | R53 § 6.14.7.1 (4,0 ± 1,0 Hz) — Analogie |
| 15 | `ESS_ON/OFF_MS2` | 5,0 / 3,0 m/s² | **A** | bewusste Abweichung von N (6,0 / 2,5) — begründet |
| 16 | `TAILLIGHT_DUTY_PCT` | 20 % | **A** | **keine** — photometrisch unbelegt |

---

## 1. Ansprechschwelle `BRAKE_ON_MS2` = 2,0 m/s²

**Herkunft: A — angenommen.** Bei der Erstellung der Anforderungsspezifikation festgelegt; in der Project Bible bis v0.20 ohne Herleitung geführt.

**Absicherung heute:**

*Normative Analogie.* UN-Regelung Nr. 13-H, Abs. 5.2.22.2 (Fassung ABl. EU 2023/401) legt für Pkw fest, dass bei automatisch angeforderter oder rekuperativer Bremsung ab **1,3 m/s²** das Bremslichtsignal erzeugt werden **muss**, darunter erzeugt werden **darf**. Die gewählten 2,0 m/s² liegen darüber, sind also gegenüber dem Kfz-Maßstab **konservativer** — das System meldet später, nicht früher. Für Nutzfahrzeuge nennt UN-R13 Abs. 5.2.1.30.3 eine Unterdrückungserlaubnis unterhalb **0,7 m/s²**. Ein zahlenmäßiges Verbot, unterhalb einer bestimmten Verzögerung zu melden, existiert in keiner der beiden Regelungen; die Verbotstatbestände sind dort funktional formuliert (Motorschleppmoment, Roll- und Luftwiderstand, Fahrbahnneigung).

*Messtechnische Absicherung (Messfahrt 08.08.2026).* Von neun aus der GNSS-Referenz identifizierten Bremsvorgängen erreichte das Firmwaresignal in **sechs** Fällen 2,0 m/s²; alle neun wurden angezeigt, die übrigen drei über die Grundhelligkeitsstufe der Kennlinie. Die Störgrundlinie außerhalb der Bremsvorgänge erreicht bei 25–35 km/h im 99-%-Quantil **1,68 m/s²** (Bible Kap. 9.5.6). Der Sicherheitsabstand zur Ansprechschwelle beträgt damit **0,32 m/s²** — er ist vorhanden, aber knapp, und er ist geschwindigkeitsabhängig.

*Effektiv wirksamer Wert.* Durch die verbleibende Filterdämpfung von 5,9 % wird die nominelle Schwelle real erst bei etwa **2,13 m/s²** erreicht (Bible Kap. 9.5, `stufe1_normgate.md`). In der Arbeit ist dieser Wert anzugeben, nicht 2,0.

**Formulierungsvorschlag für die Thesis:** „Die Ansprechschwelle wurde zu 2,0 m/s² angenommen. Sie liegt oberhalb der in UN-R13-H für Kraftfahrzeuge festgelegten Signalisierungsschwelle von 1,3 m/s² und damit auf der sicheren Seite gegenüber Fehlauslösungen. Die Messfahrt bestätigt die Wahl: Alle neun Referenzbremsungen wurden angezeigt, und die Störgrundlinie bleibt mit einem 99-%-Quantil von 1,68 m/s² unterhalb der Schwelle. Der verbleibende Abstand von 0,32 m/s² ist gering und geschwindigkeitsabhängig; er ist als Grenze der Auslegung ausgewiesen."

---

## 2. Sättigung `BRAKE_FULL_MS2` = 5,0 m/s²

**Herkunft: A — angenommen.**

**Absicherung heute: physikalisch und literaturgestützt — das ist der am besten belegte Wert der ganzen Auslegung.**

*Physikalische Obergrenze.* Die maximal erreichbare Verzögerung eines Fahrrads ist nicht durch die Bremse begrenzt, sondern durch den Überschlag um den Vorderradaufstandspunkt. Aus der Momentenbilanz um diesen Punkt folgt bei verschwindender Aufstandskraft am Hinterrad

$$a_{\max} = g \cdot \frac{b}{h}$$

mit *b* als horizontalem Abstand des Gesamtschwerpunkts von Fahrer und Rad zum Vorderradaufstandspunkt und *h* als Schwerpunkthöhe. Die Herleitung ist elementare Starrkörperstatik und in der Arbeit selbst zu führen; die Literatur bestätigt sie numerisch:

| Quelle | b | h | a_max |
|---|---|---|---|
| Whitt/Wilson, *Bicycling Science*, 2. Aufl., MIT Press 1982 | 60 cm | 120 cm | 0,50 g ≈ 4,9 m/s² |
| Wilson, *Bicycling Science*, 3. Aufl., MIT Press 2004 | — | — | 0,56 g ≈ 5,5 m/s² |
| Broker/Hottman, *Bicycle Accidents, Crashes, and Collisions*, 2. Aufl. 2017 | 63,5 cm | 102 cm | 0,63 g ≈ 6,2 m/s² |
| Stein-Cadenbach, *ABS für das Fahrrad?*, Fahrradzukunft 26 (2018) | — | — | 5,5–6,5 m/s² |

*Gemessene Vollbremsungen in der Literatur.* Lyubenov et al., Engineering Proceedings 70 (2024), Nr. 26, DOI 10.3390/engproc2024070026: Erwachsenenrad mit Scheibenbremse, beide Bremsen, trockener Asphalt, 40 Versuche — **Mittelwert 5,08 m/s²** (Spanne 4,68–5,62). Bulla/Schnädelbach, Verkehrsunfall und Fahrzeugtechnik 46 (2008), Heft 6, S. 193–200: bis **6,8 m/s²** mit hydraulischer Scheibenbremse. Famiglietti et al., SAE 2020-01-0876: 0,40–0,71 g über acht Fahrradtypen.

*Messtechnische Absicherung.* In der Messfahrt erreichten **zwei von neun** Referenzbremsungen die Sättigung; die höchste gemessene Referenzverzögerung betrug 2,99 m/s², das höchste Firmwaresignal 7,61 m/s². Die Sättigung wird also selten, aber nicht nie erreicht — genau das ist bei einer Notbremsschwelle beabsichtigt.

**Formulierungsvorschlag:** „Die Sättigung wurde zu 5,0 m/s² angenommen und lässt sich nachträglich physikalisch begründen: Sie entspricht dem unteren Rand der Überschlagsgrenze eines Fahrrads, die aus der Momentenbilanz zu a_max = g·b/h folgt und je nach Schwerpunktlage bei 0,50 bis 0,63 g liegt. Gemessene Vollbremsungen erreichen im Mittel 5,08 m/s². Oberhalb von 5,0 m/s² liegen damit nur noch Notbremsungen — die Kennlinie erreicht ihre volle Helligkeit also genau dort, wo die Bremsung physikalisch nicht mehr steigerbar ist."

---

## 3. Ausschalthysterese `BRAKE_OFF_MS2` = 1,5 m/s²

**Herkunft: A — angenommen**, als 25 % unterhalb der Einschaltschwelle.

**Absicherung heute — hier ist der Befund unbequem und gehört genau so in die Arbeit.**

*Das Prinzip ist normativ gedeckt, der Wert nicht.* UN-R13-H Abs. 5.2.22.2 fordert wörtlich: *„An appropriate measure (e.g. switch-off-hysteresis, averaging, time delay) shall be implemented in order to avoid fast changes of the signal resulting in flickering of the stop lamps."* Die Regelung verlangt also **eine** Maßnahme gegen Flackern und nennt die Ausschalthysterese ausdrücklich als eine davon — sie nennt aber **keinen Zahlenwert**.

*Die Messung stützt den Wert nur teilweise.* Die Störgrundlinie erreicht bei 25–35 km/h ein 99-%-Quantil von 1,68 m/s² und liegt damit **oberhalb** der Ausschaltschwelle von 1,5 m/s². Das heißt: Im Reisegeschwindigkeitsbereich überschreitet das Signal in mehr als einem Prozent der Abtastungen die Rückfallschwelle, auch wenn gar nicht gebremst wird. Ist die Zustandsmaschine einmal im Zustand *Bremslicht*, setzt jede solche Abtastung den Haltezeitzähler zurück und verzögert den Rückfall auf das Schlusslicht.

Für die Ansprechschwelle von 2,0 m/s² besteht ein Störabstand von 0,32 m/s²; für die Ausschaltschwelle von 1,5 m/s² besteht **kein** Störabstand. Beide Aussagen folgen aus derselben Tabelle in Bible Kap. 9.5.6.

*Warum das im Feld nicht aufgefallen ist.* Die beobachteten Anzeigevorgänge dauerten im Median 1,6 s bei einer Spanne von 0,1 bis 2,3 s — kein Fall lief erkennbar zu lang. Die Wirkung ist also vorhanden, aber klein gegenüber der Dauer der Bremsvorgänge selbst. Zusätzlich war während der Messfahrt die Mindesthaltezeit ohnehin unwirksam (Mangel M-01, seit 10.08.2026 behoben) — nach der Korrektur kann sich dieser Effekt stärker zeigen als in den vorliegenden Daten.

**Formulierungsvorschlag:** „Die Ausschalthysterese wurde zu 1,5 m/s² angenommen, also 25 % unterhalb der Einschaltschwelle. Das Vorhandensein einer Hysterese ist in UN-R13-H ausdrücklich gefordert, ihr Wert dagegen nicht geregelt. Die Messfahrt zeigt, dass dieser Wert im Reisegeschwindigkeitsbereich keinen Störabstand zur gemessenen Grundlinie besitzt (99-%-Quantil 1,68 m/s²). Der Rückfall auf das Schlusslicht ist damit nicht gegen die Störgrundlinie abgesichert. Der Effekt war in den Felddaten nicht als überlange Anzeige erkennbar, ist aber als Auslegungsgrenze zu führen."

---

## 4. Mindesthaltezeit `BRAKE_MIN_HOLD_MS` = 300 ms

**Herkunft: A — angenommen.**

**Absicherung heute: physikalisch begründbar über die Wahrnehmung.**

*Blondel-Rey-Beziehung.* Für kurze Lichtimpulse ist nicht die stationäre Lichtstärke *I* wahrnehmungswirksam, sondern die effektive Lichtstärke

$$I_{\mathrm{eff}} = I \cdot \frac{t}{a + t}$$

mit der visuellen Zeitkonstante *a*. Für Nachtbeobachtung ist international *a* = 0,2 s abgestimmt (Transportation Research Record 1111 (1987), Beitrag 9, unter Bezug auf die IALA-Empfehlung; wörtlich: *„for nighttime observation the visual time constant, a, used for calculations of effective intensity, should be equal to 0.2"*). Daraus folgt unmittelbar:

| Leuchtdauer *t* | I_eff / I |
|---|---|
| 50 ms | 20 % |
| 100 ms | 33 % |
| 200 ms | 50 % |
| **300 ms** | **60 %** |
| 1000 ms | 83 % |

Bei 300 ms erreicht das Bremslicht also 60 % seiner stationären Wahrnehmungswirkung; bei 100 ms wären es nur 33 %. Die Haltezeit ist damit **keine willkürliche Zahl, sondern der Punkt, ab dem der Wahrnehmungsverlust gegenüber einem Dauersignal auf unter die Hälfte begrenzt bleibt.**

*Wichtige Einschränkung.* Eine Quelle, die eine „Mindestleuchtdauer für zuverlässige Wahrnehmung" als solche angibt, existiert nach der Recherche nicht. Die Formulierung in der Arbeit muss deshalb lauten: „bei 300 ms erreicht die effektive Lichtstärke nach Blondel-Rey 60 % des stationären Werts" — **nicht** „ab 300 ms wird ein Signal wahrgenommen".

*Messtechnische Absicherung.* Von zwölf Anzeigevorgängen der Messfahrt waren **zwei kürzer als 0,3 s** (0,1 s und 0,2 s) — genau die Fälle, die die Haltezeit auffangen soll und im Aufzeichnungsstand nicht auffangen konnte. Der Median lag bei 1,6 s. Der Bedarf ist damit gemessen belegt, nicht nur angenommen.

*Wegbezug.* Ein mit 50 km/h folgendes Fahrzeug legt in 300 ms 4,2 m zurück.

---

## 5. Blinker-Selbstabschaltung `BLINKER_TIMEOUT_MS` = 60 s

**Herkunft: A — angenommen. Absicherung: keine.**

Es gibt weder eine Norm noch einen Versuch. Die Begründung ist rein funktional: Ein Abbiegevorgang dauert wenige Sekunden. Kraftfahrzeuge stellen den Fahrtrichtungsanzeiger über die Lenkbewegung selbsttätig zurück; am Fahrrad ist das mit der vorhandenen Sensorik nicht darstellbar, weil eine Lenkwinkelmessung fehlt. Die Zeitabschaltung ist deshalb ein reiner Vergessens-Schutz: Sie verhindert, dass ein unbemerkt aktiver Blinker den nachfolgenden Verkehr dauerhaft fehlleitet und den Akku entleert. 60 s liegen um mehr als eine Größenordnung über der Dauer eines Abbiegevorgangs und weit unterhalb einer Fahrtdauer.

**Ehrliche Aussage für die Arbeit:** „Angenommen; im Betrieb nicht als störend aufgefallen. Der Wert ist nicht optimiert, und eine kürzere Abschaltzeit wäre ebenso vertretbar. Eine Rückstellung über die Fahrdynamik — etwa über die integrierte Gierrate nach abgeschlossenem Abbiegevorgang — wäre die technisch bessere Lösung und ist als Ausblick zu nennen; sie erfordert die Z-Achse des vorhandenen Gyroskops und wäre ohne zusätzliche Hardware umsetzbar."

---

## 6. Langdruck Warnblinker `LONGPRESS_MS` = 5 s

**Herkunft: A — angenommen. Absicherung: keine.**

Begründung: Der Warnblinker ist der einzige Zustand, der nicht versehentlich erreichbar sein darf, weil er beide Blinker dauerhaft aktiviert und damit ein widersprüchliches Signalbild erzeugt. Die verwendete ASK-Fernbedienung sendet kein Kombinationssignal, ein gleichzeitiges Drücken zweier Tasten ist auf der Funkstrecke also nicht auswertbar (Bible Kap. 10). Damit bleibt nur die Betätigungsdauer als Unterscheidungsmerkmal. 5 s liegen weit oberhalb jeder unbeabsichtigten Betätigung und sind vom Nutzer bewusst durchzuhalten.

**Ehrliche Aussage:** „Angenommen und im Betrieb bestätigt. Der Wert ist eine Fehlbedienungsschwelle, kein optimierter Parameter; das Wiederholintervall der Fernbedienung, aus dem sich eine untere Grenze ableiten ließe, wurde nicht gemessen (abgegrenzt, Bible Kap. 12.2)."

---

## 7. Initialisierungs-Timeout `INIT_TIMEOUT_MS` = 5 s

**Herkunft: A — angenommen. Absicherung: P, als Größenordnungsvergleich.**

Der Wert ist keine Auslegungsgröße, sondern eine **Obergrenze**: Er legt fest, wie lange die Zustandsmaschine im Zustand INIT auf die Sensorinitialisierung wartet, bevor sie in den degradierten Betrieb übergeht (FR-STA-01/02). Die tatsächlichen Initialisierungszeiten liegen um mehr als zwei Größenordnungen darunter — Buskonfiguration, WHO_AM_I-Prüfung und Registerkonfiguration der beiden I²C-Sensoren sind Transaktionen im Millisekundenbereich.

Sicherheitsrelevant ist die Frage, ob das Rücklicht während dieser Zeit dunkel bleibt. Es bleibt es **nicht**: FR-TL-03 schreibt für den INIT-Zustand ein Diagnose-Blinken mit 2,0 Hz vor, das Fahrzeug ist also von Anfang an sichtbar. Der Timeout kann deshalb großzügig gewählt werden, ohne die Sicherheit zu berühren.

**Ehrliche Aussage:** „Angenommen als großzügige Obergrenze. Die tatsächlich benötigte Initialisierungszeit liegt um mehr als zwei Größenordnungen darunter; der Wert ist kein Auslegungspunkt, sondern eine Abbruchbedingung. Die Sichtbarkeit während des INIT-Zustands ist durch das Diagnose-Blinken nach FR-TL-03 sichergestellt."

---

## 8. Reaktionszeit NFR-RT-01 ≤ 50 ms

**Herkunft: A — angenommen.**

**Absicherung heute: über eine Verhältnisbetrachtung, und die ist stark.**

*Menschliche Reaktionszeit.* Green, M.: *„How Long Does It Take to Stop?" Methodological Analysis of Driver Perception-Brake Times*, Transportation Human Factors 2 (2000), Nr. 3, S. 195–216, DOI 10.1207/STHF0203_1. Wörtlich: *„Response to unexpected, but common signals, such as a lead car's brake lights, is about 1.25 sec."* Diese Quelle ist für den vorliegenden Fall **direkt** gültig und nicht bloß analog: Der Nachfolgeverkehr hinter dem Fahrrad ist genau die untersuchte Population, und das Bremslicht des vorausfahrenden Fahrzeugs ist genau der untersuchte Reiz.

Damit gilt: 50 ms sind **4 % der Perzeptions-Reaktionszeit** des Empfängers. Bei 30 km/h (8,33 m/s) entsprechen sie 0,42 m Weg, gegenüber 10,4 m allein durch die menschliche Reaktion. Die Signalkette ist also nicht der begrenzende Faktor.

*Normative Analogie.* UN-R13-H, Anhang 3, Abs. 3.1.1 lässt für die Betriebsbremsanlage eines Pkw eine Ansprechzeit von **0,6 s** zu. Die geforderten 50 ms sind um den Faktor 12 kürzer. Der Vergleich ist als Größenordnungsvergleich zu führen, nicht als Vergleich gleichartiger Größen — die 0,6 s betreffen den Bremskraftaufbau, nicht die Lichtansteuerung.

*Messtechnische Absicherung.* Am Prüfstand gemessen: ≤ 10 ms bei sprunghafter Anregung, 20 ± 10 ms bei Gefälle mit anschließender Bremsung (`bench_run_notes.md`, Schritt A6.3). Die Anforderung wird mit mindestens Faktor 2,5 unterschritten. Die Messauflösung beträgt 10 ms; Angaben unterhalb davon sind als „≤ 10 ms" zu führen.

---

## 9. Schleifenzeit NFR-RT-04 < 10 ms

**Herkunft: P — physikalisch hergeleitet. Der einzige Wert der Liste, der tatsächlich zwingend folgt.**

Bei einer Abtastrate von 100 Hz beträgt die Abtastperiode 10 ms. Ein kooperativer Scheduler ohne Nebenläufigkeit muss seinen Durchlauf innerhalb dieser Periode abschließen, sonst geht die nächste Abtastung verloren und der Komplementärfilter arbeitet mit lückenhaften Daten. Die Anforderung ist damit keine eigenständige Festlegung, sondern eine **Folge** der Abtastrate (Punkt 10).

*Messtechnische Absicherung.* Prüfstand 0,651 ms Worst Case, Fahrbetrieb 6,7 ms Worst Case bei 0,00 % der Fenster über 10 ms (Bible Kap. 9.5.5). Die Anforderung ist erfüllt; die Ursachenzuordnung der 1-Hz-Spitze ist zurückgenommen (ebenda).

---

## 10. Abtastrate `PERIOD_IMU_MS` = 10 ms (100 Hz)

**Herkunft: A — angenommen. Absicherung: P, aus zwei unabhängigen Bedingungen.**

*Untergrenze aus der Stoßerkennung.* Fahrbahnstöße dauern 10 bis 30 ms (Feldtestbericht 06.08.2026, Kap. 5.3). Damit ein Stoß als solcher klassifiziert und nicht als Bremsung verarbeitet wird, muss er mit mindestens einer, besser mehreren Abtastungen erfasst werden. Bei 100 Hz sind das ein bis drei Abtastungen — gerade ausreichend. Eine niedrigere Rate würde Stöße unterabtasten, und genau daraus entstand Fehlermechanismus B der Erstauslegung.

*Antialiasing.* Der digitale Tiefpass des MPU-6050 ist auf 44 Hz konfiguriert und liegt damit unterhalb der halben Abtastrate von 50 Hz (Bible Kap. 6, Entscheidung vom 07.08.2026). Diese Bedingung ist nur bei 100 Hz oder mehr erfüllbar; bei der ursprünglichen Konfiguration mit 260 Hz Bandbreite war sie verletzt, was als Unterabtastung ohne Antialiasing nachgewiesen und behoben wurde.

*Obergrenze.* Eine höhere Rate hätte den Rechenzeitbedarf und die Stromaufnahme erhöht, ohne einen Nachweisgewinn zu bringen; der gemessene Rechenzeitbedarf liegt bei 92 µs im Median gegenüber 10 ms Budget, die Reserve wäre also vorhanden gewesen. 100 Hz ist damit die kleinste Rate, die beide Bedingungen erfüllt.

---

## 11. Ringpuffer `RINGBUFFER_FRAMES` = 600 (≈ 60 s bei 10 Hz)

**Herkunft: A — angenommen. Absicherung: P, aber als Obergrenze, nicht als Bedarf.**

Die Rechnung: 600 Rahmen × 113 Byte = **67 800 Byte ≈ 66 kB**. Das sind rund 63 % des gemessenen RAM-Bedarfs von 106 912 Byte und etwa 13 % des SRAM des ESP32. Der Puffer ist damit der mit Abstand größte Einzelverbraucher im Speicherbudget.

**Ehrlich ist deshalb folgende Aussage:** Die 60 s sind **nicht** aus einer gemessenen oder erwarteten Verbindungsausfalldauer abgeleitet. Es liegt keine Statistik über BLE-Abrissdauern vor. Die 60 s sind das, was sich im Speicherbudget unterbringen ließ, ohne den BLE-Stack zu gefährden — eine Obergrenze also, keine Bedarfsgröße. Dass die Nachlieferung im Betrieb funktioniert, ist app-seitig verifiziert; dass 60 s ausreichen, ist nicht belegt.

Der einzige Datenpunkt: Während der Messfahrt gingen **3 von 1776 Rahmen** verloren (0,17 %), es kam also zu keinem längeren Abriss.

---

## 12. Laufzeit ≈ 8 h

**Herkunft: P — Rechenergebnis, keine Anforderung. Absicherung: unvollständig.**

2000 mAh Nennkapazität geteilt durch 0,24 A Akkustrom im Dauerbetrieb mit Schlusslicht ergibt rund 8,3 h (Bible Kap. 5.2). Die Zahl ist also hergeleitet — **aber zwei Eingangsgrößen der Herleitung sind selbst Annahmen:**

- die Flussspannung der COB-LED mit V_f = 2,2 V (Mittelwert der Produktangabe 2,0–2,4 V; ein Datenblatt des Herstellers liegt nicht vor),
- der Wirkungsgrad des MT3608 mit η = 0,90 (Datenblattwert, nicht unter realer Last gemessen).

Hinzu kommt, dass die Nennkapazität des Akkus nicht geprüft ist und die Entladeschlussspannung des DW01 die nutzbare Kapazität weiter reduziert.

**Ehrliche Aussage:** „Rechnerisch rund 8 h im Dauerbetrieb mit Schlusslicht. Der Wert ist nicht gemessen, und zwei seiner Eingangsgrößen sind selbst Annahmen. Er ist als Größenordnung zu führen, nicht als Kennwert. Die messtechnische Verifikation ist als NFR-PWR-02 offen."

---

## 13. Blinkfrequenz `BLINK_FREQ_HZ` = 1,5 Hz

**Herkunft: N — aus einer Norm übernommen.** Der Quelltext nennt seit der Erstfassung ECE R6 als Bezug.

**Absicherung heute:** Der Wert von 90 ± 30 Impulsen pro Minute ist primär belegt in UN-Regelung Nr. 53, Abs. 6.3.8.1 (ABl. EU 2020/31), wörtlich: *„The light flashing frequency shall be 90 ± 30 times per minute."* 90/min entsprechen genau 1,5 Hz, das Toleranzband 60–120/min entspricht 1,0–2,0 Hz.

**Zwei Einschränkungen, die genannt werden müssen:** Erstens ist die im Quelltext genannte Fundstelle **ECE R6 nicht verifiziert** worden; belegt ist die wortgleiche Anforderung in R53 (Krafträder). Der Bezug im Code ist entsprechend zu korrigieren oder zu prüfen. Zweitens gilt keine dieser Regelungen für Fahrräder. Die StVZO enthält **keine** Vorschrift über Fahrtrichtungsanzeiger an einspurigen Fahrrädern; § 67a betrifft Fahrrad**anhänger**, nicht das Fahrrad selbst. Der Wert ist damit eine Analogie, begründet über die Wiedererkennbarkeit des Signalbilds für andere Verkehrsteilnehmer.

---

## 14. ESS-Blinkfrequenz `ESS_BLINK_FREQ_HZ` = 4,0 Hz

**Herkunft: N — aus einer Norm übernommen. Absicherung: primär belegt.**

UN-Regelung Nr. 53, Abs. 6.14.7.1 (ABl. EU 2020/31), wörtlich: *„All the lamps of the emergency stop signal shall flash in phase at a frequency of 4,0 ±1,0 Hz."* Der gewählte Wert liegt exakt auf dem Nennwert. Die entsprechende Anforderung in UN-R48 § 6.23 für Pkw ist inhaltlich identisch, die Absatznummer konnte jedoch **nicht am Primärtext verifiziert** werden — in der Arbeit ist deshalb R53 zu zitieren.

---

## 15. ESS-Schwellen `ESS_ON_MS2` = 5,0 / `ESS_OFF_MS2` = 3,0 m/s²

**Herkunft: A — angenommen, und zwar bewusst abweichend von der Norm.**

UN-R13-H Abs. 5.2.23.1 legt für Pkw fest, wörtlich: *„The signal shall not be activated when the vehicle deceleration is below 6 m/s² … The signal shall be de-activated at the latest when the deceleration has fallen below 2,5 m/s²."* Das Projekt verwendet stattdessen 5,0 und 3,0 m/s².

**Diese Abweichung ist begründbar und sollte in der Arbeit offensiv vertreten werden:** 6 m/s² liegen beim Fahrrad an oder oberhalb der Überschlagsgrenze (Punkt 2: 4,9–6,2 m/s² je nach Schwerpunktlage). Eine aus der Kfz-Regelung übernommene Schwelle von 6 m/s² würde am Fahrrad praktisch nie erreicht — das Notbremssignal wäre funktionslos. Die Absenkung auf 5,0 m/s² verlegt die Schwelle an den Beginn des Notbremsbereichs, wie er sich aus der Fahrradliteratur ergibt.

**Zwei Punkte gehören zwingend dazu.** Erstens: Der Wert ist eine Annahme, gestützt auf die Literatur zur Fahrradverzögerung, nicht auf eine eigene Messung. Zweitens — und wichtiger: **Ein blinkendes Notbremssignal an der Schlussleuchte ist nach § 67 Abs. 4 StVZO unzulässig** (*„Blinkende Schlussleuchten sind unzulässig"*). Die Funktion ist deshalb standardmäßig deaktiviert (`ESS_ENABLED_DEFAULT = false`) und ausschließlich als Funktionsdemonstrator zu führen. Der Zielkonflikt zwischen der Sicherheitswirkung eines Notbremssignals und dem Blinkverbot gehört in Kapitel 10.2 der Arbeit.

---

## 16. Schlusslicht-Grundhelligkeit `TAILLIGHT_DUTY_PCT` = 20 %

**Herkunft: A — angenommen. Absicherung: keine.**

§ 67 StVZO nennt **keine** Candela-Werte; die photometrischen Anforderungen sind über § 22a in die Technischen Anforderungen der Bauartgenehmigung verlagert. Eine photometrische Messung wurde nicht durchgeführt. Der Wert von 20 % ist damit weder normativ belegt noch messtechnisch geprüft.

Was er leistet, ist eine hinreichende Spreizung zur Bremslichtstufe: Zwischen 20 % und 100 % Tastverhältnis liegt ein Faktor 5 im mittleren Strom, der Helligkeitsunterschied ist deutlich sichtbar. Was er **nicht** leistet, ist ein Nachweis, dass die Schlussleuchte in dieser Stufe die für eine Bauartgenehmigung erforderliche Lichtstärke erreicht.

**Ehrliche Aussage:** „Angenommen. Die Grundhelligkeit ist so gewählt, dass zwischen Schluss- und Bremslicht ein deutlicher Helligkeitsunterschied entsteht. Ein photometrischer Nachweis der Lichtstärke nach § 67 StVZO in Verbindung mit § 22a wurde nicht geführt; das Gerät ist ein Funktionsprototyp und nicht bauartgenehmigungsfähig."

---

## Was aus dieser Betrachtung als Aufgabe folgt

Drei Werte stehen ohne jede Absicherung da und sollten in der Arbeit ausdrücklich als reine Annahmen ausgewiesen werden: die Blinker-Selbstabschaltung (60 s), die Langdruckdauer (5 s) und die Schlusslicht-Grundhelligkeit (20 %). Bei den ersten beiden ist das unproblematisch, weil kein Sicherheitsbezug besteht. Bei der Grundhelligkeit ist es der Berührungspunkt zur Zulassungsfähigkeit und deshalb in Kapitel 10.1 zu diskutieren.

Ein Befund ist neu und sollte nachgezogen werden: Die Ausschalthysterese von 1,5 m/s² besitzt im Reisegeschwindigkeitsbereich keinen Störabstand zur gemessenen Grundlinie. Das ist keine Fehlfunktion, sondern eine Auslegungsgrenze — aber sie steht bisher nirgends.

Zwei Fundstellen sind vor der Verwendung zu verifizieren: der Bezug auf ECE R6 im Quelltext bei der Blinkfrequenz und die R48-Absatznummer beim Notbremssignal.

Der wichtigste normative Fund betrifft nicht die Parameter, sondern die Arbeit insgesamt: **§ 67 Abs. 4 StVZO erlaubt die Bremslichtfunktion an der Schlussleuchte ausdrücklich.** Das Projekt hat damit eine positive Rechtsgrundlage und muss sich nicht als Grauzone rechtfertigen. Dieselbe Vorschrift verbietet blinkende Schlussleuchten, und für Fahrtrichtungsanzeiger an einspurigen Fahrrädern existiert überhaupt keine Regelung. Beides gehört als begründete Lückenanalyse in Kapitel 2.2 und 10.2 — das ist wissenschaftlich wertvoller als eine unreflektierte Umsetzung.

---

## Quellenübersicht

Welcher Wert stützt sich worauf. Die Spalte **Status** unterscheidet: *primär* = am amtlichen bzw. verlagsseitigen Volltext belegt · *sekundär* = über eine referierende Quelle belegt · *eigen* = eigene Messung oder Rechnung · *unverifiziert* = Fundstelle vor der Verwendung zu prüfen.

### Normen und Regelwerke

| Nr. | Quelle | Fundstelle | Verwendet für | Status |
|---|---|---|---|---|
| N1 | **§ 67 StVZO**, Fassung 10.06.2024 | Abs. 1, 3, 4 | Bremslichtfunktion ausdrücklich zulässig; blinkende Schlussleuchten unzulässig; keine cd-Werte → Punkte 15, 16 und normative Einordnung | primär, aber über Gesetzesportal — **vor Zitat an `gesetze-im-internet.de/stvzo_2012/__67.html` gegenprüfen** |
| N2 | **§ 67a StVZO** | gesamt | Negativbefund: betrifft Fahrrad**anhänger**, regelt keine Fahrtrichtungsanzeiger am Fahrrad → Punkt 13 | primär, gleiche Einschränkung |
| N3 | **§ 22a StVZO** | Verweis aus § 67 Abs. 1 | Photometrie liegt in der Bauartgenehmigung, nicht in § 67 → Punkt 16 | primär |
| N4 | **UN-R13-H**, ABl. EU 2023/401 | Abs. 5.2.22.2 | Bremslichtschwelle 1,3 m/s² (Kfz) → Punkt 1; Forderung nach einer Anti-Flacker-Maßnahme → Punkt 3 | primär (EUR-Lex) |
| N5 | **UN-R13-H**, ABl. EU 2023/401 | Abs. 5.2.23.1 | ESS-Schwellen 6,0 / 2,5 m/s² → Punkt 15 | primär (EUR-Lex) |
| N6 | **UN-R13-H**, CELEX 42015X1222(01) | Anhang 3, Abs. 3.1.1 | Ansprechzeit Betriebsbremsanlage 0,6 s → Punkt 8 | primär (EUR-Lex) |
| N7 | **UN-R13**, CELEX 42010X0930(01) | Abs. 5.2.1.30.2/.3 | 1,0 m/s² (Dauerbremsanlage), 0,7 m/s² als Unterdrückungserlaubnis → Punkt 1 (Korrektur einer verbreiteten Fehlannahme) | primär (EUR-Lex) |
| N8 | **UN-R53**, ABl. EU 2020/31 | Abs. 6.3.8.1 | Blinkfrequenz 90 ± 30/min = 1,5 Hz → Punkt 13 | primär (EUR-Lex) |
| N9 | **UN-R53**, ABl. EU 2020/31 | Abs. 6.14.7.1 | ESS-Blinkfrequenz 4,0 ± 1,0 Hz → Punkt 14 | primär (EUR-Lex) |
| N10 | ~~UN-R48 § 6.23 / § 6.5.9~~ | — | inhaltsgleiche Anforderungen für Pkw | **unverifiziert** — Volltext nicht bis zum Abschnitt auslesbar; in der Arbeit R53 zitieren |
| N11 | ~~ECE R6~~ | — | im Quelltext als Bezug der Blinkfrequenz genannt | **unverifiziert** — Bezug im Code prüfen oder auf R53 umstellen |

### Fachliteratur

| Nr. | Quelle | Verwendet für | Status |
|---|---|---|---|
| L1 | **Whitt, F. R.; Wilson, D. G.:** *Bicycling Science*, 2. Aufl., MIT Press 1982 | b = 60 cm, h = 120 cm → a_max = 0,50 g → Punkt 2 | sekundär (über Referenzwerk zitiert) |
| L2 | **Wilson, D. G.:** *Bicycling Science*, 3. Aufl., MIT Press 2004 | a_max = 0,56 g → Punkt 2 | sekundär |
| L3 | **Broker, J.; Hottman, M. M.:** *Bicycle Accidents, Crashes, and Collisions*, 2. Aufl. 2017 | b = 63,5 cm, h = 102 cm → a_max = 0,63 g → Punkt 2 | sekundär |
| L4 | **Stein-Cadenbach, R.:** *ABS für das Fahrrad?*, Fahrradzukunft 26 (2018) | Grenzverzögerung 5,5–6,5 m/s² → Punkt 2 | primär (frei zugänglich), Fachpublikum ohne Peer Review |
| L5 | **Lyubenov, D. et al.:** *Experimental Determination of Bicycles and Electric Bicycle Stopping Distance*, Engineering Proceedings 70 (2024), Nr. 26, **DOI 10.3390/engproc2024070026** | gemessene Vollbremsung Ø 5,08 m/s² (40 Versuche) → Punkt 2 | primär, peer-reviewed — **die beste Einzelquelle für Punkt 2** |
| L6 | **Bulla, M.; Schnädelbach, C.:** *Messungen von Geschwindigkeiten, Verzögerungen und Beschleunigungen mit neueren Fahrradtypen und Inline-Skates*, Verkehrsunfall und Fahrzeugtechnik 46 (2008), H. 6, S. 193–200 | bis 6,8 m/s² mit hydraulischer Scheibenbremse → Punkt 2 | sekundär (über Fachdatenbank), Fachzeitschrift |
| L7 | **Famiglietti, N. et al.:** *Bicycle Braking Performance Testing and Analysis*, SAE Technical Paper 2020-01-0876 | 0,40–0,71 g über acht Fahrradtypen → Punkt 2 | sekundär (Abstract) |
| L8 | **Joganich, T.:** *A Video-Based System for Measuring the Braking Performance of a Bicycle*, SAE 2018-01-5032 | 0,41 ± 0,07 g → Punkt 2 | sekundär (Abstract) |
| L9 | **Green, M.:** *„How Long Does It Take to Stop?" Methodological Analysis of Driver Perception-Brake Times*, Transportation Human Factors 2 (2000), Nr. 3, S. 195–216, **DOI 10.1207/STHF0203_1** | Reaktionszeit auf ein Bremslicht 1,25 s → Punkt 8 | primär (Abstract wörtlich), peer-reviewed — **direkt anwendbar, keine Analogie** |
| L10 | **Transportation Research Record 1111** (1987), Beitrag 9, unter Bezug auf die IALA-Empfehlung | visuelle Zeitkonstante a = 0,2 s (Blondel-Rey) → Punkt 4 | primär (TRB-PDF) |
| L11 | **Madgwick, S. O. H.:** *An efficient orientation filter for inertial and inertial/magnetic sensor arrays*, Internal Report, x-io Technologies / University of Bristol, 30.04.2010 | Filterprinzip, Problem der Beschleunigungskorrektur (Abschnitt 6) | primär — **die kursierende Zitation „25:113–118" ist falsch, es gibt keine Berichtsnummer** |
| L12 | **Mahony, R.; Hamel, T.; Pflimlin, J.-M.:** *Nonlinear Complementary Filters on the Special Orthogonal Group*, IEEE TAC 53 (2008), Nr. 5, S. 1203–1218, **DOI 10.1109/TAC.2008.923738** | Filterprinzip | Zitierangaben primär bestätigt; **Volltext nicht eingesehen** — keine Aussage über das Gating zitieren |

### Eigene Messdaten und Projektdokumente

| Nr. | Quelle | Verwendet für |
|---|---|---|
| E1 | **Messfahrt 08.08.2026**, CSV Schema v3, 1773 Frames, 177,86 s | Punkt 1 (6 von 9 Ereignissen ≥ 2,0 m/s²), Punkt 2 (2 von 9 erreichen Sättigung), Punkt 3, Punkt 4 (2 von 12 Anzeigevorgängen < 0,3 s, Median 1,6 s), Punkt 9, Punkt 11 (3 von 1776 Rahmen verloren) |
| E2 | **Project Bible Kap. 9.5.6**, Grundlinientabelle nach Geschwindigkeitsklassen | Punkt 1 (Reserve 0,32 m/s²) und Punkt 3 (99-%-Quantil 1,68 m/s² über der Ausschaltschwelle → Befund B-8) |
| E3 | **`docs/Validierung/bench_run_notes.md`**, Schritte A6.3 und A6.4 | Punkt 8 (Reaktionszeit ≤ 10 ms bzw. 20 ± 10 ms), Punkt 9 (0,651 ms Prüfstand) |
| E4 | **`docs/Validierung/stufe1_normgate.md`**, Host-Simulation T11 | Punkt 1 (effektive Ansprechschwelle 2,13 m/s², Restdämpfung 5,9 %) |
| E5 | **Feldtest 06.08.2026**, Kap. 5.3 | Punkt 10 (Stoßdauer 10–30 ms als Untergrenze der Abtastrate) |
| E6 | **`firmware/include/config.h`**, Commit `835c7b3` | alle sechzehn Werte (verbindliche Parameterquelle) |
| E7 | **Build-Ausgabe `esp32dev`**, Commit `835c7b3` | Punkt 11 (RAM 106 912 B, Flash 674 487 B) |
| E8 | **Project Bible Kap. 5.2**, Energiebilanz | Punkt 12 (0,24 A Akkustrom, V_f = 2,2 V und η = 0,90 als Annahmen) |

### Eigene Rechnungen (in der Arbeit selbst zu führen)

| Nr. | Rechnung | Für |
|---|---|---|
| R1 | Momentenbilanz um den Vorderradaufstandspunkt → a_max = g·b/h | Punkt 2 — **die Formel ist in der zugänglichen Literatur nicht in dieser Schreibweise belegt und deshalb selbst herzuleiten**; L1–L4 bestätigen sie numerisch |
| R2 | Blondel-Rey: I_eff = I·t/(a+t) mit a = 0,2 s, ausgewertet für t = 50…1000 ms | Punkt 4 (60 % bei 300 ms) |
| R3 | 600 Rahmen × 113 Byte = 67 800 Byte, Anteil am RAM-Budget | Punkt 11 |
| R4 | 2000 mAh / 0,24 A ≈ 8,3 h | Punkt 12 |
| R5 | Nyquist: DLPF 44 Hz < 50 Hz bei 100 Hz Abtastung | Punkt 10 |
| R6 | Wegbetrachtungen: 300 ms bei 50 km/h = 4,2 m; 50 ms bei 30 km/h = 0,42 m gegenüber 10,4 m Reaktionsweg | Punkte 4 und 8 |

### Nicht gefunden

Eine Quelle, die eine **Mindestleuchtdauer für zuverlässige Wahrnehmung** als solche angibt, existiert nach dieser Recherche nicht. Punkt 4 argumentiert deshalb über den Verlust an effektiver Lichtstärke, nicht über eine Wahrnehmungsschwelle. Ebenso ist ein **explizites Verwerfungskriterium der Accelerometer-Korrektur bei ‖a‖ ≠ g** weder bei Madgwick noch nachweisbar bei Mahony formuliert; Madgwick nennt die dynamische Verstärkungsanpassung in Abschnitt 6 nur als Ausblick. Das Gating ist deshalb physikalisch selbst zu begründen.
