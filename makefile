HFILES := $(wildcard *.h)

CC := clang
WINDOWS_CC := x86_64-w64-mingw32-gcc

CFLAGS := -std=c99 -pedantic -lm -pipe \
	-DHEXPROC_DATE="\"$(shell date --universal)\"" \
	-DHEXPROC_VERSION="\"$(shell cat VERSION)\""

ANALYSIS_FLAGS := -Wfloat-equal -Wstrict-overflow=4 -Wwrite-strings \
	-Wswitch-enum -Wconversion -DCLEANUP

SANITIZE_FLAGS := -O0 -g -fsanitize=undefined -fsanitize=leak \
	-fsanitize=address -DCLEANUP

DEBUG_FLAGS := -O0 -g -DCLEANUP

RELEASE_FLAGS := -Os -O3 -march=native -funroll-loops

CHECK_FLAGS := --std=c99 --std=c99 --enable=all -j6 --quiet -I/usr/include/

.PHONY: windows test check valgrind sanitize analyze doc clean
.DEFAULT_GOAL := build/hexproc

build/hexproc: hexproc.c $(HFILES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $<
	strip $@

windows: build/hexproc-win64.exe

build/hexproc-win64.exe: hexproc.c $(HFILES)
	@mkdir -p build
	$(WINDOWS_CC) $(CFLAGS) $(WINDOWS_FLAGS) -o $@ $<

build/hexproc-sanitized: hexproc.c $(HFILES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $@ $<

build/hexproc-debug: hexproc.c $(HFILES)
	@mkdir -p build
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $<

test: build/hexproc
	./$< example/showcase.hxp

valgrind: build/hexproc-debug
	valgrind --leak-check=full ./$< example/showcase.hxp > /dev/null

sanitize: build/hexproc-sanitized
	./$< example/showcase.hxp > /dev/null

analyze: hexproc.c $(HFILES)
	cppcheck $(CHECK_FLAGS) $^
	$(CC) $(CFLAGS) $(ANALYSIS_FLAGS) -fsyntax-only $<

check: analyze valgrind sanitize

doc: man/hexproc.1 man/hexproc.html

man/%.1: man/%.adoc
	asciidoctor -b manpage $<

man/%.html: man/%.adoc
	asciidoctor -b html5 $<

clean:
	rm -r -v build || true
