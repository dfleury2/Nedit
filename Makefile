SUBDIRS=source util

source:util/libutil.a
	(cd source && make)

util/libutil.a:
	(cd util && make)

clean:
	(cd source && make clean)
	(cd util && make clean)
