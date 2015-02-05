#
# Makefile générique pour les .cpp
#

include ../../Makefile.common

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

LIBS=../../libNUtil.a

Font:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LIBS) $(LDLIBS) 

clean:
	rm -f Font *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
