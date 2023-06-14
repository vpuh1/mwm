PREFIX = /usr/local


mwm: mwm.c config.h
	gcc mwm.c -Wall -o mwm -lX11

clean:
	rm -f mwm

install: mwm
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/mwm
	chmod 755 ${DESTDIR}${PREFIX}/bin/mwm
