UNAME := $(shell uname)
PLAT= DetectOS

LUAPKG := $(shell ( luajit -e 'print("luajit")'	 2> /dev/null ) || lua5.2 -e 'print("lua5.2")'	2> /dev/null || lua5.1 -e 'print("lua5.1")'  2> /dev/null || lua -e 'print("lua" .. string.match(_VERSION, "%d+.%d+"))'	 2> /dev/null)

LUA:= $(shell echo $(LUAPKG) | sed 's/[0-9].[0-9]//g')
VER:= $(shell echo $(LUAPKG) | sed 's/.*\([0-9].[0-9]\)/\1/g')

NANOMSGFLAGS := $(shell pkg-config --cflags libnanomsg)
NANOMSGLIBS := $(shell pkg-config --libs libnanomsg)

# macports or homebrew
ifeq ("$(UNAME)","Darwin")
LUAFLAGS := $(shell pkg-config --cflags '$(LUA) >= $(VER)')
LUALIBS := $(shell pkg-config --libs '$(LUA) >= $(VER)')
OSFLAGS :=
OSLIBS := -bundle -rdynamic -flat_namespace
else
LUAFLAGS = $(shell pkg-config --cflags $(LUAPKG))
OSFLAGS:= -shared -fPIC
OSLIBS:=
endif

PREFIX := /usr
LUADIR := $(PREFIX)/lib/$(LUA)/$(VER)

CFLAGS := -O2 -Wall $(LUAFLAGS) $(NANOMSGFLAGS) $(OSFLAGS)
LIBS := $(LUALIBS) $(NANOMSGLIBS) $(OSLIBS)

all: check $(PLAT)

DetectOS: check
	@make $(UNAME)

Linux: xemsg.so

Darwin: xemsg.so

check:
	@if [ -z $(LUAPKG) ]; then echo "Impossible to detect Lua version, you need LuaJIT, Lua 5.1 or Lua 5.2 installed!"; exit 1; fi
	@if [ -z $(LUAFLAGS) ]; then echo "Unable to configure with pkg-config, luamongo needs developer version of $(LUAPKG). You can force other Lua version by declaring variable LUAPKG=lua5.1 or LUAPKG=lua5.2"; exit 1; fi

echo: check
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LIBS = $(LIBS)"
	@echo "LUAPKG = $(LUAPKG)"

xemsg.so: xemsg.c
	$(CC) $(CFLAGS) -o xemsg.so xemsg.c $(LIBS)

install:
	install xemsg.so $(LUADIR)

uninstall:
	rm -f $(LUADIR)/xemsg.so

clean:
	rm -f xemsg.so

.PHONY: all check clean DetectOS Linux Darwin echo install uninstall
