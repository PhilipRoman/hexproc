HFILES := $(wildcard *.h)

CC := clang
WINDOWS_CC := x86_64-w64-mingw32-gcc

CFLAGS := -std=c99 -pedantic -lm -pipe \
	-DHEXPROC_DATE="\"$(shell date --universal)\"" \
	-DHEXPROC_VERSION="\"$(shell cat VERSION)\""

ANALYSIS_FLAGS := -Wfloat-equal -Wstrict-overflow=4 -Wwrite-strings \
	-Wswitch-enum -Wconversion -DCLEANUP

SANITIZE_FLAGS := -Og -g -fsanitize=undefined -fsanitize=leak \
	-fsanitize=address -DCLEANUP

DEBUG_FLAGS := -Og -g -DCLEANUP
RELEASE_FLAGS := -Os -O3 -march=native -funroll-loops
CHECK_FLAGS := --std=c99 --std=c99 --enable=all -j6 --quiet -I/usr/include/

.PHONY: linux windows test check valgrind sanitize analyze doc clean
.DEFAULT_GOAL := linux

###############################################################
########################   COMPILING   ########################
###############################################################

# Release targets
linux: build/linux/hexproc
build/linux/hexproc: build/linux/hexproc.o
	@mkdir -p build/linux
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^
	strip $@

build/linux/%.o: %.c $(HFILES)
	@mkdir -p build/linux
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<


windows: build/windows/hexproc.exe
build/windows/hexproc.exe: build/windows/hexproc.o
	@mkdir -p build/windows
	$(WINDOWS_CC) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^
	strip $@

build/windows/%.o: %.c $(HFILES)
	@mkdir -p build/windows
	$(WINDOWS_CC) $(CFLAGS) $(RELEASE_FLAGS) -c -o $@ $<

# Sanitized executables for finding bugs
build/sanitized/hexproc: build/sanitized/hexproc.o
	@mkdir -p build/sanitized
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -o $@ $^

build/sanitized/%.o: %.c $(HFILES)
	@mkdir -p build/sanitized
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -c -o $@ $<

# Debug targets for Valgrind, etc.
build/debug/hexproc: build/debug/hexproc.o
	@mkdir -p build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

build/debug/%.o: %.c $(HFILES)
	@mkdir -p build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

###############################################################
#########################   TESTING   #########################
###############################################################

test: build/linux/hexproc
	./$< example/showcase.hxp

valgrind: build/debug/hexproc
	valgrind --leak-check=full ./$< example/showcase.hxp > /dev/null

# Runs the sanitized executable
sanitize: build/sanitized/hexproc
	./$< example/showcase.hxp > /dev/null

analyze: hexproc.c $(HFILES)
	cppcheck $(CHECK_FLAGS) $^
	$(CC) $(CFLAGS) $(ANALYSIS_FLAGS) -fsyntax-only $<

check: analyze valgrind sanitize

###############################################################
#####################   DOCUMENTATION   #######################
###############################################################

doc: man/hexproc.1 man/hexproc.html

man/%.1: man/%.adoc
	asciidoctor -b manpage $<

man/%.html: man/%.adoc
	asciidoctor -b html5 $<

###############################################################
#######################   CLEANUP   ###########################
###############################################################

clean:
	rm -r -v build || true
