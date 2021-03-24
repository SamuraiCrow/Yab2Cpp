CC := g++
CFLAGS := -Wall
CFLAGS += -std=c++11
CFLAGS += -Os
LFLAGS := 

all: binaries

binaries: bin_main

SRC := ../output/

OUTPUT_DEPS := $(SRC)consts.h $(SRC)heap.h $(SRC)functions.h runtime.h $(SRC)output.cpp

MAIN_OBJ_DEPS := output.o main.o

bin_main: $(MAIN_OBJ_DEPS)
	$(CC) -o main $(MAIN_OBJ_DEPS)

main.o: $(OUTPUT_DEPS)
	$(CC) -o main.o -c main.cpp

output.o: $(OUTPUT_DEPS)
	$(CC) -I$(SRC) -o output.o -c $(SRC)output.cpp

.PHONY: clean
clean:
	rm -rf *.o main