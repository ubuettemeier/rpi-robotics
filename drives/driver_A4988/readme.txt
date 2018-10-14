The software is developed for the Raspberry Pi 3 B.
A DAYCOM ST-A4988 is used as motor driver.
The module is distributed by Pollin Electronic:

https://www.pollin.de/p/schrittmotor-treiber-daycom-st-a4988-712678

The A4988 stepper motor driver is suitable for full, half, quarter, eighth and
sixteenth step. In this example, it is operated in half step mode.

The program uses the pins ENABLE, DIR, STEP.

To avoid disturbing latencies, the working thread is set to priority 95.
For real-time operation, the priority would have to be set to 99. 
This requires root privileges.

script's
- source/edit.sh => opens the source files with geany
- source/run.sh => starts the program build/test_driver_A4988

// ---------------------------------------------------------------------
Die Software ist auf den Raspberry Pi 3 B entwickelt.
Als Motortreiber wird ein DAYCOM ST-A4988 verwendet.
Vertrieben wird das Modul von Pollin Electronic:

https://www.pollin.de/p/schrittmotor-treiber-daycom-st-a4988-712678

Der A4988 Schrittmotortreiber ist für Voll-, Halb-, Viertel, Achtel und
Sechzehntel Schritt ausgelegt. In diesem Beispiel wird er im Halb Schritt Modus betrieben.

Das Programm nutzt die Pin's ENABLE, DIR und STEP.

Um keine störenden Latenzen zu bekommen, wird der Arbeitsthread auf Priorität 95 gesetzt.
Für den Realtime Betrieb müsste die Priorität auf 99 gesetzt werden. 
Hierfür sind dann root-Rechte erforderlich.

script's
- source/edit.sh => öffnet die Source Files mit geany
- source/run.sh => startet das Programm build/test_driver_A4988
