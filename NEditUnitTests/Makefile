#
# Makefile générique pour les .cpp
#

#CXX=clang
CXX=g++

FLTK_HOME=/opt/fltk-1.3.2

CXXFLAGS=--std=c++11 -Dlinux -O2 $(shell $(FLTK_HOME)/bin/fltk-config --cxxflags) -I../gtest-1.6.0/include
LDLIBS=$(shell $(FLTK_HOME)/bin/fltk-config --ldflags --use-images)

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

NEditUnitTests:$(OBJECTS)
	$(CXX) $(GCOV_FLAGS) -o $@ $(OBJECTS) ../source/libsource.a ../util/libutil.a ../gtest-1.6.0/gtest.a $(LDLIBS)

clean:
	rm -f NEditUnitTests *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c -o $@ $<
