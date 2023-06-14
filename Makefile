PREFIX = /usr/local


mwm: mwm.c config.h
	gcc mwm.c -Wall -o mwm -lX11

clean:
	rm -f mwm

install: mwm
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/mwm
	chmod 755 ${DESTDIR}${PREFIX}/bin/mwm
	touch mwm.desktop
	chmod 644 mwm.desktop
	echo "[Desktop Entry]\nName=mwm\nExec=$(DESTDIR)$(PREFIX)/bin/mwm\nPath=$(DESTDIR)$(PREFIX)/bin\nType=Applicaton" > mwm.desktop
	cp mwm.desktop /usr/share/xsessions
