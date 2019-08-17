#pragma once

#ifdef _VERSION
#error Name clash with "_VERSION" of Lua
#endif

#ifndef HEXPROC_VERSION
#error You must define "HEXPROC_VERSION" to be a quoted version string
#endif

if ... == "-v" or ... == "--version" then
	print("hexproc "..HEXPROC_VERSION.." ["..__DATE__.."] running on ".._VERSION)
	return
elseif ... == "-h" or ... == "--help" then
	print 'Usage:'
	print '  hexproc [OPTION|FILE]'
	print 'Options:'
	print '  -v, --version: print program version and exit'
	print '  -h, --help: print this message and exit'
	print 'For more information, see the manual pages for hexproc(1)'
	return
elseif ... then
	local inputfile = ...
	io.input(inputfile)
end

if not _VERSION:match '5%.3' then
	io.stderr:write '# \n'
	io.stderr:write '# Warning! You are using an older Lua version \n'
	io.stderr:write '# hexproc may or may not work correctly \n'
	io.stderr:write '# \n'
	-- let's be nice and try to keep this compatible
	-- add missing functions here:
	math.tointeger = tonumber
end
