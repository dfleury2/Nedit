SUBDIRS=source util

all:nedit++ NEditUnitTests

nedit++:libutil.a libsource.a NEdit/*.cpp
	(cd NEdit && make)

NEditUnitTests:libutil.a libsource.a gtest.a NEditUnitTests/*.cpp
	(cd NEditUnitTests && make)

gcov:util/*.cpp util/*.h source/*.cpp source/*.h NEditUnitTests/*.cpp
	make GCOV_FLAGS=--coverage NEditUnitTests
	NEditUnitTests/NEditUnitTests
	lcov --no-external --capture --directory . --output-file coverage.info
	genhtml --branch-coverage --demangle-cpp coverage.info --output-directory lcov

libsource.a:source/*.cpp source/*.h
	(cd source && make)

libutil.a:util/*.cpp util/*.h
	(cd util && make)

gtest.a:gtest-1.6.0/src/*.cc
	(cd gtest-1.6.0 && make)

clean:
	(cd NEdit && make clean)
	(cd NEditUnitTests && make clean)
	(cd source && make clean)
	(cd util && make clean)

fullclean:clean
	-rm -rf lcov
	-find . -name "*.gc*" -exec rm {} \;
