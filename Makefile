#Makefile for yab2cpp
#by Samuel D. Crow

#presently architecture-neutral

CC=g++
LD=g++

#release build
#FLAGS=-c -Os -std=c++11
#LDFLAGS=LDFLAGS=-std=c++11

#debug build
FLAGS=-c -g -Og -std=c++11
LDFLAGS=-std=c++11

AR=ar r

default: yab2cpp

yabDataStructures.o: yabDataStructures.cpp yab2cpp.h
	$(CC) $(FLAGS) -o yabDataStructures.o yabDataStructures.cpp

yabCodeStructures.o: yabCodeStructures.cpp yab2cpp.h
	$(CC) $(FLAGS) -o yabCodeStructures.o yabCodeStructures.cpp

yabFunctions.o: yabFunctions.cpp yab2cpp.h
	$(CC) $(FLAGS) -o yabFunctions.o yabFunctions.cpp

yab2cpp.o: yab2cpp.cpp yab2cpp.h
	$(CC) $(FLAGS) -o yab2cpp.o yab2cpp.cpp

#BASIC_framework.a: yabDataStructures.o yabCodeStructures.o yabFunctions.o
#	$(AR) BASIC_framework.a yabDataStructures.o yabCodeStructures.o yabFunctions.o

#yab2cpp: BASIC_framework.a yab2cpp.o
yab2cpp: yab2cpp.o yabCodeStructures.o yabDataStructures.o yabFunctions.o
	$(LD) $(LDFLAGS) -o yab2cpp yab2cpp.o yabCodeStructures.o yabDataStructures.o yabFunctions.o
#BASIC_framework.a

clean:
	rm -f *.o yab2cpp BASIC_framework.a
