# CommonLibF4 as Reference Implementation

CommonLibF4 serves as the reference implementation for the minimal interface pattern, demonstrating sophisticated multi-runtime support with complex build configurations.

## Overview

CommonLibF4 showcases:
- **Minimal interface implementation** requiring only one function
- **Three-runtime support** for Fallout 4 Pre-NG, NG, and VR variants
- **Sophisticated runtime detection** using filesystem and version analysis
- **Flexible build system** supporting 1-3 runtime configurations
- **Complex fallback logic** for VR compatibility

## File Structure

```
CommonLibF4/
├── include/
│   └── REL/
│       └── F4/
│           ├── Config.h              # Runtime configuration and validation
│           ├── Module.h              # F4 Module singleton wrapper
│           └── Runtime.h             # Runtime detection declarations
└── src/
    └── REL/
        ├── Module.cpp                # ONLY required function implementation
        └── Runtime.cpp               # Complex runtime detection logic
```

## Minimal Interface Implementation

### Core Implementation (`src/REL/Module.cpp`)

The **only** function that needs implementation:

```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return F4::get_runtime_index();  // Delegates to F4-specific logic
    }
}
```

**That's it!** Everything else (IDDB, Module, RelocationID, utilities) is provided by the shared library.

## Sophisticated Runtime Detection

### Runtime Types (`include/REL/F4/Runtime.h`)

```cpp
namespace REL::F4 {
    enum class Runtime {
        Unknown,  // Unknown runtime
        F4,       // Pre-NG Fallout 4 (1.10.163)
        NG,       // Next-Gen Fallout 4 (1.10.984+)
        VR        // Fallout 4 VR (1.2.x)
    };

    [[nodiscard]] Runtime GetRuntime() noexcept;
    [[nodiscard]] bool IsF4() noexcept;
    [[nodiscard]] bool IsNG() noexcept;
    [[nodiscard]] bool IsVR() noexcept;
}
```

### Complex Detection Logic (`src/REL/Runtime.cpp`)

The actual implementation uses sophisticated detection:

```cpp
namespace REL::F4 {
    Runtime detect_runtime() noexcept {
        if (g_runtime != Runtime::Unknown) {
            return g_runtime;  // Cached result
        }

        try {
            // 1. Filesystem-based detection
            if (std::filesystem::exists("Fallout4VR.exe")) {
                g_runtime = Runtime::VR;
                return g_runtime;
            }

            if (std::filesystem::exists("Fallout4.exe")) {
                // 2. Version-based detection for F4 vs NG
                const auto module = Module::GetSingleton();
                const auto version = module->version();
                
                // NG is version 1.10.980 and above
                if (version.major() == 1 && version.minor() == 10 && version.patch() >= 980) {
                    g_runtime = Runtime::NG;
                } else {
                    g_runtime = Runtime::F4;
                }
                return g_runtime;
            }

            // 3. Build-time fallbacks
            #if defined(ENABLE_FALLOUT_VR)
                g_runtime = Runtime::VR;
            #elif defined(ENABLE_FALLOUT_NG)
                g_runtime = Runtime::NG;
            #else
                g_runtime = Runtime::F4;
            #endif

        } catch (...) {
            // Exception handling with build defaults
            // ...
        }

        return g_runtime;
    }
}
```

### Runtime Index Mapping

The runtime index conversion handles multiple build configurations:

```cpp
std::size_t get_runtime_index() noexcept {
#ifdef REL_CURRENT_RUNTIME
    // Single runtime build - compile-time constant
    return REL_CURRENT_RUNTIME;
#else
    // Multi-runtime build - complex detection
    static std::size_t cached_index = []() -> std::size_t {
        const auto runtime = F4::GetRuntime();

        #if defined(ENABLE_FALLOUT_F4) && defined(ENABLE_FALLOUT_NG) && defined(ENABLE_FALLOUT_VR)
            // Three runtime build
            switch (runtime) {
                case F4::Runtime::F4: return F4::RuntimeIndex3::PRE_NG;
                case F4::Runtime::NG: return F4::RuntimeIndex3::NG_3;
                case F4::Runtime::VR: return F4::RuntimeIndex3::VR;
                default: return F4::RuntimeIndex3::PRE_NG;
            }
        #elif defined(ENABLE_FALLOUT_F4) && defined(ENABLE_FALLOUT_NG)
            // Two runtime build (traditional)
            switch (runtime) {
                case F4::Runtime::NG: return F4::RuntimeIndex::NG;
                default: return F4::RuntimeIndex::PRE_NG_VR;  // F4 and VR share
            }
        #elif defined(ENABLE_FALLOUT_F4) && defined(ENABLE_FALLOUT_VR)
            // F4 + VR build
            // ...additional configurations
        #endif
    }();

    return cached_index;
#endif
}
```

