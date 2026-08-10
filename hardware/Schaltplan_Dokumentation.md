# Begleitdokumentation zum Schaltplan

**Elektrischer Schaltplan – Intelligentes IoT-Fahrrad-Rücklichtsystem**
**Bachelorarbeit · Hochschule Düsseldorf · Luca Krahl**
**Zeichnungsstand:** Rev. 1.1 vom 09.08.2026 · Blatt 1 von 1 · A4 Querformat
**Änderung gegenüber Rev. 1.0:** Position von SW1 korrigiert — der Schalter sitzt im Akkupfad zwischen U1 OUT+ und U2 VIN+.
**Quellen:** Project Bible Kap. 4/5 · `firmware/include/pins.h` und `config.h` (Commit `1178017`) · Datenblätter im Projekt · Verdrahtungsklärung vom 09.08.2026

---

## A. Komponentenübersicht

| Ref. | Bauteil | Funktion | Versorgung | Schnittstelle |
|---|---|---|---|---|
| BT1 | LiPo LP103454, 3,7 V, 2000 mAh | Energiespeicher | — | B+/B− an U1 |
| J1 | USB-C-Buchse (Bestandteil von U1) | Ladeanschluss, von außen zugänglich | 5 V extern | — |
| U1 | TP4056 Typ-C mit DW01 | Laderegler 1 A, Tiefentlade- und Kurzschlussschutz | USB-C 5 V | B+/B−, OUT+/OUT− |
| U2 | MT3608 | Aufwärtswandler 3,7 V → 5,00 V (Trimmer) | von U1 OUT+ | VIN/VOUT |
| SW1 | Rastender Drucktaster IP65, 8 mm | Ein/Aus **im Akkupfad zwischen U1 OUT+ und U2 VIN+** | — | — |
| C1 | Elektrolytkondensator 1000 µF | Pufferung Reglereingang | +5 V ↔ GND | — |
| C2 | Elektrolytkondensator 1000 µF | Pufferung Reglerausgang | +3V3 ↔ GND | — |
| U3 | ESP32-DevKitC-32E (WROOM-32E), AMS1117-3.3 onboard | Hauptrechner | VIN 5 V → 3V3 | GPIO, I²C, UART2, PWM, BLE |
| IC2 | GY-521 / MPU-6050 | IMU, Bremserkennung | +3V3 | I²C 0x68 |
| IC3 | BMP280 | Barometer, Höhe | +3V3 | I²C 0x76 |
| IC4 | Quectel L86-M33 | GNSS GPS+GLONASS, interne Patch-Antenne | +3V3 (VCC und V_BCKP) | UART, 9600 Bd |
| U4 | SRX882S V2.0 | 433-MHz-Empfänger, superheterodyn | +3V3 (VCC und CS) | Digitalausgang DATA |
| ANT2 | Drahtantenne 17,3 cm (λ/4) | Empfangsantenne 433 MHz | — | an U4 ANT |
| Q1–Q3 | IRLZ44N (3×) | Low-Side-PWM-Treiber | — | Gate an GPIO |
| R1/R3/R5 | 100 Ω (3×) | Gate-Serienwiderstand | — | — |
| R2/R4/R6 | 10 kΩ (3×) | Gate-Pull-Down gegen GND | — | — |
| RN1–RN3 | je 8 × 100 Ω parallel = 12,5 Ω | Strombegrenzung je LED-Kanal | in Reihe zur LED | — |
| D1 | LED rot, 3-W-COB | Schluss- und Bremslicht | Anode an +5 V | PWM über Q2 |
| D2 | LED gelb, 3-W-COB | Blinker links | Anode an +5 V | PWM über Q1 |
| D3 | LED gelb, 3-W-COB | Blinker rechts | Anode an +5 V | PWM über Q3 |
| — | QIACHIP-Handsender, 2 Tasten | Blinkerauslösung, Codes 10967538 / 10967537 | eigene Batterie | 433 MHz ASK, drahtlos |

**Nicht verbaut, aber bisher in der Stückliste geführt:** die GNSS-Antenne „Namvo". Der L86 nutzt seine interne Patch-Antenne (18,4 × 18,4 × 4 mm); der Pin EX_ANT ist unbeschaltet.

Der Micro-USB-Anschluss des DevKitC dient ausschließlich der Programmierung und ist im montierten Zustand nicht zugänglich. Seine Masse liegt hardwareseitig auf derselben GND-Schiene; beim Flashen wird der Akkupfad getrennt.

