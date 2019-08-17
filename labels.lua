#pragma once

#include "datatypes.lua"
#include "shunting-yard.lua"
#include "octets.lua"

-- __FILE__

local function is_label(text)
	checks 'string'

	return text:match '^[._%w]+:.*$'
end

local function evaluate_label(text, labels)
	checks('string', 'table')

	if text:match '^%(.+%)$' then
		-- remove outer parenthesis
		text = text:match '^%((.+)%)$'
	end

	local yard = new_shunting_yard()
	for token in text:gmatch '%S+' do
		yard:accept(token)
	end

	-- the function which will be called to expand tokens
	local function callback(expr)
		if tonumber(expr) then
			return tonumber(expr)
		elseif labels[expr] then
			return evaluate_label(tostring(labels[expr]), labels)
		end
		error("Bad expression: " .. tostring(expr))
	end

	local result = yard:compute(callback)
	if DEBUG then
		io.stderr:write("# ", text, " => ", tostring(result), " \n")
	end
	return assert(math.tointeger(result))
end

local function parse_label(text)
	checks 'string'

	local name = text:match '^([._%w]+):.*$'
	local value = text:match('^[._%w]+:(.+)$')
	return name, value
end

local function substitute_labels(text, labels)
	checks('string', 'table')

	local function replace(type, expr)
		checks('string|int', 'string')

		local value = evaluate_label(expr, labels)
		return int_to_octets(type, value)
	end
	return text
		:gsub('%[(%w+)%](%b())', replace)
		:gsub('%[(%w+)%]([%w._]+)', replace)
end
