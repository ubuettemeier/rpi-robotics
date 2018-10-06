
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
LDFLAGS =

FILENAME = test_i2c_pcf8591

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
../../../sensor/ultrasonic-HC-SR04/source/keypressed.c

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/keypressed.o

# ---------------------------------------------------------------------- 
# binary code
# ----------------------------------------------------------------------
BIN = ../build/$(FILENAME)


$(BIN): $(OBJ)
	$(CC) -o $(LDFLAGS) $(BIN) $(OBJ)

$(OBJ) : $(SRC)
	$(CC) -c $(CFLAGS) $(SRC)
	mv *.o ../build

.PHONEY:	clean
clean:
	rm -rf $(OBJ) $(BIN)
