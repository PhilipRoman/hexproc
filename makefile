.POSIX:
.SUFFIXES:

HFILES := $(wildcard *.h)

ifeq ($(origin CC),default)
CC = gcc
endif

.DEFAULT_GOAL := all

ifeq ($(OS),Windows_NT)
WINDOWS_CC ?= gcc
all := windows
else
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
all := linux
endif

ASCIIDOCTOR := $(strip $(shell command -v asciidoctor))
WKHTMLTOPDF := $(strip $(shell command -v wkhtmltopdf))

CFLAGS += -Wall -Wpedantic -pedantic \
	-DHEXPROC_DATE="\"$(shell export TZ=GMT; date --rfc-3339=seconds)\"" \
	-DHEXPROC_VERSION="\"$(shell cat VERSION)\"" \
	-DHEXPROC_COMPILER="\"$(CC)\""

ANALYSIS_FLAGS := -Wfloat-equal -Wwrite-strings \
	-Wswitch-enum -Wstrict-overflow=4 -DCLEANUP

SANITIZE_FLAGS := -Og -g -fsanitize=undefined -fsanitize=leak \
	-fsanitize=address -DCLEANUP

DEBUG_FLAGS := -Og -g -DCLEANUP
RELEASE_FLAGS := -O2 -s -DNDEBUG
CHECK_FLAGS := --std=c99 --std=c11 --enable=all -j6 --quiet

VALGRIND_FLAGS += --leak-check=full --leak-resolution=high --show-reachable=yes

.PHONY: all linux windows test benchmark benchmark-linux benchmark-windows valgrind sanitize analyze doc clean install uninstall

########################   COMPILING   ########################

all: linux
# Release targets
linux: build/linux/hexproc
build/linux/hexproc: hexproc.c $(HFILES)
	@mkdir -p build/linux
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -lm -o $@ $<

windows: build/windows/hexproc.exe
build/windows/hexproc.exe: hexproc.c $(HFILES)
	@mkdir -p build/windows
	$(WINDOWS_CC) $(CFLAGS) $(RELEASE_FLAGS) -lm -o $@ $<

# Sanitized executables for finding bugs
build/sanitized/hexproc: hexproc.c $(HFILES)
	@mkdir -p build/sanitized
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -lm -o $@ $<

# Debug targets for Valgrind, etc.
build/debug/hexproc: hexproc.c $(HFILES)
	@mkdir -p build/debug
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -lm -o $@ $<

#########################   TESTING   #########################

test: build/linux/hexproc
	$(SHELL) test/test.sh

ifeq ($(OS),Windows_NT)
benchmark: benchmark-windows
else
benchmark: benchmark-linux
endif

benchmark-linux: build/linux/hexproc
	$(SHELL) test/benchmark.sh $<

benchmark-windows: build/windows/hexproc.exe
	$(SHELL) test/benchmark.sh $<

valgrind: build/debug/hexproc
	valgrind $(VALGRIND_FLAGS) ./$< example/showcase.hxp > /dev/null

# Runs the sanitized executable
sanitize: build/sanitized/hexproc
	./$< example/showcase.hxp > /dev/null

analyze: hexproc.c $(HFILES)
	$(CC) $(CFLAGS) $(ANALYSIS_FLAGS) -fsyntax-only $<
	cppcheck $(CHECK_FLAGS) $^

#####################   DOCUMENTATION   #######################

doc: man/hexproc.1 man/hexproc.html man/hexproc.pdf

man/%.1: man/%.adoc
	$(ASCIIDOCTOR) -b manpage $<
man/%.html: man/%.adoc
	$(ASCIIDOCTOR) -b html5 $<
man/%.pdf: man/%.html
	$(WKHTMLTOPDF) $< $@

#####################    INSTALLATION    ######################

ifneq ($(OS),Windows_NT)

install: build/linux/hexproc
	@mkdir -p                     /usr/local/share/doc/hexproc/
	@cp -rv  example              /usr/local/share/doc/hexproc/
	@cp -v   man/hexproc.html     /usr/local/share/doc/hexproc/
	@cp -v   man/hexproc.1        /usr/local/share/man/man1/
	@cp -v   build/linux/hexproc  /usr/local/bin/
	@echo ======== INSTALLED HEXPROC ========

uninstall:
	@rm -v   /usr/local/share/man/man1/hexproc.1  || true
	@rm -rv  /usr/local/share/doc/hexproc/        || true
	@rm -v   /usr/local/bin/hexproc               || true
	@echo ======== UNINSTALLED HEXPROC ========

endif

#######################   CLEANUP   ###########################

clean:
	@rm -rv  build  || true
	@rm -v   man/hexproc.1     || true
	@rm -v   man/hexproc.html  || true
	@rm -v   man/hexproc.pdf   || true
