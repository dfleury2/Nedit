#
# Makefile générique pour les .cpp
#

#CXX=clang
CXX=g++

CXXFLAGS=-O -c -I/Soft/mag/dev/tools/fltk-1.3.0/include
LDFLAGS=--std=c++0x -O2 ../util/libutil.a -L/Soft/mag/dev/tools/fltk-1.3.0/lib -L/usr/X11R6/lib -lfltk -lXext -lXft -lfontconfig -lXinerama -lpthread -ldl -lm -lX11


SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

nedit:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f nedit *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

