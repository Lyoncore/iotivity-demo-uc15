# -*- Mode: Makefile; indent-tabs-mode:t; tab-width: 4 -*-

all:
	cd lib/i2c-tools && make

install:
	mkdir -p $(DESTDIR)/python-eggs
	mkdir -p $(DESTDIR)/bin
	cp -a start-server.sh $(DESTDIR)/bin/start-server
	cp -a python-server.py $(DESTDIR)/bin/python-server.py
	cp -ra python-env $(DESTDIR)/bin/python-env
	cp -ra pygrovepi $(DESTDIR)/bin/python-env/lib/python2.7/site-packages/
	cp lib/i2c-tools/py-smbus/build/lib.linux-armv7l-2.7/smbus.so $(DESTDIR)/bin/python-env/lib/python2.7/site-packages/
	mkdir -p $(DESTDIR)/lib
	cp lib/i2c-tools/lib/libi2c.so.0.1.0 $(DESTDIR)/lib/
	ln -s libi2c.so.0.1.0 $(DESTDIR)/lib/libi2c.so.0
	ln -s libi2c.so.0.1.0 $(DESTDIR)/lib/libi2c.so
	chmod a+x $(DESTDIR)/bin/start-server
