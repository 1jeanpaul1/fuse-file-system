CFLAGS = -g -Wall -DFUSE_USE_VERSION=30 `pkg-config fuse --cflags`
LINKFLAGS = -Wall `pkg-config fuse --libs`

all: obj/device

clean:
	rm -rf bin obj

bin:
	mkdir -p bin

obj:
	mkdir -p obj

obj/device.o: obj device.c device.h
	g++ -g $(CFLAGS) -c device.c -o $@