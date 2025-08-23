project "testbed"
    location "."
    kind     "ConsoleApp"

    files { "src/**.c" }

    includedirs { path.join(PROJECTS_PATH, "vane/include") }

    links { "vane" }