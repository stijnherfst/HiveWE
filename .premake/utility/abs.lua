
local _abs = abs

local function abs(value)
    return path.getabsolute(value, _MAIN_SCRIPT_DIR)
end

if _abs then
    _G.abs = function(value, ...)
        if type(value) == 'string' then
            return abs(value)
        else
            return _abs(value, ...)
        end
    end
else
    _G.abs = abs
end
