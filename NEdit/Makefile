#
# Makefile g�n�rique pour les .cpp
#

include ../Makefile.common

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

nedit++:$(OBJECTS) ../source/libsource.a ../util/libutil.a
	$(CXX) -o $@ $(OBJECTS) ../source/libsource.a ../util/libutil.a $(LDLIBS)

clean:
	rm -f nedit++ *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
