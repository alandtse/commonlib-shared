# CommonLib-Shared Architecture

## Design Philosophy

CommonLib-Shared implements a **minimal interface architecture** to achieve true separation between game-agnostic shared code and game-specific implementations. This design enables:

1. **Single shared library** serving multiple games (F4, Skyrim, Starfield, etc.)
2. **Clean architectural boundaries** with no circular dependencies
3. **Minimal implementation burden** for downstream games
4. **Maintainable codebase** where changes to shared code don't break game-specific implementations

## Architectural Overview

### High-Level Structure

```
┌─────────────────────────────────────────────────────────────┐
│                     Applications                            │
│                  (F4SE Plugins, etc.)                      │
└─────────────────────┬───────────────────────────────────────┘
                      │ Consistent API
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                 Game Libraries                              │
│        ┌─────────────┬─────────────┬─────────────┐          │
│        │ CommonLibF4 │ CommonLibSSE│ CommonLibSF │          │
│        │             │             │             │          │
│        │ F4-specific │ SSE-specific│ SF-specific │          │
│        │ wrappers    │ wrappers    │ wrappers    │          │
│        └─────────────┴─────────────┴─────────────┘          │
└─────────────────────┬───────────────────────────────────────┘
                      │ Single Function Interface
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                CommonLib-Shared                             │
│                                                             │
│  ┌─────────────────┐    ┌─────────────────┐                │
│  │ Game-Agnostic   │    │ Minimal         │                │
│  │ Implementation  │    │ Interface       │                │
│  │                 │    │                 │                │
│  │ • IDDB          │    │ • get_runtime_  │                │
│  │ • Module        │    │   index()       │                │
│  │ • RelocationID  │    │                 │                │
│  │ • Templates     │    │ (only function  │                │
│  │ • Utilities     │    │  required)      │                │
│  └─────────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────────┘
```

### Dependency Flow

**Clean Dependency Direction** (v2.1):
```
Applications
    ↓ (uses)
Game Libraries
    ↓ (implements single function interface)
Shared Library
```

**Previous Broken Dependencies** (v1.x):
```
Applications
    ↓ (uses)
Game Libraries ←──────┐ (circular!)
    ↓ (uses)          │
Shared Library ───────┘ (depends on game singletons)
```

## Core Design Pattern: Minimal Interface

### The Single Function Interface

The shared library declares **only one function** that game libraries must implement:

**Shared Library (declares)**:
```cpp
namespace REL::detail {
    // Single forward declaration - implemented by downstream
    [[nodiscard]] std::size_t get_runtime_index() noexcept;
}
```

**Game Library (implements)**:
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        // Game-specific runtime detection logic
        return F4::get_runtime_index();
    }
}
```

### Why Minimal Interface?

Our analysis showed that the IDDB format is **identical across games**:
- Memory maps are the same
- File parsing logic is identical  
- Only runtime selection varies by game

Therefore, we keep **everything** in the shared library except the one function needed for multi-runtime selection.

## Template-Based Multi-Runtime Support

The shared library provides flexible templates that games can configure:

```cpp
namespace REL::detail {
    // Configurable template - games specify runtime count via build system
    template <std::size_t N = 1>
    class RelocationIDImpl {
    public:
        template <typename... Args>
        constexpr RelocationIDImpl(Args... args) noexcept
            requires(sizeof...(Args) >= 1 && (N == 1 || sizeof...(Args) == N))
        {
            // Smart parameter handling for single vs multi-runtime builds
        }

        [[nodiscard]] std::uint64_t id() const noexcept {
            if constexpr (N == 1) {
                return m_ids[0];  // Compile-time optimization
            } else {
                const auto index = get_runtime_index();
                return resolve_id(index);  // Runtime selection
            }
        }

        // Smart fallback logic for missing IDs
        [[nodiscard]] std::uint64_t resolve_id(std::size_t runtime_index) const noexcept;
        
    private:
        std::array<std::uint64_t, N> m_ids{};
    };
    
    // Build system configures via REL_DEFAULT_RUNTIME_COUNT
    using RelocationID = RelocationIDImpl<DEFAULT_RUNTIME_COUNT>;
}
```

### Build System Integration

Games configure runtime support via XMake:

```lua
-- xmake.lua configuration
target("game")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=" .. runtime_count, {public = true})
    
-- For single runtime (most common):
-- REL_DEFAULT_RUNTIME_COUNT=1

-- For dual runtime (F4 Pre-NG + NG):
-- REL_DEFAULT_RUNTIME_COUNT=2

-- For triple runtime (F4 Pre-NG + NG + VR):
-- REL_DEFAULT_RUNTIME_COUNT=3
```

## Namespace Separation

Clear namespace organization maintains boundaries:

```cpp
REL::                    // Public API (stable, game-agnostic)
REL::detail::            // Shared library internals + minimal interface
REL::F4::                // F4-specific extensions (in CommonLibF4)
REL::SSE::               // SSE-specific extensions (in CommonLibSSE)
REL::SF::                // SF-specific extensions (in CommonLibSF)
```

## Key Components

### Shared Library Core

#### RelocationID System
- **Template-based multi-runtime support**: `RelocationIDImpl<N>`
- **Smart fallback logic**: Automatic fallback when IDs are missing
- **Compile-time optimization**: Single-runtime mode with zero overhead  
- **Runtime polymorphism**: Multi-runtime detection via `get_runtime_index()`

#### IDDB Management
- **Game-agnostic implementation**: All parsing and memory mapping in shared library
- **Multiple format support**: V0, V2, V5, and CSV formats
- **Identical memory maps**: Same structure across all games
- **Performance optimized**: Binary search, memory mapped files

#### Module System
- **Singleton pattern**: `detail::ModuleBase::GetSingleton()` in shared library
- **PE parsing**: Complete Windows module analysis
- **Version detection**: Automatic game version identification
- **Segment analysis**: Text, data, rdata section mapping

#### Utility Systems
- **Pattern matching**: Game-agnostic binary pattern search
- **Trampolines**: Runtime code generation utilities
- **Hooking**: Function interception framework
- **Relocation**: Address resolution and patching

### Game Library Responsibilities

Each game library must:

1. **Implement single function**: Provide `get_runtime_index()` implementation
2. **Provide wrapper singletons**: Game-specific API compatibility layer
3. **Configure build system**: Set runtime count via preprocessor definitions
4. **Game-specific extensions**: Additional APIs beyond shared library

### Minimal Implementation Example

**Complete F4 Implementation**:
```cpp
// src/REL/Module.cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return F4::get_runtime_index();  // Delegate to game-specific logic
    }
}

