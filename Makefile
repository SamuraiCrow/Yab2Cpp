CC := g++
CFLAGS := -Wall -std=c++11
LFLAGS := 

YABCODESTRUCTURES_SOURCE_DEPS := yabCodeStructures.cpp yab2cpp.h yab2cpp.cpp tester.cpp runtime/runtime.h
YAB2CPP_SOURCE_DEPS := yab2cpp.cpp yab2cpp.h tester.cpp
YABDATASTRUCTURES_SOURCE_DEPS := yabDataStructures.cpp yab2cpp.h yab2cpp.cpp tester.cpp
YABFUNCTIONS_SOURCE_DEPS := yabFunctions.cpp yab2cpp.h yab2cpp.cpp tester.cpp
YABIO_SOURCE_DEPS := yab2cpp.h yab2cpp.cpp tester.cpp
BIN_OBJECT_DEPS := build/yabCodeStructures.o build/yabFunctions.o build/yabDataStructures.o build/yabIO.o
YAB2CPP_OBJECT_DEPS := $(BIN_OBJECT_DEPS) build/yab2cpp.o
TESTER_OBJECT_DEPS := $(BIN_OBJECT_DEPS) build/tester.o

.PHONY:all
all: CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -Os
all: binaries

.PHONY:debug
debug: CXXFLAGS := $(CFLAGS) -g
debug: binaries

.PHONY:binaries
binaries: yab2cpp tester

tester: $(TESTER_OBJECT_DEPS)
	$(CC) -o tester $(TESTER_OBJECT_DEPS) $(LFLAGS)

yab2cpp: $(YAB2CPP_OBJECT_DEPS)
	$(CC) -o yab2cpp $(YAB2CPP_OBJECT_DEPS) $(LFLAGS)

build/yabCodeStructures.o: $(YABCODESTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) yabCodeStructures.cpp -o build/yabCodeStructures.o

build/tester.o: $(YAB2CPP_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) tester.cpp -o build/tester.o

build/yab2cpp.o: $(YAB2CPP_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) yab2cpp.cpp -o build/yab2cpp.o

build/yabIO.o: $(YABIO_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) yabIO.cpp -o build/yabIO.o

build/yabDataStructures.o: $(YABDATASTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) yabDataStructures.cpp -o build/yabDataStructures.o

build/yabFunctions.o: $(YABFUNCTIONS_SOURCE_DEPS)
	$(CC) -c $(CXXFLAGS) yabFunctions.cpp -o build/yabFunctions.o

.PHONY: clean
clean:
	rm -rf build/*.o output/*.h output/*.cpp yab2cpp tester
