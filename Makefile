PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -lcurl

dcli: dcli.cpp
	$(CXX) $(CXXFLAGS) -o dcli dcli.cpp $(LDFLAGS)

install: dcli
	mkdir -p $(BINDIR)
	cp dcli $(BINDIR)/
	chmod +x $(BINDIR)/dcli

uninstall:
	rm -f $(BINDIR)/dcli

.PHONY: install uninstall

