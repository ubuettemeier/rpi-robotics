The software for I2C handling is developed for the Raspberry Pi 3 B.
The programs work with the I2C kernel modules i2c_dev and i2c_bcm.
The I2C bus 1 is used. 
The corresponding pins are SDA.1=Pin 3, SCL.1=Pin 5.

For the I2c communication it is helpful to install the program i2c-tools.
$:sudo apt-get install i2c-tools

Afterwards the following commands are available

12cdetect
i2cdump
i2cget
i2cset

script's
- source/edit.sh => opens the source files with geany
- source/run.sh => starts the program build/test_12c

// ---------------------------------------------------------------------
Die Software für I2C Handling ist auf den Raspberry Pi 3 B entwickelt.
Die Programme arbeiten mit den I2C-Kernelmodulen i2c_dev und i2c_bcm.
Es wird der I2C-Bus 1 genutzt. 
Die ensprechenden Pin's sind SDA.1=Pin 3, SCL.1=Pin 5.

Für die I2c Kommunikation ist es hilfreich das Programm i2c-tools zu installieren.
$:sudo apt-get install i2c-tools

Anschließend stehen folgende Kommandos zur Verfügung

12cdetect
i2cdump
i2cget
i2cset

script's
- source/edit.sh => öffnet die Source Files mit geany
- source/run.sh => startet das Programm build/test_12c
