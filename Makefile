CC=gcc
CFLAGS=$(shell pkg-config --cflags glib-2.0 gio-2.0)
LDFLAGS=$(shell pkg-config --libs glib-2.0 gio-2.0)

.c.o:
	$(CC) -c $< $(CFLAGS) $(DFLAGS) $(INCLUDES) # -o $@

vboxsync: vboxsync.o
	$(CC) -o vboxsync $< $(LDFLAGS)

#vboxsync: vboxsync.c
#	$(CC) -pedantic -std=c99 -o vboxsync vboxsync.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o vboxsync
