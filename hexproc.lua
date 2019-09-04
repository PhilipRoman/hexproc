#include "interface.lua"

local checks_available, checks = pcall(function()
	require 'luarocks.loader'
	require 'checks'
	function _G.checkers.int(x)
		return math.tointeger(x)
	end
	return _G.checks
end)

if not checks_available then
	io.stderr:write '# \n'
	io.stderr:write '# Warning! "checks" library not available \n'
	io.stderr:write '# Type checking will be disabled \n'
	io.stderr:write '# \n'
	checks = function() end
end

#include "octets.lua"
#include "datatypes.lua"
#include "labels.lua"

-- __FILE__

local labels = {}
local offset = 0

local tmp = io.tmpfile()

-- first pass: label recording and string expansion
for line in io.lines() do
	-- replace string literals with octets
	line = line:gsub('"([^"]+)"', string_to_octets)
	-- remove leading and trailing whitespace
	line = line:gsub('^%s+', ''):gsub('%s+$', '')
	-- remove // ; and # comments
	line = line:gsub('(#.+)$', '')
	line = line:gsub('(//.+)$', '')
	line = line:gsub('(;.+)$', '')
	if is_label(line) then
		local name, value = parse_label(line)
		labels[name] = value or offset
		tmp:write('\n') -- to keep the original line numbers
	else
		offset = offset + count_octets(line)
		tmp:write(line, '\n')
	end
end

tmp:flush()
tmp:seek('set', 0)

-- second pass: label reference substitution and output
for line in tmp:lines() do
	line = substitute_formatters(line, labels)
	line = line:gsub('%s+', '')
	io.write(line:sub(1, 2))
	for i = 3, #line, 2 do
		io.write(' ', line:sub(i, i+1));
	end
	io.write '\n'
end

tmp:close()
