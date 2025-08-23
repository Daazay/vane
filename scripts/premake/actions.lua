-- utility functions
local function remove_files(patterns)
    for _, pattern in ipairs(patterns) do
        local matches = os.matchfiles(pattern)
        for i, path in ipairs(matches) do
            printf("Removing file: %s", path)
            local ok, err = os.remove(path)
            if not ok then
                printf("  Error: %s", err)
            end
        end
    end
end

local function remove_dirs(patterns)
    for _, pattern in ipairs(patterns) do
        local matches = os.matchdirs(pattern)
        for i, path in ipairs(matches) do
            printf("Removing directory: %s", path)
            local ok, err = os.rmdir(path)
            if not ok then
                printf("  Error: %s", err)
            end
        end
    end
end

-- patterns
local VS_DIR_PATTERNS = {
    path.join(ROOT_PATH, "*.vs")
}

local VS_FILE_PATTERNS = {
    path.join(ROOT_PATH, "*.sln"),
    path.join(ROOT_PATH, "**.vcxproj*"),
    path.join(ROOT_PATH, "**.vcxproj.user")
}

local MAKEFILE_PATTERNS = {
    path.join(ROOT_PATH, "**Makefile"),
    path.join(ROOT_PATH, "**.make"),
    path.join(ROOT_PATH, "Makefile")
}

local BUILD_PATTERNS = {
    path.join(ROOT_PATH, "build/*")
}

-- actions
newaction {
    trigger = "clean",
    description = "Remove all generated files and build directory",
    execute = function()
        remove_dirs(BUILD_PATTERNS)
        remove_dirs(VS_DIR_PATTERNS)
        remove_files(VS_FILE_PATTERNS)
        remove_files(MAKEFILE_PATTERNS)
    end
}