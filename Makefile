include config.mk

SRC = main.c util.c image_io.c editor.c clipboard.c draw.c
OBJ = ${SRC:.c=.o}

all: options s2

options:
	@echo s2 build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "LDLIBS   = ${LDLIBS}"

config.h:
	cp config.def.h $@

${OBJ}: config.h config.mk
editor.o: assets/fonts/embedded.h assets/icons/masks.h assets/icons/rotate-cursor.h assets/icons/pan-cursors.h themes.h

assets/fonts/embedded.h: scripts/embed_fonts.py assets/fonts/Geist-Variable.ttf assets/fonts/Geist-Italic-Variable.ttf assets/fonts/OFL.txt
	python3 scripts/embed_fonts.py

vendor/cui/build/libcui.a:
	cmake -S vendor/cui -B vendor/cui/build -DCUI_BUILD_NATIVE=OFF
	cmake --build vendor/cui/build --target cui -j4

s2: ${OBJ} vendor/cui/build/libcui.a
	${CC} -o $@ ${OBJ} vendor/cui/build/libcui.a ${LDFLAGS} ${LDLIBS}

.c.o:
	${CC} -c ${CFLAGS} ${CPPFLAGS} $<

test-text: assets/fonts/embedded.h assets/icons/rotate-cursor.h assets/icons/pan-cursors.h themes.h config.h vendor/cui/build/libcui.a assets/icons/masks.h tests/text.c editor.c draw.c image_io.c clipboard.c util.c
	${CC} ${CFLAGS} ${CPPFLAGS} tests/text.c draw.c image_io.c clipboard.c util.c ${LDFLAGS} vendor/cui/build/libcui.a -o tests/text-test
	./tests/text-test

test-blur: tests/blur.c draw.c s2.h
	${CC} ${CFLAGS} ${CPPFLAGS} -I. tests/blur.c draw.c -lm -o tests/blur-test
	./tests/blur-test

clean:
	rm -f s2 ${OBJ} tests/blur-test tests/text-test

distclean: clean
	rm -f config.h

dist: clean
	mkdir -p s2-${VERSION}
	cp -R LICENSE Makefile README.md config.mk config.def.h *.c *.h SPEC.md cover-og.png vendor tests assets scripts docs CHANGELOG.md s2-${VERSION}
	rm -rf s2-${VERSION}/vendor/cui/build
	rm -f s2-${VERSION}/config.h
	tar -cf s2-${VERSION}.tar s2-${VERSION}
	gzip s2-${VERSION}.tar
	rm -rf s2-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install -m 755 s2 ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	install -m 644 docs/s2.1 ${DESTDIR}${MANPREFIX}/man1/s2.1
	mkdir -p ${DESTDIR}${PREFIX}/share/licenses/s2
	install -m 644 LICENSE assets/fonts/OFL.txt ${DESTDIR}${PREFIX}/share/licenses/s2/
	install -m 644 vendor/cui/LICENSE ${DESTDIR}${PREFIX}/share/licenses/s2/LICENSE-CUI
	install -m 644 vendor/cui/third_party/clay/LICENSE ${DESTDIR}${PREFIX}/share/licenses/s2/LICENSE-Clay

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/s2 ${DESTDIR}${MANPREFIX}/man1/s2.1
	rm -rf ${DESTDIR}${PREFIX}/share/s2 ${DESTDIR}${PREFIX}/share/licenses/s2

.PHONY: all options clean distclean dist install uninstall test-blur test-text
