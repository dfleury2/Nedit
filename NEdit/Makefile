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

nedit++:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) ../source/libsource.a ../util/libutil.a $(LDLIBS)

clean:
	rm -f nedit++ *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
