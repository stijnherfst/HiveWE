

project 'HiveWE'
    language 'C++'
    cppdialect 'C++17'
    qtgenerateddir '$(IntDir)/Generated Files/$(Configuration)/'
    debugdir ( abs 'HiveWE' )

    -- links
    links { 
        'casc.lib',
        'turbojpeg-static.lib',
        'opengl32.lib',
        'glu32.lib',
    }

    -- code
    files { '**.h', '**.cpp', '**.ui', '**.rc' }
    pchheader 'stdafx.h'
    pchsource 'stdafx.cpp'

    excludes { 
        'Dependencies/**.*',
        'BinaryWriter.cpp',
        'GeneratedFiles/**.*',
    }

    vpaths {}

    includedirs {
        abs 'HiveWE',
        abs 'HiveWE/Base',
        abs 'HiveWE/Base/Brush',
        abs 'HiveWE/Base/File Formats',
        abs 'HiveWE/Base/Resources',
        abs 'HiveWE/Menus',
        abs 'HiveWE/Menus/Custom Widgets',
        abs 'HiveWE/Dependencies/glm-0.9.9.3',
        abs 'HiveWE/Dependencies/soil2/include',
        abs 'HiveWE/Dependencies/stormlib-9.21/include',
        abs 'HiveWE/Dependencies/libjpeg-turbo-1.5.2/include',
        abs 'HiveWE/Dependencies/casclib-1.11/include',
        abs 'HiveWE/Dependencies/qscintilla-2.10.7/include',
    }

    qt.enable()
    qtmodules { 'core', 'gui', 'opelgl', 'widgets' }

    buildoptions { '/permissive-' }

    filter 'configurations:Debug'
        kind 'ConsoleApp'
        links { 
            'qtmaind.lib', 
            'StormLibDUS.lib',
            'soil2d.lib',
            'qscintilla2_qt5d.lib', 
        }

        libdirs {
            abs 'HiveWE/Dependencies/soil2/debug',
            abs 'HiveWE/Dependencies/stormlib-9.21/debug',
            abs 'HiveWE/Dependencies/libjpeg-turbo-1.5.2/debug',
            abs 'HiveWE/Dependencies/casclib-1.11/debug',
            abs 'HiveWE/Dependencies/qscintilla-2.10.7/debug',
        }

    filter 'configurations:Release'
        kind 'WindowedApp'
        defines { 'QT_NO_DEBUG' }

        links { 
            'qtmain.lib',
            'StormLibRUS.lib',
            'soil2.lib',
            'qscintilla2_qt5.lib', 
        }

        libdirs {
            abs 'HiveWE/Dependencies/soil2/release',
            abs 'HiveWE/Dependencies/stormlib-9.21/release',
            abs 'HiveWE/Dependencies/libjpeg-turbo-1.5.2/release',
            abs 'HiveWE/Dependencies/casclib-1.11/release',
            abs 'HiveWE/Dependencies/qscintilla-2.10.7/release',
        }

    filter { 'system:windows' }
        platforms   { 'x86', 'x64' }