---

## B. Pinbelegung

| ESP32-Pin | Funktion | Angeschlossene Komponente | Netzname |
|---|---|---|---|
| GPIO21 | I²C SDA | IC2, IC3 | `I2C_SDA` |
| GPIO22 | I²C SCL | IC2, IC3 | `I2C_SCL` |
| GPIO16 | UART2 RX | IC4 TXD1 (Pin 2) | `GNSS_TXD1` |
| GPIO17 | UART2 TX | IC4 RXD1 (Pin 1) | `GNSS_RXD1` |
| GPIO4 | Digitaleingang RF | U4 DATA | `RF_DATA` |
| GPIO25 | PWM Blinker links | R1 → Q1 Gate | `PWM_BLINK_L` |
| GPIO26 | PWM Schluss-/Bremslicht | R3 → Q2 Gate | `PWM_BRAKE` |
| GPIO27 | PWM Blinker rechts | R5 → Q3 Gate | `PWM_BLINK_R` |
| VIN | Versorgung 5 V | +5-V-Schiene (U2 VOUT+) | `+5V` |
| 3V3 | Versorgungsausgang AMS1117 | IC2, IC3, IC4, U4, C2 | `+3V3` |
| GND | Bezugspotential | gemeinsame Masse | `GND` |

Der Interrupt-Pin des MPU-6050 bleibt unbeschaltet, weil GPIO4 durch den RF-Empfänger belegt ist; die IMU wird gepollt.

---

## C. Versorgungskonzept

Der Strompfad verläuft in einer Kette: Die USB-C-Buchse speist den TP4056, dieser lädt den LiPo-Akku über B+/B− mit maximal 1 A. Die Last hängt bewusst an **OUT+** und nicht an B+, damit der im Modul enthaltene DW01 seinen Tiefentlade- und Kurzschlussschutz auch für die Systemlast ausübt. Von OUT+ führt der Akkupfad über den rastenden Schalter **SW1** auf den Eingang des Aufwärtswandlers MT3608, dessen Trimmer auf 5,00 V eingestellt ist. Dessen Ausgang bildet die **+5-V-Schiene**, an der zwei Verbraucher hängen: der ESP32 über seinen VIN-Pin und die drei LED-Zweige über ihre Vorwiderstandsnetze. Der auf dem DevKitC verbaute AMS1117-3.3 erzeugt daraus die **+3,3-V-Schiene**, an der ausschließlich die vier Peripheriemodule liegen. Gepuffert wird an beiden Seiten des Reglers mit je 1000 µF (C1 am Eingang, C2 am Ausgang).

SW1 trennt den Eingang des Wandlers. Im ausgeschalteten Zustand ist damit die gesamte 5-V-Schiene einschließlich der LED-Zweige stromlos, und zusätzlich entfällt der Ruhestrom des MT3608. Der Ladepfad USB-C → U1 → BT1 bleibt davon unberührt; das Gerät lädt also auch ausgeschaltet.

### Strombilanz

| Betriebsfall | Entnahme an +5 V | Leistung | Strom aus BT1 (η = 0,90) |
|---|---|---|---|
| Schlusslicht (D1 bei 20 % Duty) + ESP32 + Sensorik | 159 mA | 0,80 W | 0,24 A |
| Bremslicht 100 % + ESP32 + Sensorik | 338 mA | 1,69 W | 0,51 A |
| Bremslicht + beide Blinker 100 % | 786 mA | 3,93 W | **1,18 A** |

Zugrunde gelegt: I_LED = (5 V − 2,2 V) / 12,5 Ω = 224 mA je Kanal; ESP32 mit Sensorik 113 mA an 5 V. Der Strom aus BT1 fließt vollständig über SW1, da der Schalter im Akkupfad sitzt. Rechnerische Laufzeit im Dauerbetrieb mit Schlusslicht: rund **8 h**. Die Werte sind berechnet, nicht gemessen.

---

## D. Kommunikationsschnittstellen

**I²C** verbindet den ESP32 über GPIO21 (SDA) und GPIO22 (SCL) mit IC2 (0x68) und IC3 (0x76) auf einem gemeinsamen Bus. Pull-Up-Widerstände existieren ausschließlich modulintern gegen +3,3 V; externe Widerstände sind nicht verbaut.

