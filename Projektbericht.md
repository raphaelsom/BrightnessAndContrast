# Projektbericht

## Aufgabenstellung

Unser Projekt konzentriert sich auf das Feld der Bildverarbeitung. Das resultierende Programm soll ein Pixelbild im PPM-Format anhand verschiedener Eingabeparameter zu Graustufen konvertieren, Helligkeit und Kontrast anpassen und das verarbeitete Bild schließlich wieder geeignet speichern.


## Überblick der verschiedenen Implementierungen

### Allgemein

Es wurde versucht, so wenig Instruktionen wie möglich innerhalb der Schleifen zu verwenden, konstante Werte oder Bitmasken wurden stets außerhalb gesetzt. Speziell die Division bei der Graustufenkonvertierung wurde vorgezogen.

In den Assembly-Implementierungen wird nur mit Registern gearbeitet, um Speicherzugriffe zu minimieren.


### C-SISD

In der ersten Schleife werden drei Bytes aus dem Eingabearray ausgelesen, die den RGB-Werten eines Pixels entsprechen. Für die Konvertierung in Graustufen werden die Werte aufaddiert, wobei vom Benutzer festgelegt werden kann, wie stark die einzelnen Farbwerte in die Konvertierung einfließen sollen. In der ersten Schleife wird außerdem der durchschnittliche Farbwert der Graustufenpixel berechnet, der in der zweiten Schleife zur Berechnung der Standardabweichung benötigt wird. Danach wird in der dritten Schleife der Kontrast der Pixel mithilfe der Standardabweichung angepasst.


### Assembly-SISD

Basierend auf dem gleichen Ansatz wurde hier zusätzlich versucht, die Instruktionsreihenfolge in den Schleifen hinsichtlich der verwendeten Register bestmöglich für Software-Pipelining anzuordnen.


### C-SIMD

In der ersten Schleife werden zwölf anstatt drei Bytes, also vier statt eines Pixels, in eine `xmm`-Variable gelesen. Daraus werden Farbwerte in drei `xmm`-Variablen geshuffelt, womit vier rot/grün/blau-Werte in den Variablen gleichzeitig verarbeitet werden können. In den anderen beiden Schleifen werden jeweils vier Graustufenpixel gelesen und parallel verarbeitet.


### Assembly-SIMD

Die erste Schleife basiert auf dem gleichen Ansatz wie die in der C-SIMD-Implementierung. Zusätzlich wurde eine AVX-Optimierung eingebaut, die zum Shuffeln der Register nur drei statt fünf Instruktionen benötigt. Als weitere Optimierung werden in der zweiten und dritten Schleife 16 Bytes statt vier gelesen und in vier verschiedenen `xmm`-Registern verarbeitet. Hierbei wurde erneut die AVX-Optimierung zum Shuffeln der 16 Bytes in die vier Register, auf denen dann gearbeitet wird, eingebaut. Da das Result-Array im Rahmenprogramm von malloc alloziert wird, ist die Anfangsadresse 16-aligned, somit kann immer aligned gelesen werden. Am Ende der Schleifen werden die 16 Bytes wieder in ein einziges `xmm`-Register geshuffelt, um das Schreiben der 16 Bytes in das Ausgabearray wieder mit nur einer Instruktion durchführen zu können.


### C-SISD Multithreaded

Die drei Schleifen der C-SISD Implementierung wurden mit der OpenMP-Library als parallel markiert und werden somit auf mehrere Threads aufgeteilt. Die Anzahl der Threads wird von OpenMP entschieden und festgelegt, per Default entspricht die Threadanzahl der CPU-Kernanzahl. Da die ersten beiden Schleifen als `parallel for reduction` markiert sind, musste die race-condition beim Aufsummieren der Variablen über mehrere Threads hinweg nicht explizit behandelt werden.


### C-SISD sqrt\_heron, sqrt\_ieee

Diese beiden Implementierungen führen denselben Code wie C-SISD aus, nur die Wurzel zur Standardabweichungsberechnung wird einmal mit dem Heron-Verfahren, welches selbst implementiert wurde, und einmal mit einem Approximationsalgorithmus, der auf der IEEE754-Darstellung der Zahlen basiert (Quelle: siehe Code), durchgeführt.


## Performanzmessung

### Methodik

