#pragma once

-- Shunting Yard Algorithm by Edsger W. Dijkstra

local precedence = {
	['+'] = 10;
	['-'] = 10;
	['*'] = 20;
	['/'] = 20;
	['^'] = 30;
}

local leftassoc = {
	['+'] = true;
	['-'] = true;
	['*'] = true;
	['/'] = true;
}

local yard_eval = {
	['+'] = function(a, b) return a + b end;
	['-'] = function(a, b) return a - b end;
	['*'] = function(a, b) return a * b end;
	['/'] = function(a, b) return a / b end;
	['^'] = function(a, b) return a ^ b end;
}

local function yard_peek(yard)
	checks 'table'

	return yard[#yard]
end

local function yard_pop(yard)
	checks 'table'

	local tmp = yard[#yard]
	yard[#yard] = nil
	return tmp
end

local function yard_accept(yard, token)
	checks('table', 'string')

	-- if token is operator
	if token:match "[+%-*/%^]" then
		-- while top has greater precedence OR top has equal precedence but is left assoc
		while
			#yard > 0 and (
				precedence[yard:peek()] > precedence[token]
				or (leftassoc[yard:peek()] and precedence[yard:peek()] == precedence[token])
			)
		do
			yard:append(yard:pop())
		end
		yard[#yard + 1] = token
	else
		yard:append(token)
	end
end

local function yard_append(yard, value)
	checks('table', 'string')

	yard.output[#(yard.output) + 1] = value
end

local function yard_compute(yard, callback)
	checks('table', 'function')

	for i = 1, #yard do
		yard:append(yard:pop())
	end

	for i = 1, #(yard.output) do
		local token = yard.output[i]
		if token:match "[+%-*/%^]" then
			local b = assert(yard:pop())
			local a = assert(yard:pop())
			yard[#yard + 1] = yard_eval[token](a, b)
		else
			yard[#yard + 1] = callback(token)
		end
	end

	return assert(yard:pop())
end

local function new_shunting_yard()
	return {
		peek = yard_peek,
		pop = yard_pop,
		append = yard_append,
		accept = yard_accept,
		compute = yard_compute,
		output = {}
	}
end

-- use like this:
--[[
local yard = new_shunting_yard()
yard:accept("3")
yard:accept("+")
yard:accept("4")
yard:accept("*")
yard:accept("2")
assert(yard:compute() == 11)
]]
