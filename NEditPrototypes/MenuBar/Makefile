#
# Makefile générique pour les .cpp
#

include ../../Makefile.common

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

MenuBar:$(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDLIBS)

clean:
	rm -f MenuBar *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
