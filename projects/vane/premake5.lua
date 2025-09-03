project "vane"
    location "."
    kind "StaticLib"

    files {
        "src/**.c",
        "include/**.h"
    }

    includedirs { "include" }

    defines { "_FILE_OFFSET_BITS=64" }