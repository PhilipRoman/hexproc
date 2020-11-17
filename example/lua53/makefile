.POSIX:

CPP:=$(shell command -v mcpp || command -v cpp || command -v clang-cpp)
LUA:=$(shell command -v lua5.3 || if lua -v | grep -q '5.3'; then command -v lua; fi)

ifeq ($(LUA),)
$(error Suitable Lua 5.3 installation not found; set LUA variable to override)
endif
ifeq ($(CPP),)
$(error Suitable C preprocessor installation not found; set LUA variable to override)
endif

.PHONY: lua clean

lua: hello.lua
	$(LUA) $<

hello.lua: lua_hello.hxp lua53.hxp
	$(CPP) $< | hexproc -b > $@

clean:
	rm -v hello.lua

