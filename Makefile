SUBDIRS=source util

all:nedit++ NEditUnitTests

nedit++:libutil.a libsource.a NEdit/*.cpp
	(cd NEdit && make)

NEditUnitTests:libutil.a libsource.a gtest.a NEditUnitTests/*.cpp
	(cd NEditUnitTests && make)

libsource.a:source/*.cpp source/*.h
	(cd source && make)

libutil.a:util/*.cpp util/*.h
	(cd util && make)

gtest.a:gtest-1.6.0/src/*.cc
	(cd gtest-1.6.0 && make)

clean:
	(cd NEdit && make clean)
	(cd source && make clean)
	(cd util && make clean)
