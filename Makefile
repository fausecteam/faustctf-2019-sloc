SERVICE := sloc
DESTDIR ?= dist_root
SERVICEDIR ?= /srv/$(SERVICE)

TESTFILES := $(wildcard checker/sloc/*.sl)

.PHONY: build install test

build: Compiler test

Compiler: Compiler.cpp
	g++ -O2 -s $^ -o $@

test: test.py $(TESTFILES)
	python3 test.py

install: build
	install -d -m 755 $(DESTDIR)/etc/systemd/system/
	install -d -m 755 $(DESTDIR)$(SERVICEDIR)
	install -m 755 Compiler $(DESTDIR)$(SERVICEDIR)/Compiler
	install -m 755 runner.py $(DESTDIR)$(SERVICEDIR)/runner.py
	install -m 444 systemd/sloc@.service $(DESTDIR)/etc/systemd/system/
	install -m 444 systemd/sloc.slice $(DESTDIR)/etc/systemd/system/
	install -m 444 systemd/sloc.socket $(DESTDIR)/etc/systemd/system/