// include/REL/F4/Module.h  
namespace REL::F4 {
    class Module : public REX::Singleton<Module> {
        // F4-specific wrapper that delegates to shared library
        [[nodiscard]] constexpr std::uintptr_t base() const noexcept {
            return REL::detail::ModuleBase::GetSingleton()->base();
        }
    };
}
```

## Design Benefits

### 1. Minimal Implementation Burden

- **Single function**: Only `get_runtime_index()` must be implemented
- **No complex patterns**: No dependency injection framework needed
- **Copy-paste ready**: Templates make implementation trivial

### 2. True Multi-Game Support

Same shared library binary works across different games:
```cpp
// Same shared library code
RelocationID func{12345, 67890};  // Works in F4, SSE, SF, etc.
auto addr = func.address();       // Shared library handles everything
```

### 3. Performance Optimized

- **Compile-time optimization**: Single-runtime builds have zero overhead
- **Runtime efficiency**: Minimal indirection, direct function calls
- **Memory efficiency**: Shared library code reused across games

### 4. Backward Compatibility

- **Existing code works**: `REL::ID` unchanged
- **Gradual migration**: Can adopt `RelocationID` incrementally  
- **API stability**: Public interface remains consistent

### 5. Easy Maintenance

- **Minimal interface**: Less code to maintain in game libraries
- **Centralized logic**: IDDB, Module, patterns all in shared library
- **Clear boundaries**: Only runtime detection varies by game

## Implementation Guide

### For New Games

1. **Implement single function**:
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        // Return 0 for single runtime, or game-specific detection logic
        return 0;
    }
}
```

2. **Configure build system**:
```lua
target("commonlib-yourgame")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})
```

3. **Create wrapper singletons** (optional, for API compatibility):
```cpp
namespace REL::YourGame {
    class Module : public REX::Singleton<Module> {
        [[nodiscard]] constexpr std::uintptr_t base() const noexcept {
            return REL::detail::ModuleBase::GetSingleton()->base();
        }
    };
}
```

### For Multi-Runtime Games

1. **Implement runtime detection**:
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        const auto version = ModuleBase::GetSingleton()->version();
        
        if (version >= REL::Version{1, 10, 984}) {
            return 1;  // Next-Gen
        }
        return 0;  // Pre-NG
    }
}
```

2. **Configure build system**:
```lua
target("commonlib-f4")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=2", {public = true})
```

3. **Use RelocationID in code**:
```cpp
// Pre-NG ID: 12345, NG ID: 67890
inline constexpr REL::RelocationID SomeFunction{12345, 67890};
auto addr = SomeFunction.address();  // Automatically selects based on runtime
```

## Advanced Features

### Smart Fallback Logic

RelocationID includes intelligent fallback strategies:

```cpp
// Example: VR might reuse Pre-NG IDs
RelocationID func{12345, 67890, 0};  // Pre-NG, NG, VR(fallback to Pre-NG)

// Automatic fallback resolution:
// - If runtime 2 (VR) requested but ID is 0, fallback to runtime 0 (Pre-NG)
// - If runtime is out of bounds, use primary ID
// - If primary is 0, find first non-zero ID
```

### Compile-Time Optimization

Single-runtime builds are fully optimized:

```cpp
// When REL_DEFAULT_RUNTIME_COUNT=1, this becomes:
constexpr std::uint64_t id() const noexcept {
    return m_ids[0];  // Direct access, no runtime overhead
}
```

### CSV Format Support

For development and modding tools:

```csv
id,offset
12345,0x1A2B3C
67890,0x4D5E6F
```

The shared library automatically detects and loads CSV format when binary files are unavailable.

## Migration from v1.x

### Breaking Changes

1. **Game libraries must implement `get_runtime_index()`**
2. **Some singleton dependencies removed from shared library**
3. **Build system must define `REL_DEFAULT_RUNTIME_COUNT`**

### Migration Steps

1. **Update build configuration** to define runtime count
2. **Implement single function** in game library  
3. **Update includes** if using internal `detail::` APIs
4. **Test thoroughly** with existing plugin code

### Migration Tool

The shared library provides templates and examples in `/templates/commonlib-game/` to accelerate migration.

## Future Extensibility

The minimal interface pattern enables future enhancements without breaking changes:

### Potential Extensions
- **Additional runtime detection strategies**
- **Custom fallback logic per game**
- **Enhanced CSV format support**
- **Performance profiling integration**

### Maintained Compatibility
- **Core interface remains minimal**
- **Shared library evolves independently** 
- **Games can extend without affecting others**

This architecture provides a robust, performant, and maintainable foundation for the entire CommonLib ecosystem while minimizing the implementation burden on game-specific libraries! 🏗️