
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
LDFLAGS =

FILENAME = test_i2c_pcf8591

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
../../../tools/keypressed/keypressed.c \
../../../tools/rpi_tools/rpi_tools.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
../../../tools/keypressed/keypressed.h \
../../../tools/rpi_tools/rpi_tools.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/keypressed.o \
../build/rpi_tools.o

# ---------------------------------------------------------------------- 
# binary code
# ----------------------------------------------------------------------
BIN = ../build/$(FILENAME)


$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(LDFLAGS) $(OBJ)

$(OBJ) : $(SRC) $(HEADER)
	$(CC) -c $(CFLAGS) $(SRC)
	mv *.o ../build

.PHONEY:	clean
clean:
	rm -rf $(OBJ) $(BIN)
