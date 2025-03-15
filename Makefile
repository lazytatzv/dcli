PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

install: dcli
	mkdir -p $(BINDIR)
	cp dcli $(BINDIR)/
	chmod +x $(BINDIR)/dcli

uninstall:
	rm -f $(BINDIR)/dcli

.PHONY: install uninstall

