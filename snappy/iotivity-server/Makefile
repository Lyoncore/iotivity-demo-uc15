# -*- Mode: Makefile; indent-tabs-mode:t; tab-width: 4 -*-

all:

install:
	mkdir -p $(DESTDIR)/bin
	cp -a start-server.sh $(DESTDIR)/bin/start-server
	cp -a demoserver $(DESTDIR)/bin
	cp -ra lib $(DESTDIR)/
	chmod a+x $(DESTDIR)/bin/start-server
	chmod a+x $(DESTDIR)/bin/demoserver
