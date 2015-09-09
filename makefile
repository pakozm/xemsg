LIBS := $(shell pkg-config --libs libnanomsg) $(shell pkg-config --libs lua5.2) -pthread -lm
CFLAGS := $(shell pkg-config --cflags libnanomsg) $(shell pkg-config --cflags lua5.2) -O2 -shared -fPIC 

all: xemsg.so

xemsg.so: xemsg.c
	$(CC) $(CFLAGS) -o xemsg.so xemsg.c $(LIBS)

clean:
	rm -f xemsg.so
