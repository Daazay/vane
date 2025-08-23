workspace "vane"
    location (ROOT_PATH)

    configurations { "debug", "release", "sanitizer" }
    platforms { "x86", "x64" }

    targetdir (BUILD_BIN_PATH)
    objdir    (BUILD_OBJ_PATH)

    language "C"
    cdialect "C17"

    warnings "High"
    fatalwarnings { "All" }

    filter "configurations:debug"
        symbols "On"
        optimize "Off"

    filter "configurations:release"
        symbols "off"
        optimize "Full"

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }

    filter "system:linux or system:macosx"
        defines { "PLATFORM_UNIX" }

    filter { "toolset:gcc or toolset:clang" }
        buildoptions { "-Wall", "-Wextra", "-Werror" }

    filter { "toolset:gcc or toolset:clang", "configurations:debug" }
        buildoptions { "-fanalyzer" }

    filter "action:vs*"
        buildoptions { "/WX", "/permissive-" }

    filter { "action:vs*", "configurations:debug" }
        buildoptions { "/analyze" }