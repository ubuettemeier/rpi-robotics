
CC = gcc

CFLAGS = -Wall -c -O0 -DNDEBUG
FLAGS =
INC=
LDFLAGS = -lwiringPi

FILENAME = test_laser_sensor
OUTPUTFOLDER = ../build

SOURCE = $(FILENAME).c 
HEADER =
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

