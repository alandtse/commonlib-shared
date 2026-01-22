# CommonLib Game Template

This template provides the minimal implementation needed to create a new CommonLib game library using the shared library architecture.

## Overview

The minimal interface requires implementing **only one function** in your game library:

```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept;
}
```

Everything else (IDDB, Module, RelocationID, etc.) is provided by the shared library.

## Quick Start

1. **Copy this template** to your new game library directory
2. **Replace `GAME` placeholders** with your game name (e.g., F4, SSE, SF)
3. **Implement runtime detection** in `src/REL/Module.cpp`
4. **Configure build system** in `xmake.lua`

## Files Overview

- `include/REL/GAME/` - Game-specific wrapper headers
- `src/REL/` - Minimal implementation (single function)
- `xmake.lua` - Build configuration template

## Build Configuration

The template supports both single-runtime and multi-runtime configurations:

### Single Runtime (Most Common)
```lua
-- Most games only need single runtime support
local runtime_count = 1
target:add("defines", "REL_DEFAULT_RUNTIME_COUNT=" .. runtime_count, {public = true})
```

### Multi-Runtime Examples
```lua
-- Dual runtime (Original + Remaster)
local runtime_count = 2

-- Triple runtime (Original + Remaster + VR)  
local runtime_count = 3
```

## Implementation Examples

### Single Runtime Game
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return 0;  // Always return 0 for single runtime
    }
}
```

### Multi-Runtime Game
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        const auto version = ModuleBase::GetSingleton()->version();
        
        // Example: Fallout 4 Pre-NG vs NG detection
        if (version >= REL::Version{1, 10, 984}) {
            return 1;  // Next-Gen runtime
        }
        return 0;  // Pre-NG runtime
    }
}
```

## Usage in Your Code

### Traditional ID (works as before)
```cpp
REL::ID SomeFunction{12345};
auto addr = SomeFunction.address();
```

### Multi-Runtime RelocationID
```cpp
// Pre-NG ID: 12345, NG ID: 67890
inline constexpr REL::RelocationID SomeFunction{12345, 67890};
auto addr = SomeFunction.address();  // Automatically selects correct ID
```

## Migration from Existing Libraries

If you have an existing CommonLib game library:

1. **Add the single function implementation**
2. **Update your build system** to define `REL_DEFAULT_RUNTIME_COUNT`
3. **Replace direct singleton calls** with wrapper pattern (optional)
4. **Test thoroughly** with existing plugin code

## Support

- **Documentation**: `/docs/ARCHITECTURE.md` for detailed architecture info
- **Examples**: See CommonLibF4 implementation for real-world usage
- **Issues**: Report problems in the shared library repository