## Build System Integration

### Runtime Configuration (`include/REL/F4/Config.h`)

CommonLibF4 uses a sophisticated configuration system:

```cpp
// Validate that REL_DEFAULT_RUNTIME_COUNT is properly defined by build system
#ifndef REL_DEFAULT_RUNTIME_COUNT
#error "REL_DEFAULT_RUNTIME_COUNT must be defined by the build system (XMake)"
#endif

// Validate that at least one runtime is enabled
#if !defined(ENABLE_FALLOUT_F4) && !defined(ENABLE_FALLOUT_NG) && !defined(ENABLE_FALLOUT_VR)
#error "At least one Fallout runtime must be enabled"
#endif

namespace REL::F4 {
    // Dual runtime indices (traditional)
    enum RuntimeIndex : std::size_t {
        PRE_NG_VR = 0,  // Pre-NG and VR share same IDs (historical)
        NG = 1          // Next-Gen has separate IDs
    };

    // Triple runtime indices (explicit VR support)
    enum RuntimeIndex3 : std::size_t {
        PRE_NG = 0,  // Pre-NG only
        NG_3 = 1,    // Next-Gen
        VR = 2       // VR explicit (when different from Pre-NG)
    };
}
```

### XMake Build System

The actual build system calculates runtime count dynamically:

```lua
-- Runtime preset logic
local f4_enabled = has_config("fallout_f4")
local ng_enabled = has_config("fallout_f4ng") 
local vr_enabled = has_config("fallout_f4vr")

-- Enable runtime-specific defines
if f4_enabled then
    target:add("defines", "ENABLE_FALLOUT_F4=1", {public = true})
end
if ng_enabled then
    target:add("defines", "ENABLE_FALLOUT_NG=1", {public = true})
end
if vr_enabled then
    target:add("defines", "ENABLE_FALLOUT_VR=1", {public = true})
end

-- Calculate and define REL_DEFAULT_RUNTIME_COUNT for the build
local runtime_count = (f4_enabled and 1 or 0) + (ng_enabled and 1 or 0) + (vr_enabled and 1 or 0)
target:add("defines", "REL_DEFAULT_RUNTIME_COUNT=" .. runtime_count, {public = true})
```

The build system supports multiple preset configurations:
- **f4**: Pre-NG only (single runtime)
- **ng**: NG only (single runtime) 
- **all**: F4 + NG + VR (triple runtime)
- **Custom combinations** via individual flags

## Multi-Runtime RelocationID Usage

### Configuration Complexity

CommonLibF4 supports sophisticated RelocationID configurations:

```cpp
// Single ID (works in all runtimes with fallback)
REL::RelocationID func1{12345};

// Dual runtime (traditional F4 + NG)
REL::RelocationID PlayerCharacter_ctor{
    1234567,  // Pre-NG/VR ID (shared)
    7654321   // NG ID (explicit)
};

// Triple runtime (explicit VR support)
REL::RelocationID PlayerCharacter_update{
    1111111,  // Pre-NG ID
    2222222,  // NG ID  
    3333333   // VR ID (explicit)
};

// Smart fallback (VR reuses Pre-NG when ID missing)
REL::RelocationID SomeFunction{
    5555555,  // Pre-NG ID
    6666666,  // NG ID
    0         // VR ID = 0, automatically falls back to Pre-NG (5555555)
};
```

### Fallback Logic

CommonLibF4 includes custom fallback configuration:

```cpp
namespace REL::F4::Fallback {
    constexpr std::size_t VR_FALLBACK_INDEX = 0;  // VR falls back to Pre-NG

    constexpr std::size_t get_fallback_index(std::size_t a_runtime_index) noexcept {
        #ifdef ENABLE_FALLOUT_VR
            if (a_runtime_index == 2) {  // VR index in 3-runtime mode
                return VR_FALLBACK_INDEX;
            }
        #endif
        return a_runtime_index;  // No fallback for other runtimes
    }
}
```

