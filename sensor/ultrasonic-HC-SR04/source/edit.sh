#!/bin/bash

# opens all files with geany

kph="../../../tools/keypressed/keypressed.h"
kpc="../../../tools/keypressed/keypressed.c"

rth="../../../tools/rpi_tools/rpi_tools.h"
rtc="../../../tools/rpi_tools/rpi_tools.c"

geany  test_hc_sr04.c  hc_sr04.c hc_sr04.h $kpc $kph $rtc $rth Makefile run.sh edit.sh ../readme.txt &
