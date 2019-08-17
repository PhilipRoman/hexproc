#pragma once

#include "datatypes.lua"

-- __FILE__

local function string_to_octets(text)
	checks 'string'

	local t = {string.byte(text, 1, #text)}
	for i = 1, #t do
		t[i] = string.format("%x", t[i])
	end
	return t
end

local function int_to_octets(size, value)
	checks('int|string', 'int')

	size = size_of_type(size)
	-- format("%0Nx", n) returns 'n' as a N-digit hex string
	return string.format('%0'..(size * 2)..'x', value):gsub('%x%x', function(octet)
		-- add padding between octets
		return octet .. ' '
	end)
end

local function count_octets(line)
	checks 'string'

	local n = 0
	-- this is a cheap hack
	-- patterns don't allow overlapping matches
	-- duplicate all whitespace and then prepend and append a space
	-- for example, replace "xx xx xx" with " xx  xx  xx "
	-- then match '%s(%x%x)%s
	for _ in (' '..line:gsub('%s', '  ')..' '):gmatch '%s(%x%x)%s' do
		n = n + 1
	end
	for size in line:gmatch '%[(%w+)%][._%w]+' do
		n = n + size_of_type(size)
	end
	for size in line:gmatch '%[(%w+)%]%([._%w]+%)' do
		n = n + size_of_type(size)
	end
	return n
end
