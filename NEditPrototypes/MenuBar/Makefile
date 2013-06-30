#
# Makefile générique pour les .cpp
#

#CXX=clang
CXX=g++

FLTK_HOME=/opt/fltk-1.3.2

CXXFLAGS=--std=c++11 -fpermissive -Wwrite-strings -Dlinux -O2 $(shell $(FLTK_HOME)/bin/fltk-config --cxxflags)
LDLIBS=$(shell $(FLTK_HOME)/bin/fltk-config --ldflags --use-images)

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

MenuBar:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDLIBS)

clean:
	rm -f MenuBar *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
