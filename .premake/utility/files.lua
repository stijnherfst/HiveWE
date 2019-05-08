
premake.override(_G, 'files', function(orig, files)
    return orig(table.join({'premake5.lua'}, files))
end)
