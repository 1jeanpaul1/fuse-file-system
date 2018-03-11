CFLAGS = -g -Wall -DFUSE_USE_VERSION=30 `pkg-config fuse --cflags`
LINKFLAGS = -Wall `pkg-config fuse --libs`

all: bin/filesystem

clean:
	rm -rf bin obj

bin:
	mkdir -p bin

bin/filesystem: bin obj/filesystem.o obj/device.o obj/main.o
	g++ -g -o bin/filesystem obj/* $(LINKFLAGS)

obj:
	mkdir -p obj

obj/main.o: obj main.c filesystem.h
	gcc -g $(CFLAGS) -c main.c -o $@

obj/filesystem.o: obj filesystem.c filesystem.h 
	g++ -g $(CFLAGS) -c filesystem.c -o $@

obj/device.o: obj device.c device.h
	g++ -g $(CFLAGS) -c device.c -o $@