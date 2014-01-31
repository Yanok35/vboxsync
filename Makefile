CC=gcc
CFLAGS=$(shell pkg-config --cflags glib-2.0 gio-2.0)
LDFLAGS=$(shell pkg-config --libs glib-2.0 gio-2.0)

OBJECTS=vboxsync.o cfgfile.o

all: vboxsync

.c.o:
	$(CC) -g -c -Wno-format $< $(CFLAGS) $(DFLAGS) $(INCLUDES) # -o $@

vboxsync: $(OBJECTS)
	$(CC) -g -o vboxsync $^ $(LDFLAGS)

#vboxsync: vboxsync.c
#	$(CC) -pedantic -std=c99 -o vboxsync vboxsync.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o vboxsync