## Build Configuration Examples

### Single Runtime Builds

```bash
# Build for Pre-NG only
xmake config --runtime_preset=f4
# Results in: REL_DEFAULT_RUNTIME_COUNT=1, REL_CURRENT_RUNTIME=0

# Build for NG only  
xmake config --runtime_preset=ng
# Results in: REL_DEFAULT_RUNTIME_COUNT=1, REL_CURRENT_RUNTIME=1
```

### Multi-Runtime Builds

```bash
# Build for all three runtimes
xmake config --runtime_preset=all
# Results in: REL_DEFAULT_RUNTIME_COUNT=3, dynamic runtime detection

# Custom dual runtime
xmake config --fallout_f4=true --fallout_f4ng=true --fallout_f4vr=false
# Results in: REL_DEFAULT_RUNTIME_COUNT=2
```

## API Compatibility

### Public API Unchanged

Application code continues to work without changes:

```cpp
// All existing code works unchanged
REL::ID simpleId{12345};
auto addr = simpleId.address();

// New multi-runtime support
REL::RelocationID multiId{12345, 67890, 11111};
auto addr2 = multiId.address();  // Automatically selects correct ID

// F4-specific runtime detection
bool isNG = REL::F4::IsNG();
bool isVR = REL::F4::IsVR();
auto runtime = REL::F4::GetRuntime();
```

### Shared Library Integration

CommonLibF4 can use either local submodule or remote package:

```lua
-- Check for local development setup
local use_local_commonlib = os.isdir("lib/commonlib-shared")

if use_local_commonlib then
    -- Local submodule development
    add_includedirs("lib/commonlib-shared/include", {public = true})
    add_files("lib/commonlib-shared/src/**.cpp")
else
    -- Remote package for production
    add_requires("commonlib-shared", {configs = {...}})
end
```

## Performance Characteristics

### Compile-Time Optimization

Single-runtime builds optimize to constants:

```cpp
// When REL_CURRENT_RUNTIME=0 (Pre-NG only build):
std::size_t get_runtime_index() noexcept {
    return 0;  // Compile-time constant
}

// RelocationID.id() becomes:
constexpr std::uint64_t id() const noexcept {
    return m_ids[0];  // Direct array access, zero overhead
}
```

### Runtime Efficiency

Multi-runtime builds cache detection:

```cpp
// Detection happens once, then cached
static std::size_t cached_index = []() -> std::size_t {
    // Complex detection logic runs once
    return detect_runtime_and_map_to_index();
}();

return cached_index;  // O(1) lookup thereafter
```

## Key Design Patterns

### What Makes CommonLibF4 Complex

1. **Sophisticated Detection**: Filesystem + version + build-time fallbacks
2. **Multiple Build Modes**: 1-3 runtime configurations with different index mappings
3. **Historical Compatibility**: VR shares Pre-NG IDs for most functions
4. **Flexible Fallbacks**: Custom fallback logic for missing VR IDs
5. **Build System Integration**: Dynamic runtime count calculation

### Lessons for Other Games

1. **Start Simple**: Most games only need single runtime detection
2. **Build Complexity Gradually**: Add multi-runtime support when needed
3. **Cache Detection**: Expensive detection should be cached
4. **Validate Configuration**: Use static_assert for build-time validation
5. **Test Thoroughly**: Complex configurations need extensive testing

## Using CommonLibF4 as Reference

When implementing your own game library:

1. **Copy the minimal pattern**: Start with single function implementation
2. **Adapt detection logic**: Most games won't need filesystem detection
3. **Simplify build system**: Start with single runtime, add complexity as needed
4. **Consider fallback strategies**: Plan how missing IDs should be handled
5. **Validate extensively**: Test all supported runtime combinations

## Complete Minimal Example

For comparison, here's a simple game implementation:

```cpp
// src/REL/Module.cpp - Minimal implementation
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return YourGame::get_runtime_index();
    }
}

// src/REL/Runtime.cpp - Simple version detection
namespace REL::YourGame {
    std::size_t get_runtime_index() noexcept {
        const auto module = detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        
        // Simple version threshold
        return (version >= Version{2, 0, 0}) ? 1 : 0;
    }
}
```

CommonLibF4 demonstrates that the minimal interface pattern can handle extremely complex multi-runtime scenarios while maintaining clean architecture and optimal performance! 🎯