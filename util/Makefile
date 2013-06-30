#
# Makefile générique pour les .cpp
#

#CXX=clang
CXX=g++

FLTK_HOME=/opt/fltk-1.3.2

CXXFLAGS=-O2  --std=c++11 $(shell $(FLTK_HOME)/bin/fltk-config --cxxflags)
LDLIBS=$(shell $(FLTK_HOME)/bin/fltk-config --ldflags --use-images)

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

libutil.a:$(OBJECTS)
	ar -r $@ $(OBJECTS)

clean:
	rm -f libutil.a *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c -o $@ $<

