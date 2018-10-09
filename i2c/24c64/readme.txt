The software for I2C handling is developed on the Raspberry Pi 3 B.
The programs work with the I2C kernel modules i2c_dev.
The I2C bus 1 is used. 
The corresponding pins are SDA.1=Pin 3, SCL.1=Pin 5.

The program was tested with a 24C64 EEPROM.
In write mode the EEPROM is organized in 32 byte blocks.
–24C32: 4Kx8 (128 pages of 32 bytes)
–24C64: 8Kx8 (256 pages of 32 bytes)

script's
- source/edit.sh => opens the source files with geany
- source/run.sh => starts the program build/test_i2c_24C64

// ---------------------------------------------------------------------
Die Software für I2C Handling ist auf den Raspberry Pi 3 B entwickelt.
Die Programme arbeiten mit den I2C-Kernelmodulen i2c_dev.
Es wird der I2C-Bus 1 genutzt. 
Die ensprechenden Pin's sind SDA.1=Pin 3, SCL.1=Pin 5.

Das Programm wurde mit einem 24C64 EEPROM getestet.
Im Schreibmodus ist das EEPROM in 32 Byte Blöcken organisiert.
–24C32: 4Kx8 (128 Blöcke mit 32 Bytes)
–24C64: 8Kx8 (256 Blöcke mit 32 bytes)

script's
- source/edit.sh => öffnet die Source Files mit geany
- source/run.sh => startet das Programm build/test_i2c_24C64
