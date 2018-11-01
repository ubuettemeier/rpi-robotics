#!/bin/bash

# opens all files with geany
rth="../../../tools/rpi_tools/rpi_tools.h"
rtc="../../../tools/rpi_tools/rpi_tools.c"

geany  test_driver_A4988.c driver_A4988.c driver_A4988.h $rtc $rth Makefile run.sh edit.sh ../readme.txt &
