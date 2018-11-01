
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
FLAGS =
INC=
LDFLAGS = -lpthread -lrt -lwiringPi

FILENAME = test_hc_sr04

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
hc_sr04.c \
../../../tools/keypressed/keypressed.c \
../../../tools/rpi_tools/rpi_tools.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
hc_sr04.h \
../../../tools/keypressed/keypressed.h \
../../../tools/rpi_tools/rpi_tools.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/hc_sr04.o \
../build/keypressed.o \
../build/rpi_tools.o

# ---------------------------------------------------------------------- 
# binary code
# ----------------------------------------------------------------------
BIN = ../build/$(FILENAME)


$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(LDFLAGS)  $(OBJ) 

$(OBJ) : $(SRC) $(HEADER)
	$(CC) -c $(CFLAGS) $(SRC)
	mv *.o ../build

.PHONEY:	clean
clean:
	rm -rf $(OBJ) $(BIN)
