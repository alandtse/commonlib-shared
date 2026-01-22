# Minimal Implementation Examples

This guide shows complete, working examples of implementing the minimal interface for different types of games.

## Example 1: Single Runtime Game

Most games only need single runtime support. Here's a complete implementation:

### File Structure
```
commonlib-yourgame/
├── include/REL/YourGame/
│   ├── Module.h
│   └── Runtime.h
├── src/REL/
│   ├── Module.cpp
│   └── Runtime.cpp
└── xmake.lua
```

### include/REL/YourGame/Module.h
```cpp
#pragma once

#include <REL/Module.h>
#include "REX/REX/Singleton.h"

namespace REL::YourGame
{
    class Module : public REX::Singleton<Module>
    {
    public:
        [[nodiscard]] constexpr std::uintptr_t base() const noexcept 
        { 
            return REL::detail::ModuleBase::GetSingleton()->base(); 
        }
        
        [[nodiscard]] constexpr Version version() const noexcept
        {
            return REL::detail::ModuleBase::GetSingleton()->version();
        }
        
        // Add other delegating methods as needed...
    };
}
```

### include/REL/YourGame/Runtime.h
```cpp
#pragma once

#include "REX/BASE.h"

namespace REL::YourGame
{
    enum RuntimeIndex : std::size_t
    {
        Primary = 0
    };

    [[nodiscard]] std::size_t get_runtime_index() noexcept;
}
```

### src/REL/Module.cpp
```cpp
#include "REL/YourGame/Module.h"
#include "REL/YourGame/Runtime.h"

namespace REL::detail
{
    std::size_t get_runtime_index() noexcept
    {
        return YourGame::get_runtime_index();
    }
}
```

### src/REL/Runtime.cpp
```cpp
#include "REL/YourGame/Runtime.h"

namespace REL::YourGame
{
    std::size_t get_runtime_index() noexcept
    {
        return RuntimeIndex::Primary;  // Always return 0 for single runtime
    }
}
```

### xmake.lua
```lua
target("commonlib-yourgame")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("commonlib-shared")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})
```

### Usage
```cpp
// In your plugin code
#include "REL/YourGame/Module.h"

void some_function() {
    auto module = REL::YourGame::Module::GetSingleton();
    auto base = module->base();
    
    // Traditional ID usage (works unchanged)
    REL::ID some_func{12345};
    auto addr = some_func.address();
    
    // Single-runtime RelocationID (no difference from ID)
    REL::RelocationID other_func{67890};
    auto addr2 = other_func.address();
}
```

## Example 2: Dual Runtime Game (Fallout 4 Style)

For games with two runtime variants (e.g., Pre-NG and Next-Gen):

### include/REL/F4/Runtime.h
```cpp
#pragma once

#include "REX/BASE.h"
#include "REL/Version.h"

namespace REL::F4
{
    enum RuntimeIndex : std::size_t
    {
        PreNG = 0,    // Fallout 4 1.10.163 and earlier
        NG = 1        // Fallout 4 1.10.984 Next-Gen
    };

    [[nodiscard]] std::size_t get_runtime_index() noexcept;
    [[nodiscard]] std::size_t detect_runtime_from_version(const Version& version) noexcept;
}
```

### src/REL/Runtime.cpp
```cpp
#include "REL/F4/Runtime.h"
#include "REL/Module.h"

namespace REL::F4
{
    std::size_t get_runtime_index() noexcept
    {
        const auto module = detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        return detect_runtime_from_version(version);
    }

    std::size_t detect_runtime_from_version(const Version& version) noexcept
    {
        // Next-Gen detection based on version
        if (version >= Version{1, 10, 984}) {
            return RuntimeIndex::NG;
        }
        return RuntimeIndex::PreNG;
    }
}
```

### xmake.lua
```lua
target("commonlibf4")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_packages("commonlib-shared")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=2", {public = true})
```

### Usage
```cpp
// In your F4SE plugin code
#include "REL/F4/Module.h"

void hook_function() {
    // Multi-runtime RelocationID automatically selects correct ID
    inline constexpr REL::RelocationID PlayerCharacter_ctor{
        1234567,  // Pre-NG ID
        7654321   // NG ID
    };
    
    auto addr = PlayerCharacter_ctor.address();  // Gets correct address for current runtime
    
    // Install hook...
    REL::Relocation<decltype(&hook_impl)> target{PlayerCharacter_ctor};
    auto& trampoline = REL::GetTrampoline();
    trampoline.write_call<5>(target.address(), reinterpret_cast<std::uintptr_t>(&hook_impl));
}
```

## Example 3: Triple Runtime Game (Skyrim Style)

For games with three runtime variants (Original, Special Edition, VR):

### include/REL/SSE/Runtime.h
```cpp
#pragma once

#include "REX/BASE.h"
#include "REL/Version.h"

namespace REL::SSE
{
    enum RuntimeIndex : std::size_t
    {
        Original = 0,     // Classic Skyrim
        SE = 1,           // Special Edition  
        VR = 2            // VR Edition
    };

    [[nodiscard]] std::size_t get_runtime_index() noexcept;
    [[nodiscard]] std::size_t detect_runtime_from_version(const Version& version) noexcept;
}
```

