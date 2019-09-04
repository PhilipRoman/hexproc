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

local hexdigits = {
	[0] = '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
}

local function nth_byte(value, n)
	checks 'int'

	local bitindex = (n - 1) * 8
	return (value >> bitindex) & 0xff
end

local function int_to_octets(size, value)
	checks('int|string', 'int')

	size = size_of_type(size)

	local result = {}
	for i = size, 1, -1 do
		local byte = nth_byte(value, i)
		local hi, lo = byte & 0xf, (byte >> 4) & 0xf
		local len = #result
		result[len+1] = hexdigits[hi]
		result[len+2] = hexdigits[lo]
		result[len+3] = ' '
	end

	return table.concat(result)
end

local function count_octets(line)
	checks 'string'

	local n = 0
	for _ in line:gmatch '(%x%x)' do
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
