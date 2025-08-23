project "vane"
    location "."
    kind "StaticLib"

    files {
        "src/**.c",
        "include/**.h"
    }

    includedirs { "include" }