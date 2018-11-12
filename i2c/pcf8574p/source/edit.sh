#!/bin/bash

rth="../../../tools/rpi_tools/rpi_tools.h"
rtc="../../../tools/rpi_tools/rpi_tools.c"

# opens all files with geany
geany -s test_i2c_pcf8574.c Makefile run.sh edit.sh $rtc $rth ../readme.txt &
