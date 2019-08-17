#pragma once

-- __FILE__

local aliases = {
	int = 4,
	ptr = 4,
}

local function size_of_type(x)
	if aliases[x] then
		return aliases[x]
	end
	if tonumber(x) then
		return tonumber(x)
	end
	error('invalid type name: ' .. tostring(x))
end
