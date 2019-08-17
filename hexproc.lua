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

local output = {}
local echo = function(x)
	output[#output+1] = x
end

local labels = {}
local offset = 0

-- first pass: label recording and string expansion
for line in io.lines() do
	line = line:gsub('^%s*', ''):gsub('%s*$', ''):gsub("^#.+", "")
	line = line:gsub('"([^"]+)"', function(text)
		return table.concat(string_to_octets(text), ' ')
	end)
	if is_label(line) then
		local name, value = parse_label(line)
		labels[name] = value or offset
		echo '' -- to keep the original lines numbers
	else
		offset = offset + count_octets(line)
		echo(line)
	end
end

-- second pass: label reference substitution
for i = 1, #output do
	output[i] = substitute_labels(output[i], labels)
end

-- and finally, print all lines
for i = 1, #output do
	print(output[i])
end
