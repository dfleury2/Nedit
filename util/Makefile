#
# Makefile générique pour les .cpp
#

include ../Makefile.common

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

libutil.a:$(OBJECTS)
	ar -r $@ $(OBJECTS)

clean:
	rm -f libutil.a *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) -c -o $@ $<

