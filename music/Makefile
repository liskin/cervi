# GTK Cervi
# vim:set sw=8 nosta:
#
# Copyright (C) 2003  Tomas Janousek <tomi@nomi.cz>
# See file COPYRIGHT and COPYING

.PHONY: all clean install dep-check

WAVS=$(patsubst %.mid,%.wav,$(wildcard *.mid))

all: $(WAVS)
clean:
	$(RM) *.wav
install: dep-check $(WAVS)
	mkdir -p $(DESTDIR)/$(datadir)
	$(INSTALL) $(WAVS) $(DESTDIR)/$(datadir)/
%.wav: %.mid
	timidity -Ow1slM -A 200 -o $@ $< >/dev/null

dep-check:
ifndef datadir
	@echo PACKAGE is not defined, maybe you did not run this makefile from top Makefile
	@echo Do not run this Makefile directly, use top makefile instead
	@false
else
	@true
endif
