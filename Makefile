SUBDIRS=source util

source:libutil.a source/*.cpp source/*.h
	(cd source && make)

libutil.a:util/*.cpp util/*.h
	(cd util && make)

clean:
	(cd source && make clean)
	(cd util && make clean)
