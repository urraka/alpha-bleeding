ifeq ($(shell uname | grep 'MINGW32_NT' -c),1)
  flags += -static-libgcc -static-libstdc++
endif

all: bin bin/alpha-bleeding.exe bin/alpha-remove.exe bin/alpha-set.exe

bin:
	mkdir bin

bin/alpha-bleeding.exe: src/alpha-bleeding.cpp src/png.cpp src/png.h
	g++ $(CFLAGS) src/alpha-bleeding.cpp src/png.cpp $(LDFLAGS) -lpng -lz -O3 $(flags) -o $@

bin/alpha-remove.exe: src/alpha-remove.cpp src/png.cpp src/png.h
	g++ $(CFLAGS) src/alpha-remove.cpp src/png.cpp $(LDFLAGS) -lpng -lz -O3 $(flags) -o $@

bin/alpha-set.exe: src/alpha-set.cpp src/png.cpp src/png.h
	g++ $(CFLAGS) src/alpha-set.cpp src/png.cpp $(LDFLAGS) -lpng -lz -O3 $(flags) -o $@

clean:
	rm -r bin

.PHONY: clean
