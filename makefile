.PHONY: test check clean docs
.DEFAULT_GOAL := hexproc

LUA_FILES := $(wildcard *.lua)
HEXPROC_VERSION := $(shell cat VERSION)
CPP_FLAGS := -P -traditional-cpp

# use c preprocessor to combine the lua files
hexproc: $(LUA_FILES) VERSION
	# clear output file and add shebang on first line
	echo '#!/usr/bin/env lua5.3' > $@
	# append the resulting script to the output file
	cpp $(CPP_FLAGS) -DHEXPROC_VERSION=\"$(HEXPROC_VERSION)\" hexproc.lua >> $@
	chmod +x $@

check: hexproc test
	type luacheck && luacheck $<

test: hexproc test/source.txt
	./hexproc test/source.txt

clean:
	rm -v hexproc || true
	rm -v -r doc/man/ || true
	rm -v -r doc/html/ || true

doc/man/%.1: doc/%.adoc VERSION
	mkdir -p doc/man/
	asciidoctor -b manpage -D doc/man/ $<

doc/html/%.html: doc/%.adoc VERSION
	mkdir -p doc/html/
	asciidoctor -b html5 -D doc/html/ $<

docs: doc/html/hexproc.html doc/man/hexproc.1