**UART2** verbindet den ESP32 mit dem GNSS-Modul: GPIO17 als Sendeleitung auf RXD1, GPIO16 als Empfangsleitung von TXD1, 9600 Bd, 8N1, 1 Hz Ausgaberate.

**GPIO** wird für den 433-MHz-Empfänger genutzt: U4 gibt die demodulierten Daten als Digitalsignal auf GPIO4 aus. Der Chip-Select-Pin von U4 liegt fest auf +3,3 V, der Empfänger ist damit dauerhaft aktiv.

**PWM** steuert alle drei Leuchtenkanäle über die LEDC-Peripherie mit 5 kHz und 8 bit Auflösung.

**BLE 4.2** überträgt die Telemetrie unidirektional vom Gerät zur iOS-App. Es ist eine reine Funkstrecke ohne Leitungsbezug und im Schaltplan als Annotation am ESP32 vermerkt.

**433 MHz** bildet die Strecke vom QIACHIP-Handsender zu ANT2/U4. Der Sender ist ein gekauftes Gerät mit eigener Batterie und nicht Bestandteil der Leiterplatte.

---

## E. Elektrische Plausibilitätsprüfung

Die Prüfung wurde am fertigen Schaltplan durchgeführt. Die Verbindungsprüfung erfolgte maschinell über die aus der Zeichnung exportierte Netzliste; die Bewertungen darunter sind analytisch. **An der Schaltung wurde nichts geändert.**

### E.1 Verbindungsprüfung (maschinell, bestanden)

Die exportierte Netzliste enthält 32 Netze. Alle 28 Bauteile sind vollständig verdrahtet; jedes Netz trägt mindestens zwei Knoten. Die einzigen Einzelknoten sind die sechs bewusst offenen Pins des L86 (1PPS, FORCE_ON, AADET_N, RESET, EX_ANT, NC), die im Schaltplan mit Nichtanschluss-Markierungen versehen sind. Stichproben: `GND` verbindet 19 Knoten, `+3V3` acht, `+5V` sechs, `I2C_SDA` und `I2C_SCL` je drei. Die Schalterposition ist maschinell bestätigt: SW1 liegt zwischen `Net-(U1-OUT+)` und `Net-(U2-VIN+)`.

### E.2 Befunde

**B-1 · UART-Pegel überschreitet die spezifizierte Eingangsgrenze des L86 [relevant].**
Das Datenblatt des L86 (Tab. 3) gibt für die Digitaleingänge RXD1, RESET und FORCE_ON **VIHmax = 3,1 V** an. Der ESP32 treibt GPIO17 mit 3,3 V. Der Wert liegt damit **0,2 V über der spezifizierten Obergrenze**, aber deutlich unter dem absoluten Grenzwert von 3,6 V (Tab. 11) — eine Zerstörung ist nicht zu erwarten, der Betrieb erfolgt jedoch außerhalb der zugesicherten Bedingungen. In Gegenrichtung ist die Lage unkritisch: TXD1 liefert nominal 2,8 V gegen eine Eingangsschwelle des ESP32 von 0,75 · 3,3 V = 2,48 V, also 0,32 V Reserve. *Lösungsvorschlag:* Spannungsteiler 1 kΩ / 10 kΩ oder ein 1-kΩ-Serienwiderstand in der Leitung GPIO17 → RXD1. *Freigabe erforderlich, ich ändere nichts eigenmächtig.*

**B-2 · Verlustleistung des Boardreglers unter Spitzenlast [prüfen].**
Am AMS1117-3.3 des DevKitC hängen jetzt alle vier Peripheriemodule. Im ungünstigsten Fall summieren sich ESP32-Sendespitze (~250 mA), L86-Spitzenstrom (100 mA laut Datenblatt Tab. 12) und die beiden I²C-Sensoren (4 mA) auf rund 354 mA. Bei 1,7 V Spannungsabfall sind das **0,60 W** im SOT-223-Gehäuse; bei einem Wärmewiderstand von 60–100 K/W entspricht das 36–60 K Übertemperatur. Der Strom liegt innerhalb der Belastbarkeit des Reglers, die Verlustleistung ist aber nicht vernachlässigbar — und es ist derselbe Pfad, in dem der BLE-Brownout des Altboards saß. *Empfehlung:* im Rahmen der ohnehin offenen Messung NFR-PWR-02 mitmessen.

