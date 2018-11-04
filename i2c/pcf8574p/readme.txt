The software for I2C handling has been developed on the Raspberry Pi 3 B.
The programs work with the I2C kernel modules i2c_dev and i2c_bcm.
The I2C bus 1 is used. 
The corresponding pins are SDA.1=Pin 3, SCL.1=Pin 5.

The PCF8574 is an 8-port I/O expander I2C chip.
Each of the eight I/Os can be independently used as an input or output. 
To setup an I/O as an input, you have to write a 1 to the corresponding output.

script's
- source/edit.sh => opens the source files with geany
- source/run.sh => starts the program build/test_i2c_pcf8591

// ---------------------------------------------------------------------
Die Software für I2C Handling ist auf den Raspberry Pi 3 B entwickelt.
Die Programme arbeiten mit den I2C-Kernelmodulen i2c_dev und i2c_bcm.
Es wird der I2C-Bus 1 genutzt. 
Die ensprechenden Pin's sind SDA.1=Pin 3, SCL.1=Pin 5.

Der PCF8574 ist ein 8-Port I/O Expander I2C Chip.
Jede der acht I/Os kann unabhängig voneinander als Eingang oder Ausgang verwendet werden. 
Um eine I/O als Eingang einzurichten, müssen Sie eine 1 in den entsprechenden Ausgang schreiben.

script's
- source/edit.sh => öffnet die Source Files mit geany
- source/run.sh => startet das Programm build/test_i2c_pcf8591
