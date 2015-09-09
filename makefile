PREFIX := /usr
LIBS := $(shell pkg-config --libs libnanomsg) $(shell pkg-config --libs lua5.2) -lm
CFLAGS := $(shell pkg-config --cflags libnanomsg) $(shell pkg-config --cflags lua5.2) -O2 -shared -fPIC 
LUADIR := $(PREFIX)/lib/lua/5.2

all: xemsg.so

xemsg.so: xemsg.c
	$(CC) $(CFLAGS) -o xemsg.so xemsg.c $(LIBS)

install:
	install xemsg.so $(LUADIR)

uninstall:
	rm -f $(LUADIR)/xemsg.so

clean:
	rm -f xemsg.so
