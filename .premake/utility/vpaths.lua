
local function default(baseDir)
    baseDir = baseDir or ''
    return {
        ['Header Files/*'] = {
            baseDir .. '**.h',
            baseDir .. '**.hpp',
        },
        ['Source Files/*'] = {
            baseDir .. '**.asm',
            baseDir .. '**.c',
            baseDir .. '**.cc',
            baseDir .. '**.cpp',
        },
        ['Form Files/*'] = baseDir .. '**.ui',
        ['Resources'] = {
            baseDir .. '**.ico',
            baseDir .. '**.manifest',
            baseDir .. '**.qrc',
            baseDir .. '**.rc*',
        },
        [''] = {
            '**.def',
            '**.lua',
        },
    }
end

premake.override(_G, 'vpaths', function(orig, opts)
    if type(opts) == 'table' then
        local def = opts.default or opts[1]
        if def then
            local opts = table.shallowcopy(opts)
            opts[1] = nil
            opts.default = nil
            return vpaths(table.merge(default(def), opts))
        else
            return orig(table.merge(default(), opts))
        end
    elseif type(opts) == 'string' then
        return vpaths({default = opts})
    else
        error('bad argument', 2)
    end
end)