**B-3 · Laden unter Last [prüfen].**
Da die Last an OUT+ hängt, speist der TP4056 im Ladebetrieb gleichzeitig Akku und System. Der Laststrom fließt durch die Strommessung des Ladereglers; die Abschalterkennung kann dadurch verzögert oder verhindert werden. Das ist eine bekannte Eigenschaft dieser Topologie und kein Verdrahtungsfehler, gehört aber dokumentiert.

**B-4 · Fehlende hochfrequente Entkopplung [dokumentieren].**
Das L86-Datenblatt (Kap. 3.3) empfiehlt 10 µF und 100 nF unmittelbar am VCC-Pin. Verbaut sind ausschließlich die beiden 1000-µF-Elkos an den Schienen. Elektrolytkondensatoren haben oberhalb einiger 10 kHz keine wirksame Impedanz mehr; für die Störfestigkeit des GNSS-Empfängers ist das eine Abweichung von der Herstellerempfehlung.

**B-5 · Kein Verpolungs- und kein Überstromschutz [bestätigt, dokumentieren].**
Eine Sicherung existiert nicht. Ein Kurzschluss im LED-Zweig würde nur durch die Strombegrenzung des MT3608 und den DW01 des TP4056 begrenzt. Ein Verpolungsschutz an der Akkuklemme fehlt ebenfalls. Beides ist bewusster Aufbaustand und in Kap. 12 der Project Bible bereits als Risiko geführt.

**B-6 · Schalterstrom durch die Position im Akkupfad [prüfen].**
Da SW1 vor dem Aufwärtswandler sitzt, führt er dessen Eingangsstrom. Im Worst Case sind das **1,18 A** bei 3,7–4,2 V; auf der 5-V-Seite wären es bei gleicher Leistung nur 0,79 A gewesen. Für den verbauten 8-mm-Drucktaster liegt kein Datenblatt vor, Nennstrom und Kontaktwiderstand sind damit unbelegt. Ein erhöhter Kontaktwiderstand wirkt an dieser Stelle zusätzlich direkt auf den Eingangsspannungsbereich des MT3608, weil er den ohnehin niedrigen Akkuspannungspegel weiter absenkt. *Empfehlung:* Datenblatt beschaffen oder den Spannungsabfall über dem geschlossenen Kontakt unter Last messen — sinnvollerweise zusammen mit NFR-PWR-02.

### E.3 Als unkritisch geprüft

**GPIO-Belegung.** Die acht verwendeten Pins sind paarweise verschieden. GPIO4 ist kein Strapping-Pin. Die Strapping-Pins GPIO0, 2, 5, 12 und 15 sind unbeschaltet. GPIO16 und GPIO17 sind beim WROOM-32E frei nutzbar — die Einschränkung gilt nur für WROVER-Module mit PSRAM. GPIO25 bis 27 sind als Digitalausgänge ohne Boot-Bedingungen verwendbar.

**I²C-Bus.** Adressen 0x68 und 0x76 kollidieren nicht. Zwei modulinterne Pull-Ups von typisch je 4,7 kΩ ergeben parallel etwa 2,35 kΩ; daraus folgt ein Low-Strom von 1,4 mA, deutlich unter der Senkfähigkeit des ESP32 von 3 mA nach I²C-Spezifikation. Die Anstiegszeit beträgt bei geschätzten 50 pF Busskapazität rund 0,1 µs gegen ein zulässiges Maximum von 1 µs im Standard-Modus. Beide Sensoren liegen an derselben 3,3-V-Schiene wie der Controller, ein Pegelwandler ist nicht erforderlich.

**UART-Kreuzung.** TX und RX sind korrekt gekreuzt (GPIO17 → RXD1, TXD1 → GPIO16).

**MOSFET-Ansteuerung.** Der IRLZ44N ist ein Logic-Level-Typ mit einer Gate-Schwellspannung von 1,0 bis 2,0 V. Bei 3,3 V Ansteuerung und 224 mA Laststrom liegt der Durchlasswiderstand bei etwa 0,05 Ω, die Verlustleistung damit bei 2,5 mW je Transistor — thermisch belanglos. Die Gate-Zeitkonstante beträgt mit 100 Ω und einer Eingangskapazität von rund 3,3 nF etwa 0,33 µs; bei 5 kHz Schaltfrequenz (200 µs Periode) entfallen damit unter 1 % der Periode auf die Umschaltvorgänge. Die 10-kΩ-Pull-Downs halten die Gates während Reset und Bootvorgang sicher auf Masse. Der Transistor ist mit 47 A Nennstrom für 224 mA erheblich überdimensioniert; das ist kein Fehler, sondern eine Folge der Bauteilverfügbarkeit und darf so begründet werden.

