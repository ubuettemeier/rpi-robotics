
CC = gcc

target = bmc
# target = amd64

CFLAGS = -Wall -c -O0 -DNDEBUG

ifeq	($(target),bmc)
	LDFLAGS = -lwiringPi -lpthread -lm -lrt
else
	LDFLAGS = -lpthread -lm -lrt
endif

FILENAME = test_driver_A4988

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
driver_A4988.c \
../../../tools/rpi_tools/rpi_tools.c \
../../../tools/keypressed/keypressed.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
driver_A4988.h \
../../../tools/rpi_tools/rpi_tools.h \
../../../tools/keypressed/keypressed.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/driver_A4988.o \
../build/rpi_tools.o \
../build/keypressed.o 

# ---------------------------------------------------------------------- 
# binary code
# ----------------------------------------------------------------------
BIN = ../build/$(FILENAME)


$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

$(OBJ) : $(SRC) $(HEADER)
	$(CC) -c $(CFLAGS) $(SRC)
	mv *.o ../build

.PHONEY:	clean
clean:
	rm -rf $(OBJ) $(BIN)
