#pragma once

#include "datatypes.lua"
#include "octets.lua"

-- __FILE__

local function is_label(text)
	checks 'string'

	return text:match '^[._%w]+:.*$'
end

local function parse_label(text)
	checks 'string'

	local name = text:match '^([._%w]+):.*$'
	local value = math.tointeger(text:match('^[._%w]+:(.+)$'))
	return name, value
end

local function substitute_labels(text, labels)
	checks('string', 'table')

	return text:gsub('%[(%w+)%]([._%w]+)', function(size, name)
		checks('string|int', 'string')

		-- references in form of "[fmt]name"
		local value = labels[name]
		if not value then
			error("No such label: " .. tostring(name))
		end
		return int_to_octets(size, value)
	end):gsub('%[(%w+)%]%(([^)]+)%)', function(size, constant)
		checks('string|int', 'int')

		-- references in form of "[fmt](constant)"
		constant = math.tointeger(constant)
		if not constant then
			error("Expected integer constant: " .. tostring(constant))
		end
		return int_to_octets(size, constant)
	end)
end
