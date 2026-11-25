# CommonLib-Shared

**A game-agnostic foundation library for Bethesda game modding with minimal interface design.**

## Overview

CommonLib-Shared provides a robust, game-agnostic foundation for creating game-specific CommonLib libraries (F4, Skyrim, Starfield, etc.) with **minimal implementation burden**. 

### Key Features

- **🎯 Minimal Interface**: Only **one function** needs implementation per game
- **🚀 Multi-Runtime Support**: Automatic ID selection for multiple game versions
- **⚡ Performance Optimized**: Zero overhead for single-runtime builds
- **🏗️ Clean Architecture**: True separation between shared and game-specific code
- **🔄 Backward Compatible**: Existing code continues to work unchanged
- **📦 Reusable**: Same shared library works across all supported games

## Quick Start

### For Game Library Authors

1. **Copy the template**:
```bash
cp -r templates/commonlib-game/* your-commonlib-yourgame/
```

2. **Replace placeholders**:
```bash
find . -name "*.h" -o -name "*.cpp" -o -name "*.lua" | xargs sed -i 's/GAME/YourGame/g'
```

3. **Implement one function** (`src/REL/Module.cpp`):
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return 0;  // For single runtime, or your detection logic
    }
}
```

4. **Configure build system**:
```lua
target("commonlib-yourgame")
    add_packages("commonlib-shared")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})
```

**That's it!** Your game library now has full RelocationID support with IDDB, Module, and utilities.

### For Plugin Developers

Multi-runtime support with automatic ID selection:

```cpp
// Traditional ID (still works)
REL::ID some_func{12345};
auto addr = some_func.address();

// Multi-runtime RelocationID (new)
inline constexpr REL::RelocationID PlayerCharacter_ctor{
    1234567,  // Pre-NG ID
    7654321   // NG ID
};
auto addr = PlayerCharacter_ctor.address();  // Automatically selects correct ID
```

## Architecture

### Minimal Interface Design

CommonLib-Shared uses a **minimal interface** approach:

- **Shared Library**: Contains IDDB, Module, RelocationID, and all utilities
- **Game Libraries**: Implement only `get_runtime_index()` for runtime detection
- **Applications**: Use consistent API regardless of underlying game

```
┌─────────────────────────────────────────┐
│           Applications                  │
│       (F4SE Plugins, etc.)             │
└─────────────────┬───────────────────────┘
                  │ Consistent API
                  ▼
┌─────────────────────────────────────────┐
│         Game Libraries                  │
│  ┌─────────┬─────────┬─────────────┐   │
│  │CommonLF4│CommonSSE│ CommonLibSF │   │
│  └─────────┴─────────┴─────────────┘   │
└─────────────────┬───────────────────────┘
                  │ Single Function
                  ▼
┌─────────────────────────────────────────┐
│        CommonLib-Shared                 │
│  • IDDB (all formats: V0,V2,V5,CSV)    │
│  • Module (PE parsing, version detect) │
│  • RelocationID (multi-runtime)        │
│  • Utilities (patterns, hooks, etc.)   │
└─────────────────────────────────────────┘
```

### Multi-Runtime Support

Automatic ID and offset selection based on runtime detection:

```cpp
// Example: Fallout 4 Pre-NG vs NG
inline constexpr REL::RelocationID PlayerCharacter_Update{
    12345,  // Pre-NG (1.10.163)
    67890   // NG (1.10.984)
};

// System automatically:
// 1. Calls REL::detail::get_runtime_index() when N > 1
// 2. Selects appropriate ID from array
// 3. Resolves address via shared IDDB
// 4. Returns correct address for current game version

// VariantID supports mixing IDDB IDs and direct RVA offsets via REL_VARIANT_OFFSET_MASK (default: bit 2 = VR)
inline constexpr REL::VariantID MixedTarget{
    12345,     // Slot 0: IDDB ID
    67890,     // Slot 1: IDDB ID
    0x123450   // Slot 2: direct RVA offset
};

// VariantOffset stores per-runtime raw offsets
inline constexpr REL::VariantOffset StructFieldOffset{ 0x10, 0x18, 0x20 };

// Multi-runtime hooks via THook / THookVFT
inline REL::THook HookTarget{ PlayerCharacter_Update, 0x15, MyHook };
```

### Explicit Zero & 2-Arg VR Behavior

- An explicit `0` in any runtime slot stays `0` (`address() == 0`), so plugins can safely check availability with `if (target.id() != 0)` instead of silently jumping to the wrong runtime's ID.
- Passing 2 arguments `(primary, secondary)` to `RelocationID` populates slot 2 (VR) with `primary` (traditional SE/AE/VR behavior where VR shares SE IDs when not overridden); passing 2 arguments to `VariantID` leaves masked offset slots (such as slot 2) as `0`.

## Implementation Examples

### Single Runtime Game
```cpp
// Complete implementation for games with one version (also default when REL_DEFAULT_RUNTIME_COUNT=1)
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return 0;  // Always return 0
    }
}
```

### Multi-Runtime Game
```cpp
#include "REX/FModule.h"

// Example: Fallout 4 Pre-NG vs NG detection
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        const auto version = REX::FModule::GetExecutingModule().GetFileVersion();

        if (version >= REL::Version{1, 10, 984}) {
            return 1;  // NG runtime
        }
        return 0;  // Pre-NG runtime
    }
}
```

## Performance

### Compile-Time Optimization
Single-runtime builds have **zero overhead**:

```cpp
// When REL_DEFAULT_RUNTIME_COUNT=1, this:
RelocationID func{12345, 67890};
auto id = func.id();

