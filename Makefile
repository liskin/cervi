# GTK Cervi
# vim:set sw=8 nosta:
#
# Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
# See file COPYRIGHT and COPYING

# This you may want to change
RELEASE=no
DEBUG=no
prefix=/usr
incdir=$(prefix)/include
bindir=$(prefix)/bin
datadir=$(prefix)/share/cervi
sysconfdir=/etc


# And below this line do not change anything if not needed
export
VERSION=0.0.4
PACKAGE=cervi
CFLAGS=-Wall -D_GNU_SOURCE -D_REENTRANT -DVERSION=\"$(VERSION)\" \
       $(shell gtk-config --cflags) $(shell pkg-config --cflags esound) \
       -DDATADIR=\"$(datadir)\"
CXXFLAGS=-Wall -D_GNU_SOURCE -D_REENTRANT -DVERSION=\"$(VERSION)\" \
	 $(shell gtk-config --cflags) $(shell pkg-config --cflags esound) \
	 -DDATADIR=\"$(datadir)\"
CPPFLAGS=
LDFLAGS=
LDLIBS=-lm $(shell gtk-config --libs) $(shell pkg-config --libs esound) \
       -lpthread
LINK.o=$(CXX) $(LDFLAGS) $(TARGET_ARCH)
MAIN=cervi
INSTALL=install -c -m 644
INSTALL_BIN=install -c -m 755

ifeq ($(RELEASE),yes)
 CFLAGS += -O2
 CXXFLAGS += -O2
 LDFLAGS += -s -Wl,-O,2
endif

ifeq ($(DEBUG),yes)
 CFLAGS += -g -DDEBUG
 CXXFLAGS += -g -DDEBUG
else
 CFLAGS += -DNDEBUG
 CXXFLAGS += -DNDEBUG
endif

.PHONY: all backup clean tags DEBUG RELEASE install clean-music \
	install-music all-music
all: all-music $(MAIN)
all-music:
	$(MAKE) -C music
backup:
	./backup.sh $(PACKAGE)-$(VERSION)
clean: clean-music
	$(RM) $(MAIN) *.o
clean-music:
	$(MAKE) -C music clean
tags:
	ctags -R .
DEBUG: clean
	$(MAKE) DEBUG=yes
RELEASE: clean
	$(MAKE) RELEASE=yes
install: all install-music
	$(INSTALL_BIN) $(MAIN) $(bindir)
install-music: all-music
	$(MAKE) -C music install

$(MAIN): main.o keymap.o game.o field.o music.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
main.o: main.cc field.h game.h music.h
keymap.o: keymap.cc keymap.h
game.o: game.cc game.h field.h keymap.h music.h
game.h: keymap.h field.h
field.o: field.cc field.h
music.o: music.cc music.h
