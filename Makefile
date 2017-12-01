CC := gcc
EXE_NAME := track

all: clean build

build: track.c
	$(CC) -g -o $(EXE_NAME) track.c entry.c

clean:
	rm -rf *.o $(EXE_NAME)
