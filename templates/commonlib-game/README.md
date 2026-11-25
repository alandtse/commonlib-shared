# CommonLib Game Template

This template provides the minimal implementation needed to create a new CommonLib game library using the shared library architecture.

## Overview

When `REL_DEFAULT_RUNTIME_COUNT > 1` (and `REL_CURRENT_RUNTIME` is not fixed at compile time), the game library implements **only one function**:

```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept;
}
```

Everything else (`IDDB`, `REX::FModule`, `RelocationID`, `VariantID`, `VariantOffset`, `THook`, `THookVFT`, `Trampoline`, `VerifyBytes`) is provided by `commonlib-shared`. When `REL_DEFAULT_RUNTIME_COUNT == 1`, `get_runtime_index()` is not ODR-used at all.

## Quick Start

1. **Copy this template** to your new game library directory
2. **Replace `GAME` placeholders** with your game name (e.g., `F4`, `SSE`, `SF`)
3. **Implement runtime detection** in `src/REL/Runtime.cpp`
4. **Configure build system** in `xmake.lua`

## Files Overview

- `include/REL/GAME/` - Game-specific wrapper headers (`Module.h`, `Runtime.h`)
- `src/REL/` - Minimal implementation (`Module.cpp`, `Runtime.cpp`)
- `xmake.lua` - Build configuration template

## Build Configuration

### Single Runtime (Most Common)
```lua
local RUNTIME_COUNT = 1
add_defines("REL_DEFAULT_RUNTIME_COUNT=" .. RUNTIME_COUNT, {public = true})
```

### Multi-Runtime Examples
```lua
-- Dual runtime (Original + Remaster)
local RUNTIME_COUNT = 2

-- Triple runtime (Original + Remaster + VR, e.g. slot 2 = VR RVA offset in VariantID)
local RUNTIME_COUNT = 3
-- Optional: customize VariantID direct-offset slot bitmask (defaults to 1ULL << 2)
-- add_defines("REL_VARIANT_OFFSET_MASK=(1ULL << 2)", {public = true})
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
#include "REX/FModule.h"

namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        const auto version = REX::FModule::GetExecutingModule().GetFileVersion();

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

### Multi-Runtime RelocationID, VariantID, and VariantOffset
```cpp
inline constexpr REL::RelocationID SomeFunction{12345, 67890};
auto addr = SomeFunction.address();  // Automatically selects ID for current runtime

// Direct RVA offset in slot 2 (default REL_VARIANT_OFFSET_MASK = 1ULL << 2):
inline constexpr REL::VariantID SomeMixedTarget{12345, 67890, 0x1A2B30};
inline constexpr REL::VariantOffset MemberOffset{0x100, 0x108, 0x110};
```

## Documentation

- **Architecture**: `docs/ARCHITECTURE.md`
- **Examples**: `docs/examples/CommonLibF4-Integration.md` and `docs/examples/MINIMAL_IMPLEMENTATION.md`
- **Migration Guide**: `docs/migration/MIGRATION.md`
