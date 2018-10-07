The software for I2C handling has been developed on the Raspberry Pi 3 B.
The programs work with the I2C kernel modules i2c_dev and i2c_bcm.
The I2C bus 1 is used. 
The corresponding pins are SDA.1=Pin 3, SCL.1=Pin 5.

The PCF8591 is an 8-bit A/D and D/A converter. It has 4 ADC's and 1 DAC.
The IC is addressed via the I2C bus. The base address is 0x48.

An AD conversion with the PCF9591 is started automatically after a read request.
This means that the read value is obsolete. This must be taken into account for time-critical applications.

script's
- source/edit.sh => opens the source files with geany
- source/run.sh => starts the program build/test_i2c_pcf8591

// ---------------------------------------------------------------------
Die Software für I2C Handling ist auf den Raspberry Pi 3 B entwickelt.
Die Programme arbeiten mit den I2C-Kernelmodulen i2c_dev und i2c_bcm.
Es wird der I2C-Bus 1 genutzt. 
Die ensprechenden Pin's sind SDA.1=Pin 3, SCL.1=Pin 5.

Der PCF8591 ist ein 8-Bit A/D- und D/A Wandler. Er verfügt über 4 ADC's und 1 DAC.
Der Baustein wird über den I2C Bus angesprochen. Die Basisadresse ist 0x48.

Eine AD-Wandlung wird beim PCF9591 automatisch nach einer Leseanforderung gestartet.
D.h. der gelesene Wert ist veraltet. Dies ist bei Zeitkritischen Anwendungen zu berücksichtigen.

script's
- source/edit.sh => öffnet die Source Files mit geany
- source/run.sh => startet das Programm build/test_i2c_pcf8591
