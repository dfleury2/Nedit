#
# Makefile g�n�rique pour les .cpp
#

#CXX=clang
CXX=g++

FLTK_HOME=/opt/fltk-1.3.2

CXXFLAGS=--std=c++11 -fpermissive -Wwrite-strings -Dlinux -O2 $(shell $(FLTK_HOME)/bin/fltk-config --cxxflags)
LDLIBS=$(shell $(FLTK_HOME)/bin/fltk-config --ldflags)

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

Tabs:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDLIBS)

clean:
	rm -f Tabs *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
