
require('premake-qt/qt')
require('utility')

qt = premake.extensions.qt

newoption {
    trigger = 'to',
    value   = 'path',
    description = 'Set the output location for the generated files',
    default = 'Build'
}

workspace 'HiveWE'
    configurations { 'Debug', 'Release' }
    location ( _OPTIONS['to'] )
    toolset 'v142'
    systemversion 'latest'
    symbols 'Full'
    architecture 'x64'

    flags {
        'MultiProcessorCompile',
    }

    startproject 'Client'

    -- Qt config
    qtprefix 'Qt5'

    -- global macros
    defines { 'WIN32', '_WINDOWS' }
    filter 'configurations:Debug'
        defines { '_DEBUG' }
        qtsuffix 'd'
        
    filter 'configurations:Release'
        defines { 'NDEBUG' }
        optimize 'On'

    filter { 'system:windows' }
        platforms   { 'x64' }
        debugenvs {
            'Path=$(QTDIR)/bin/;%Path%'
        }

    -- projects
    -- main project
    include 'HiveWE'
