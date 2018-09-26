
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
FLAGS =
INC=
LDFLAGS = -lm -lpthread -lrt -lwiringPi

FILENAME = test_hc_sr04
OUTPUTFOLDER = ../build

SOURCE = $(FILENAME).c hc_sr04.c keypressed.c
HEADER = hc_sr04.h keypressed.h
QUELLE = $(SOURCE) $(HEADER)

OBJ	= $(SOURCE:.c=.o)
BIN = $(FILENAME)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)
	mv *.o $(OUTPUTFOLDER)
	mv $(FILENAME) $(OUTPUTFOLDER)

$(OBJ): $(QUELLE) $(HEADER)	
	$(CC) $(CFLAGS) $(FLAGS) $(INC) $(SOURCE)

.PHONEY:	clean
clean:
	rm -rf $(OBJ)
	rm -rf $(FILENAME)