### src/REL/Runtime.cpp
```cpp
#include "REL/SSE/Runtime.h"
#include "REL/Module.h"

namespace REL::SSE
{
    std::size_t get_runtime_index() noexcept
    {
        const auto module = detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        return detect_runtime_from_version(version);
    }

    std::size_t detect_runtime_from_version(const Version& version) noexcept
    {
        // VR detection (version pattern may vary)
        if (version >= Version{1, 4, 15}) {
            return RuntimeIndex::VR;
        }
        
        // SE detection  
        if (version >= Version{1, 5, 97}) {
            return RuntimeIndex::SE;
        }
        
        // Original Skyrim
        return RuntimeIndex::Original;
    }
}
```

### Usage with Smart Fallback
```cpp
// VR often reuses Original IDs when SE-specific changes don't apply
inline constexpr REL::RelocationID SomeFunction{
    12345,  // Original ID
    67890,  // SE ID  
    0       // VR ID (0 = fallback to Original automatically)
};

// The system automatically:
// - Uses 12345 for Original runtime
// - Uses 67890 for SE runtime  
// - Uses 12345 for VR runtime (fallback because VR ID is 0)
```

## Example 4: Advanced Runtime Detection

For complex detection scenarios:

### Custom Detection Logic
```cpp
namespace REL::YourGame
{
    std::size_t get_runtime_index() noexcept
    {
        const auto module = detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        const auto filename = module->filename();
        
        // Method 1: Filename-based detection
        if (filename.find(L"GameVR.exe") != std::wstring::npos) {
            return RuntimeIndex::VR;
        }
        
        // Method 2: Version-based detection with complex logic
        if (version >= Version{2, 0, 0}) {
            return RuntimeIndex::NextGen;
        }
        
        // Method 3: Registry-based detection (Windows only)
        #ifdef _WIN32
        // Check registry for installation type...
        #endif
        
        // Method 4: File signature detection
        // Check for specific files or signatures in game directory...
        
        // Method 5: Command line argument detection
        // Parse command line for specific flags...
        
        return RuntimeIndex::Primary;  // Default fallback
    }
}
```

### Environment-Based Detection
```cpp
namespace REL::YourGame
{
    std::size_t get_runtime_index() noexcept
    {
        // Check environment variable
        if (const char* runtime_env = std::getenv("YOURGAME_RUNTIME")) {
            if (std::strcmp(runtime_env, "NG") == 0) {
                return RuntimeIndex::NextGen;
            }
            if (std::strcmp(runtime_env, "VR") == 0) {
                return RuntimeIndex::VR;
            }
        }
        
        // Fallback to version detection
        const auto module = detail::ModuleBase::GetSingleton();
        return detect_runtime_from_version(module->version());
    }
}
```

## Common Patterns

### Pattern 1: Graceful Degradation
```cpp
// When some runtimes don't have certain functions
inline constexpr REL::RelocationID NewFeatureFunction{
    0,      // Original - feature didn't exist
    12345,  // Remaster - feature added
    12345   // VR - reuses Remaster implementation
};

if (NewFeatureFunction.id() != 0) {
    // Feature is available in this runtime
    auto addr = NewFeatureFunction.address();
    // Use the feature...
}
```

### Pattern 2: Runtime-Specific Implementations
```cpp
void platform_specific_code() {
    const auto runtime = REL::YourGame::get_runtime_index();
    
    switch (runtime) {
        case REL::YourGame::RuntimeIndex::Original:
            // Original-specific code path
            break;
        case REL::YourGame::RuntimeIndex::Remaster:
            // Remaster-specific code path  
            break;
        case REL::YourGame::RuntimeIndex::VR:
            // VR-specific code path
            break;
    }
}
```

### Pattern 3: Development/Testing Support
```cpp
namespace REL::YourGame
{
    std::size_t get_runtime_index() noexcept
    {
        // Allow override for testing
        #ifdef _DEBUG
        if (const char* test_runtime = std::getenv("TEST_RUNTIME_INDEX")) {
            return static_cast<std::size_t>(std::atoi(test_runtime));
        }
        #endif
        
        // Normal detection logic
        return detect_runtime_from_version(
            detail::ModuleBase::GetSingleton()->version()
        );
    }
}
```

## Build System Integration

### XMake Advanced Configuration
```lua
-- Runtime configuration with validation
local runtimes = {
    {name = "PreNG", version = "1.10.163", id = 0},
    {name = "NG", version = "1.10.984", id = 1},
    {name = "VR", version = "1.2.72", id = 2}
}

local runtime_count = #runtimes

target("commonlibf4")
    -- Runtime count configuration
    add_defines("REL_DEFAULT_RUNTIME_COUNT=" .. runtime_count, {public = true})
    
    -- Runtime metadata (for debugging/validation)
    for i, runtime in ipairs(runtimes) do
        add_defines("REL_RUNTIME_" .. runtime.id .. "_NAME=\"" .. runtime.name .. "\"", {public = true})
        add_defines("REL_RUNTIME_" .. runtime.id .. "_VERSION=\"" .. runtime.version .. "\"", {public = true})
    end
```

### CMake Integration
```cmake
# For CMake users
target_compile_definitions(commonlibf4 PUBLIC REL_DEFAULT_RUNTIME_COUNT=2)
target_compile_definitions(commonlibf4 PUBLIC REL_RUNTIME_0_NAME="PreNG")
target_compile_definitions(commonlibf4 PUBLIC REL_RUNTIME_1_NAME="NG")
```

These examples show the complete implementation pattern that minimizes code while providing maximum flexibility for different game configurations.