**LED-Zweige.** Jeder Kanal besitzt eine Strombegrenzung. Die Verlustleistung im Widerstandsnetz beträgt 0,63 W, verteilt auf acht Widerstände also 78 mW je Bauteil — bei 0,25-W-Typen eine Auslastung von 31 %.

**Thermische Stabilität der Vorwiderstandslösung.** Der in Kap. 12 der Project Bible geführte Verdacht auf thermisches Weglaufen lässt sich entkräften. Mit I = (5 V − V_f)/12,5 Ω und einem Temperaturkoeffizienten der Flussspannung von etwa −2 mV/K ergibt sich dI/dT = 0,16 mA/K. Über eine Erwärmung von 50 K steigt der Strom damit um 8 mA, also um 3,6 %. Entscheidend ist, dass über dem Widerstand mit 2,8 V mehr Spannung abfällt als über der LED mit 2,2 V — die Schaltung ist dadurch hinreichend steif. Ein thermisches Weglaufen ist bei dieser Dimensionierung **nicht zu erwarten**. Das Risiko kann in der Bible entsprechend herabgestuft werden.

---

## F. Offene Punkte

| Nr. | Punkt | Status |
|---|---|---|
| F-1 | Pegelanpassung GPIO17 → L86 RXD1 (Befund B-1) | Entscheidung des Verfassers erforderlich |
| F-2 | Messung der Verlustleistung am AMS1117 unter Spitzenlast (B-2) | offen, mit NFR-PWR-02 zu verbinden |
| F-3 | Verhalten des TP4056 beim Laden unter Last (B-3) | offen, messtechnisch zu prüfen |
| F-4 | Fehlende 10 µF / 100 nF am L86-VCC (B-4) | dokumentierte Abweichung |
| F-5 | Datenblatt der 3-W-COB-LED (Hersteller Vrabocry) | fehlt weiterhin; V_f = 2,2 V ist Mittelwert der Produktangabe 2,0–2,4 V |
| F-6 | Photometrischer Nachweis der Lichtstärke nach § 67 StVZO | offen, separate Messung |
| F-7 | Wirkungsgrad des MT3608 unter realer Last | angenommen η = 0,90, nicht gemessen |
| F-8 | Namvo-GNSS-Antenne in der Stückliste als nicht verbaut kennzeichnen | Dokumentationskorrektur |
| F-9 | Anordnung der Patch-Antenne im Gehäuse | Datenblatt fordert freie Sicht nach oben, nichtmetallisches Gehäuse, ≥ 10 mm Abstand zur BLE-Antenne des ESP32 |
| F-10 | Nennstrom und Kontaktwiderstand von SW1 (B-6) | Datenblatt fehlt; Messung des Spannungsabfalls unter Last empfohlen |

Alle Größen im Schaltplan stammen aus belegten Projektquellen. Erfunden wurde nichts; die einzige rechnerische Annahme ist die mittlere Flussspannung von 2,2 V, die als solche gekennzeichnet ist.

---

## G. Dateien

| Datei | Inhalt |
|---|---|
| `schaltplan_fahrrad_ruecklichtsystem.kicad_sch` | KiCad-7-Schaltplan, in KiCad direkt zu öffnen und zu bearbeiten; die Symbolbibliothek ist in der Datei eingebettet, es werden keine externen Bibliotheken benötigt |
| `schaltplan_fahrrad_ruecklichtsystem.pdf` | Vektor-PDF, A4 Querformat, schwarzweiß — für die Bachelorarbeit |
| `schaltplan_fahrrad_ruecklichtsystem_farbe.pdf` | dieselbe Zeichnung im KiCad-Farbschema |
| `schaltplan_fahrrad_ruecklichtsystem.svg` | Vektorgrafik zur Weiterverarbeitung |
| `netzliste.net` | aus der Zeichnung exportierte Netzliste (Grundlage der Verbindungsprüfung E.1) |
| `Schaltplan_Dokumentation.md` | dieses Dokument |
| `gen.py`, `symlib.py` | Erzeugungsskripte, mit denen die Zeichnung reproduzierbar ist |

Die Zeichnung enthält keine Informationen, die ausschließlich über Farbe transportiert werden; die schwarzweiße Fassung ist vollständig gleichwertig.
