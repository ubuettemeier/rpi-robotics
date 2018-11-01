
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
LDFLAGS = -lwiringPi -lpthread -lm

FILENAME = test_driver

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
../source/driver_A4988.c \
../../../tools/rpi_tools/rpi_tools.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
../source/driver_A4988.h \
../../../tools/rpi_tools/rpi_tools.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/driver_A4988.o \
../build/rpi_tools.o

# ---------------------------------------------------------------------- 
# binary code
# ----------------------------------------------------------------------
BIN = ../build/$(FILENAME)


$(BIN): $(OBJ)
	$(CC)  $(OBJ) -o $(BIN) $(LDFLAGS)

$(OBJ) : $(SRC) $(HEADER)
	$(CC) -c $(CFLAGS) $(SRC)
	mv *.o ../build

.PHONEY:	clean
clean:
	rm -rf $(OBJ) $(BIN)