Bei der Zeitmessung wird eine Implementierung per Default 5000-mal ausgeführt, woraus danach die durchschnittliche Dauer einer Ausführung berechnet wird.


### Messumgebung

Alle Zeitmessungen wurden in folgender Umgebung durchgeführt:

- Fedora Linux 40 (x86\_64)
- Linux-Kernel 6.12.8
- Intel i7-8565U (8) @ 4.6GHz
- 16GB RAM
- GCC 14.2.1

Kompiliert wurde mit AVX-Erweiterungen, Optimierungsstufe drei, aber mit `-fno-unroll-loops`, da in den ASM-Implementierungen kein Loop-Unrolling betrieben wurde. Damit sollte in diesem Aspekt Fairness gegenüber dem Compiler geschaffen werden.

## Ergebnisse

![](https://lh7-rt.googleusercontent.com/docsz/AD_4nXdVn462IDPGHJ5ZK23zy8VpcSJT4L5KLR9jvhpI18tT_NskUEElFWx0CgKZoJHnT2MGKQgrfLpujDKYCLdwEV4mBkgVcNEa-RdTZm6aLkV21Dlp0RSNPRQohEJ2dDgQEKudICsz?key=dFBzcnl_LVT2ELuXMID2ipUs)
*Abbildung 1: Benchmarkergebnisse aller Implementierungen*

Als Vergleichsbasis wird die **C-SISD** Implementierung verwendet, da diese als naive, aber keinesfalls schlechte Implementierung anzusehen ist. Mit dieser waren die drei verschiedenen Arten der Wurzelberechnung performanzbezogen vernachlässigbar, die Implementierung der math-Library konnte nicht übertroffen werden.

**ASM-SISD** liegt mit einem Faktor von \~0.344 der Compiler-Optimierten C-SISD Implementierung beachtlich zurück.

**C-SIMD** steigert sich um den Faktor ~3.938. Der aufgrund der parallelen Verarbeitung von immer vier Pixeln erwartete Faktor vier kann durch kleinen SIMD-Overhead wie Shuffeln und von SIMD unbetroffenen Codestücken nicht erreicht werden.

**ASM-SIMD** verzeichnet einen Performancegewinn von \~4.826. Mit der Verarbeitung von 16 statt vier Bytes innerhalb der letzten beiden Schleifen wird ein Faktor von \~1.225 verglichen mit C-SIMD erreicht.

**C-SISD Multithreaded** erreicht einen Faktor von \~2.974. In einer idealen Welt wäre der Faktor acht (Anzahl der Threads gleich CPU-Kernanzahl), aufgrund von Threaderstellungs- und -management-Overhead, sowie den seriell ausgeführten Codestücken ist dieser jedoch nicht erzielbar.


### AVX-Optimierungen

![](https://lh7-rt.googleusercontent.com/docsz/AD_4nXdC4O5avlGN541hZ8dJjy13SodL093ndPh3OF0CwfGT9kYfqon9pQcVphX3v3-tA2pKqs7qrEwEnogyVIprS7nuJcn_9cxnWJ5AjT4YIfz7l4cHtlmEw99r1TN2Qvv2kjTIpmE4?key=dFBzcnl_LVT2ELuXMID2ipUs)
*Abbildung 2: Benchmarkergebnisse AVX*

Die AVX Optimierungen haben zu Faktoren von 1.028 (ASM SIMD), 1.017 (C SIMSD) geführt. Dies lässt sich auf die geringere Anzahl an Instruktionen mit AVX zurückführen. Beim Heron-Verfahren führte AVX zu einem Faktor von 0.848, da `vdivss` langsamer als `mov` und `divss` ist.


## Aussicht

Weitere Performanzgewinne wären mit Loop-Unrolling inklusive Optimierung für Software-Pipelining möglich. Mit Verwendung der 256-Bit `ymm`- oder 512-Bit `zmm`-Registern könnte in den SIMD-Implementierungen Parallelität und Performance weiters erhöht werden.


## Projektanteile

**Felix Breithaupt:**
- PPM-Format lesen, validieren, schreiben
- Assembly-SISD, C-SIMD
- Mathematische Analysen, Optimierungen
- Projektbericht

**Raphael Sommersguter:**
- Kommandozeilenparameterparsing
- C-SISD, Assembly-SIMD, C-SISD Multithreaded
- Wurzelapproximationsimplementierungen
- Tests, Benchmarking
