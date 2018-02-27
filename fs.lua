require "fs"

local hidden_files = {
    "garrysmod/lua/menu/fs.lua"
}

local not_allowed = {}
for k,v in pairs(hidden_files) do
    not_allowed[v] = true
end

local override = false

hook.Add("ShouldHideFile", "", function(path)
    if (override) then
        return false
    end
    if (not_allowed[path]) then
        return true
    end
end)

raw_include = raw_include or include

local pak = function(...)
    return {n = select("#", ...), ...}
end
local unpak = function(p)
    return unpack(p, 1, p.n)
end

include = function(...)
    override = true
    local rets = pak(CompileString("local t = ... return t(select(2, ...))", debug.getinfo(2).short_src)(raw_include, ...))
    override = false
    return unpak(rets)
end