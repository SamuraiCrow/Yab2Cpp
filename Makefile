CC := g++
CFLAGS := -Wall
CFLAGS += -std=c++11
CFLAGS += -Os
LFLAGS := 

ODIR := build
RDIR := output

YABCODESTRUCTURES_SOURCE_DEPS := yabCodeStructures.cpp yab2cpp.h yab2cpp.cpp tester.cpp runtime/runtime.h
YAB2CPP_SOURCE_DEPS := yab2cpp.cpp yab2cpp.h tester.cpp
YABDATASTRUCTURES_SOURCE_DEPS := yabDataStructures.cpp yab2cpp.h yab2cpp.cpp tester.cpp
YABFUNCTIONS_SOURCE_DEPS := yabFunctions.cpp yab2cpp.h yab2cpp.cpp tester.cpp
YABIO_SOURCE_DEPS := yab2cpp.h yab2cpp.cpp tester.cpp

all: binaries

$(ODIR):
	@mkdir $(ODIR)

$(RDIR):
	@mkdir $(RDIR)

binaries: bin_yab2cpp bin_tester

YAB2CPP_OBJECT_DEPS := $(ODIR)/yabCodeStructures.o $(ODIR)/yabFunctions.o $(ODIR)/yabDataStructures.o $(ODIR)/yabIO.o $(ODIR)/yab2cpp.o
TESTER_OBJECT_DEPS := $(ODIR)/yabCodeStructures.o $(ODIR)/yabFunctions.o $(ODIR)/yabDataStructures.o $(ODIR)/yabIO.o $(ODIR)/tester.o

bin_tester: $(RDIR) $(ODIR) $(TESTER_OBJECT_DEPS)
	$(CC) -o tester  $(TESTER_OBJECT_DEPS) $(LFLAGS)

bin_yab2cpp: $(RDIR) $(ODIR) $(YAB2CPP_OBJECT_DEPS)
	$(CC) -o yab2cpp $(YAB2CPP_OBJECT_DEPS) $(LFLAGS)

$(ODIR)/yabCodeStructures.o: $(ODIR) $(YABCODESTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabCodeStructures.cpp -o $(ODIR)/yabCodeStructures.o

$(ODIR)/tester.o: $(ODIR) $(YAB2CPP_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) tester.cpp -o $(ODIR)/tester.o

$(ODIR)/yab2cpp.o: $(ODIR) $(YAB2CPP_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yab2cpp.cpp -o $(ODIR)/yab2cpp.o

$(ODIR)/yabIO.o: $(ODIR) $(YABIO_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabIO.cpp -o $(ODIR)/yabIO.o

$(ODIR)/yabDataStructures.o: $(ODIR) $(YABDATASTRUCTURES_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabDataStructures.cpp -o $(ODIR)/yabDataStructures.o

$(ODIR)/yabFunctions.o: $(ODIR) $(YABFUNCTIONS_SOURCE_DEPS)
	$(CC) -c $(CFLAGS) yabFunctions.cpp -o $(ODIR)/yabFunctions.o

.PHONY: clean
clean:
	rm -rf build/* output/* yab2cpp tester
	