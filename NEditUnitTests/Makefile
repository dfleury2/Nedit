#
# Makefile générique pour les .cpp
#

include ../Makefile.common

SOURCES:= $(shell ls *.cpp)
OBJECTS:=$(SOURCES:.cpp=.o)

NEditUnitTests:$(OBJECTS)
	$(CXX) $(GCOV_FLAGS) -o $@ $(OBJECTS) ../source/libsource.a ../util/libutil.a ../gtest-1.6.0/gtest.a $(LDLIBS)

clean:
	rm -f NEditUnitTests *.o

%.o:%.cpp
	$(CXX) $(CXXFLAGS)  -I../gtest-1.6.0/include -c -o $@ $<
