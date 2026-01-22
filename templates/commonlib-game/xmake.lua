-- CommonLib GAME Template - XMake Configuration
-- Replace "GAME" with your game name (e.g., F4, SSE, SF)

-- Game-specific configuration
local GAME_NAME = "GAME"  -- Replace with your game name
local VERSION = "1.0.0"   -- Your library version

-- Runtime configuration
-- Set this based on how many runtime variants your game has:
-- 1 = Single runtime (most common)
-- 2 = Dual runtime (Original + Remaster/Next-Gen)
-- 3 = Triple runtime (Original + Remaster + VR)
-- 4 = Quad runtime (for future expansion)
local RUNTIME_COUNT = 1

-- Supported runtimes for this game
-- Customize this table for your specific game versions
local runtimes = {
    {
        name = "Primary",
        version = "1.0.0",
        enabled = true,
        description = "Primary game version"
    },
    -- Example for multi-runtime games (uncomment and customize):
    -- {
    --     name = "Original",
    --     version = "1.0.0", 
    --     enabled = true,
    --     description = "Original/Legacy edition"
    -- },
    -- {
    --     name = "Remaster",
    --     version = "2.0.0",
    --     enabled = false,
    --     description = "Remastered/Anniversary/Next-Gen edition"
    -- },
    -- {
    --     name = "VR",
    --     version = "1.1.0",
    --     enabled = false,
    --     description = "VR edition"
    -- },
}

-- Project configuration
set_project("CommonLib" .. GAME_NAME)
set_version(VERSION)

-- Build modes
add_rules("mode.debug", "mode.release")

-- Compiler and language settings
set_languages("c++23")
set_warnings("allextra", "error")

if is_mode("debug") then
    add_defines("_DEBUG")
else
    add_defines("NDEBUG")
end

-- Shared library dependency
add_requires("commonlib-shared")

-- Main target
target("commonlib-" .. GAME_NAME:lower())
    set_kind("static")
    
    -- Language and standards
    set_languages("c++23")
    
    -- Include directories
    add_includedirs("include", {public = true})
    
    -- Source files
    add_files("src/**.cpp")
    
    -- Dependencies
    add_packages("commonlib-shared")
    
    -- Preprocessor definitions
    add_defines("REL_DEFAULT_RUNTIME_COUNT=" .. RUNTIME_COUNT, {public = true})
    
    -- Runtime configuration
    for i, runtime in ipairs(runtimes) do
        if runtime.enabled then
            add_defines("REL_RUNTIME_" .. i-1 .. "_NAME=\"" .. runtime.name .. "\"", {public = true})
            add_defines("REL_RUNTIME_" .. i-1 .. "_VERSION=\"" .. runtime.version .. "\"", {public = true})
        end
    end
    
    -- Platform-specific settings
    if is_plat("windows") then
        add_cxxflags("/utf-8", "/permissive-", "/Zc:preprocessor", "/external:W0", "/W4")
        add_defines("UNICODE", "_UNICODE", "WINVER=0x0601", "_WIN32_WINNT=0x0601")
        add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX")
    end
    
    -- Debug/Release specific settings
    if is_mode("debug") then
        add_cxxflags("/ZI", "/Od", "/MDd")
        add_defines("_DEBUG")
    else
        add_cxxflags("/O2", "/MD", "/Zi")
        add_defines("NDEBUG")
    end

-- Configuration summary
print("=== CommonLib" .. GAME_NAME .. " Configuration ===")
print("Library version: " .. VERSION)
print("Runtime count: " .. RUNTIME_COUNT)
print("Enabled runtimes:")
for i, runtime in ipairs(runtimes) do
    if runtime.enabled then
        print("  - " .. runtime.name .. " (" .. runtime.description .. ")")
    end
end
print("==================================")

-- Example: Build tests (optional)
-- target("tests")
--     set_kind("binary")
--     add_deps("commonlib-" .. GAME_NAME:lower())
--     add_files("tests/**.cpp")
--     set_default(false)

-- Example: Build examples (optional) 
-- target("examples")
--     set_kind("binary")
--     add_deps("commonlib-" .. GAME_NAME:lower())
--     add_files("examples/**.cpp")
--     set_default(false)