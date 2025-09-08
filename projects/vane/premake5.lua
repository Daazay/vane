project "vane"
    location "."
    kind "StaticLib"

    files {
        "src/**.c",
        "include/**.h"
    }

    includedirs { "include" }

    defines {
        "_FILE_OFFSET_BITS=64",
        "_POSIX_C_SOURCE=200809L",
        "_XOPEN_SOURCE=700",
    }