#!/bin/bash

rth="../../../tools/rpi_tools/rpi_tools.h"
rtc="../../../tools/rpi_tools/rpi_tools.c"

# opens all required files with geany
geany  test_i2c_24c64.c Makefile run.sh edit.sh $rth $rtc ../readme.txt &
