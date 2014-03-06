all: bin bin/alpha-bleeding.exe bin/alpha-remove.exe

bin:
	mkdir bin

bin/alpha-bleeding.exe: src/alpha-bleeding.cpp src/png.cpp src/png.h
	g++ src/alpha-bleeding.cpp src/png.cpp -lpng -lz -O3 -o $@

bin/alpha-remove.exe: src/alpha-remove.cpp src/png.cpp src/png.h
	g++ src/alpha-remove.cpp src/png.cpp -lpng -lz -O3 -o $@

clean:
	rm -r bin

.PHONY: clean
