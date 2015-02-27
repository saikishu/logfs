all:	logfs.cpp logfs.h
	g++ -std=c++0x -o logfs logfs.cpp
debug:	logfs.cpp logfs.h
	g++ -std=c++0x -g -o logfs logfs.cpp
clean:
	rm -f *.o *~ logfs core
