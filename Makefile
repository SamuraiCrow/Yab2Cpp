CC := g++
CFLAGS := -Wall
CFLAGS += -std=c++11
CFLAGS += -Os
LFLAGS := 

ODIR := build

YABCODESTRUCTURES_SOURCE_DEPS := yabCodeStructures.cpp yab2cpp.h yab2cpp.cpp
YAB2CPP_SOURCE_DEPS := yab2cpp.cpp yab2cpp.h
YABDATASTRUCTURES_SOURCE_DEPS := yabDataStructures.cpp yab2cpp.h yab2cpp.cpp
YABFUNCTIONS_SOURCE_DEPS := yabFunctions.cpp yab2cpp.h yab2cpp.cpp

all: binaries

$(ODIR):
	@mkdir $(ODIR)

binaries: bin_yab2cpp 

YAB2CPP_OBJECT_DEPS := $(ODIR)/yabCodeStructures.o $(ODIR)/yabFunctions.o $(ODIR)/yabDataStructures.o $(ODIR)/yab2cpp.o

bin_yab2cpp: $(ODIR) $(YAB2CPP_OBJECT_DEPS)
	$(CC) -o yab2cpp $(YAB2CPP_OBJECT_DEPS) $(LFLAGS)

$(ODIR)/yabCodeStructures.o: $(ODIR) $(YABCODESTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabCodeStructures.cpp -o $(ODIR)/yabCodeStructures.o

$(ODIR)/yab2cpp.o: $(ODIR) $(YAB2CPP_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yab2cpp.cpp -o $(ODIR)/yab2cpp.o

$(ODIR)/yabDataStructures.o: $(ODIR) $(YABDATASTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabDataStructures.cpp -o $(ODIR)/yabDataStructures.o

$(ODIR)/yabFunctions.o: $(ODIR) $(YABFUNCTIONS_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabFunctions.cpp -o $(ODIR)/yabFunctions.o

.PHONY: clean
clean:
	rm -rf build/* yab2cpp 
