
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
LDFLAGS = -lwiringPi -lpthread

FILENAME = test_driver_A4988

# ----------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------
SRC = \
$(FILENAME).c \
driver_A4988.c \
../../../sensor/ultrasonic-HC-SR04/source/keypressed.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
driver_A4988.h \
../../../sensor/ultrasonic-HC-SR04/source/keypressed.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/driver_A4988.o \
../build/keypressed.o 

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