// Compiles to:
auto id = 12345;  // Direct access, no runtime overhead
```

### Runtime Efficiency
- **IDDB Operations**: Optimized binary search in shared library
- **Memory Mapping**: Efficient file-based databases  
- **ID Resolution**: Minimal function call overhead
- **Code Reuse**: Shared library binary works across all games

## Supported Features

### IDDB Formats
- **V0**: Legacy format support
- **V2**: Compressed binary format
- **V5**: Modern binary format
- **CSV**: Text Address Library format (`id,offset` with hex offsets, metadata count & version, `csv_version()`, `is_csv_at_least_version()`)

### Address Resolution & Hooking
- **REL::ID**: Traditional single-runtime IDs
- **REL::RelocationID**: Multi-runtime ID selection (`REL_DEFAULT_RUNTIME_COUNT`, `REL_CURRENT_RUNTIME`)
- **REL::VariantID**: Mixed per-runtime IDDB ID / direct RVA offset (`REL_VARIANT_OFFSET_MASK`)
- **REL::VariantOffset**: Per-runtime raw offset selection
- **REL::Relocation**: Address manipulation and patching (`RelocationID`, `VariantID`, `VariantOffset`, `ID`, `Offset`)
- **REL::Offset**: Raw offset-based addressing
- **REL::THook / REL::THookVFT**: Multi-runtime inline and VFT hooks with CTAD guides

### Utilities & Safety Options
- **Pattern Matching**: Binary pattern search
- **Trampolines & Patch Safety**: `REL::Trampoline` (`write_call`, `write_jmp`, `write_branch`) with optional `hde64` instruction-boundary & hash verification (`COMMONLIB_PATCH_SAFETY` / `commonlib_patch_safety` -> `COMMONLIB_OPTION_PATCH_SAFETY=1`, `REL::VerifyBytes`, `REL::HashBytes`)
- **Version Management**: Game version detection, `REL::ParseVersionString`, `VersionBase(std::string_view)`, and configurable separator macros `REL_VERSION_SEPARATOR` (default `"."`) / `REL_VERSION_WSEPARATOR` (default `L"."`)
- **Module Analysis**: `REX::FModule` and `REX::FModuleSection` PE parsing and section mapping

## Directory Structure

```
commonlib-shared/
├── docs/                           # Documentation
│   ├── ARCHITECTURE.md            # Design overview
│   ├── examples/                  # Usage examples
│   └── migration/                 # Migration guides
├── include/REL/                   # Public headers
│   ├── ID.h                      # REL::ID & RelocationID system
│   ├── IDDB.h                    # Address database (V0/V2/V5/CSV)
│   ├── THook.h                   # Multi-runtime THook / THookVFT
│   ├── Trampoline.h              # Branch/call trampolines
│   ├── VariantID.h               # VariantID (mixed ID/offset)
│   ├── VariantOffset.h           # VariantOffset
│   ├── Verify.h                  # Patch-site FNV-1a verification
│   ├── Version.h                 # Configurable version parsing/formatting
│   └── *.h                       # Other utilities
├── include/REX/                   # Foundation utilities (FModule, LOG, INI/JSON/TOML, etc.)
├── src/REL/                      # Implementation
├── templates/commonlib-game/     # Game library template
└── README.md                     # This file
```

## Getting Started

### For Game Library Authors
1. **Read**: [Architecture Guide](docs/ARCHITECTURE.md)
2. **Copy**: [Game Template](templates/commonlib-game/)
3. **Implement**: [Minimal Interface](docs/examples/MINIMAL_IMPLEMENTATION.md)
4. **Migrate**: [Migration Guide](docs/migration/MIGRATION.md)

### For Plugin Developers
1. **Use**: Existing CommonLib for your game (CommonLibF4, CommonLibSSE, etc.)
2. **Code**: Standard RelocationID patterns
3. **Build**: Multi-runtime support automatically handled

### For Contributors
1. **Shared Library**: Enhance game-agnostic functionality
2. **Templates**: Improve game library templates
3. **Documentation**: Add examples and guides
4. **Testing**: Validate across multiple games

## Supported Games

Game libraries using CommonLib-Shared:
- **Fallout 4**: [CommonLibF4](https://github.com/CommonLibF4/CommonLibF4) - Reference implementation
- **Skyrim**: CommonLibSSE (migration in progress)
- **Starfield**: CommonLibSF (planned)
- **Other**: Any Bethesda game with Address Library support

## Contributing

Contributions welcome! Areas of focus:
- **Performance optimization** in shared library components
- **Additional IDDB formats** for broader game support  
- **Enhanced templates** for easier game library creation
- **Documentation** and examples
- **Testing** across different games and scenarios

## License

MIT License - see [LICENSE](LICENSE) for details.

## Support

- **Documentation**: Complete guides in [`docs/`](docs/)
- **Templates**: Ready-to-use templates in [`templates/`](templates/)
- **Examples**: Real implementations in [`docs/examples/`](docs/examples/)
- **Issues**: Report problems via GitHub Issues
- **Discussions**: Architecture questions welcome

---

**CommonLib-Shared: One function to implement them all!** 🎯