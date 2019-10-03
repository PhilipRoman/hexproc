HFILES := $(wildcard *.h)

CC := clang

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

.PHONY: all test check valgrind sanitize analyze doc clean
.DEFAULT_GOAL := hexproc

hexproc: hexproc.c $(HFILES)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $<
	strip $@

hexproc-sanitized: hexproc.c $(HFILES)
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $@ $<

hexproc-debug: hexproc.c $(HFILES)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $<

test: hexproc
	./$< example/showcase.hxp

valgrind: hexproc-debug
	valgrind --leak-check=full ./$< example/showcase.hxp > /dev/null

sanitize: hexproc-sanitized
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
	rm -v hexproc || true
	rm -v hexproc-debug || true
	rm -v hexproc-sanitized || true
