project "tests"
    location "."
    kind     "ConsoleApp"
    warnings "Default"

    files { "src/**.c" }

    includedirs { path.join(PROJECTS_PATH, "vane/include") }
    externalincludedirs { DEPENDENCIES_PATH }

    links { "vane" }

    debugargs { "--enable-mixed-units" }

    filter "action:vs*"
        buildoptions { "/analyze-" }