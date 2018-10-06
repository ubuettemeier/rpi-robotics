
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
keypressed.c

# ----------------------------------------------------------------------
# Header files
# ----------------------------------------------------------------------
HEADER = \
hc_sr04.h \
keypressed.h

# ---------------------------------------------------------------------- 
# Object files
# ----------------------------------------------------------------------
OBJ = \
../build/$(FILENAME).o \
../build/hc_sr04.o \
../build/keypressed.o

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
