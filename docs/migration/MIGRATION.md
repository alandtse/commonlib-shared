# CommonLib-Shared Minimal Interface Migration Guide

## Breaking Changes in v2.1

CommonLib-Shared now uses a **minimal interface** instead of extensive function injection for better architectural separation with minimal implementation burden. This change enables:

- **True game-agnostic shared library**: IDDB, Module, and utilities stay in shared library
- **Multi-game compatibility**: Same shared library works for F4, Skyrim, Starfield, etc.
- **Minimal implementation**: Only **one function** needs to be implemented by games
- **Better performance**: Most code stays in shared library, only runtime selection varies

## Overview of Changes

### Before (v1.x - Broken Architecture)
```cpp
// Shared library directly accessing game-specific singletons
namespace REL {
    class IDDB : public REX::Singleton<IDDB> {
        std::uint64_t offset(std::uint64_t id) {
            const auto mod = Module::GetSingleton();  // ❌ Circular dependency
            return mod->base() + lookup(id);
        }
    };
}
```

### After (v2.1 - Minimal Interface)
```cpp
// Shared library keeps everything EXCEPT runtime selection
namespace REL::detail {
    [[nodiscard]] std::size_t get_runtime_index() noexcept;  // ONLY function games implement
    
    class ModuleBase : public REX::Singleton<ModuleBase> {
        // Full implementation in shared library ✅
    };
    
    class IDDB : public REX::Singleton<IDDB> {
        std::uint64_t offset(std::uint64_t id) {
            // Full implementation in shared library ✅
            return m_database[id];  // No function calls needed
        }
    };
}

// Game-specific library provides ONLY runtime detection
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return YourGame::get_runtime_index();  // Only function you implement!
    }
}
```

## Required Changes for Downstream Libraries

### 1. Implement Single Function

Your game-specific library must implement **only one function** in `src/REL/Module.cpp`:

```cpp
namespace REL::detail {
    // THE ONLY FUNCTION YOU NEED TO IMPLEMENT
    std::size_t get_runtime_index() noexcept {
        // Return 0 for single-runtime games
        // Return 0, 1, 2, etc. for multi-runtime games
        return YourGame::get_runtime_index();
    }
}
```

### 2. Update Build Configuration

#### XMake Configuration
```lua
target("your-commonlib")
    add_packages("commonlib-shared")
    add_files("src/**.cpp")
    add_includedirs("include", {public = true})
    
    -- CRITICAL: Define runtime count
    add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})  -- For single runtime
    -- add_defines("REL_DEFAULT_RUNTIME_COUNT=2", {public = true})  -- For dual runtime
    -- add_defines("REL_DEFAULT_RUNTIME_COUNT=3", {public = true})  -- For triple runtime
```

#### CMake Configuration  
```cmake
target_link_libraries(your-commonlib PUBLIC commonlib-shared)
target_compile_definitions(your-commonlib PUBLIC REL_DEFAULT_RUNTIME_COUNT=1)
```

### 3. Create Game-Specific Wrappers (Optional)

For API compatibility, create lightweight wrappers:

```cpp
// include/REL/YourGame/Module.h
namespace REL::YourGame {
    class Module : public REX::Singleton<Module> {
        [[nodiscard]] constexpr std::uintptr_t base() const noexcept {
            return REL::detail::ModuleBase::GetSingleton()->base();  // Delegate
        }
        // Add other methods as needed...
    };
}
```

## Step-by-Step Migration

### Step 1: Use Template
```bash
cp -r lib/commonlib-shared/templates/commonlib-game/* your-commonlib/
```

### Step 2: Replace Placeholders
```bash
# Replace "GAME" with your game name throughout all files
find . -type f -name "*.h" -o -name "*.cpp" -o -name "*.lua" | \
    xargs sed -i 's/GAME/YourGame/g'
```

### Step 3: Implement Runtime Detection

#### For Single Runtime Games (Most Common)
```cpp
// src/REL/Runtime.cpp
namespace REL::YourGame {
    std::size_t get_runtime_index() noexcept {
        return 0;  // Always 0 for single runtime
    }
}
```

#### For Multi-Runtime Games
```cpp
// src/REL/Runtime.cpp  
namespace REL::YourGame {
    std::size_t get_runtime_index() noexcept {
        const auto module = REL::detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        
        // Example: Fallout 4 Pre-NG vs NG
        if (version >= REL::Version{1, 10, 984}) {
            return 1;  // NG runtime
        }
        return 0;  // Pre-NG runtime
    }
}
```

### Step 4: Update Build System
Update your build files to:
- Link against `commonlib-shared`
- Define `REL_DEFAULT_RUNTIME_COUNT`
- Remove old source file inclusions

### Step 5: Test with Existing Code
```cpp
// This code should work unchanged after migration
REL::ID simple_id{12345};
auto addr = simple_id.address();

// Multi-runtime support now available
REL::RelocationID multi_id{12345, 67890};  // Pre-NG, NG
auto addr2 = multi_id.address();  // Automatically selects correct ID
```

## API Compatibility

### What Stays Exactly the Same
Public API remains 100% compatible:

```cpp
// All existing code continues to work
REL::ID myId{12345};
auto addr = myId.address();

REL::Relocation<std::uintptr_t> target{myId};
target.write<std::uint8_t>(0x90);  // NOP

auto& trampoline = REL::GetTrampoline();
trampoline.create(1024);
```

### What's New
Multi-runtime support with automatic selection:

```cpp
// New: Multi-runtime RelocationID
inline constexpr REL::RelocationID PlayerCharacter_ctor{
    1234567,  // Pre-NG ID
    7654321   // NG ID  
};

auto addr = PlayerCharacter_ctor.address();  // Gets correct address automatically
```

### What Changes (Implementation Only)
- `REL::detail::ModuleBase` instead of game-specific `Module` in shared library
- `REL::detail::IDDB` stays in shared library (no more game-specific IDDB)
- Only `get_runtime_index()` function needs implementation

## Migration Examples

### Example 1: CommonLibF4 Style
```cpp
// Before (v1.x): Complex game-specific IDDB
namespace REL::F4 {
    class IDDatabase : public REX::Singleton<IDDatabase> {
        // Lots of F4-specific code...
    };
}

// After (v2.1): Minimal implementation
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return F4::get_runtime_index();  // Just runtime detection
    }
}
// Everything else handled by shared library automatically!
```

### Example 2: Single Runtime Game
```cpp
// Complete implementation for single-runtime game:

// src/REL/Module.cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return 0;  // That's it!
    }
}

// xmake.lua
target("commonlib-yourgame")
    add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})
```

### Example 3: Complex Multi-Runtime
```cpp
// For games with complex runtime detection:
namespace REL::YourGame {
    std::size_t get_runtime_index() noexcept {
        const auto module = detail::ModuleBase::GetSingleton();
        const auto version = module->version();
        const auto filename = module->filename();
        
        // Complex detection logic
        if (filename.find(L"GameVR.exe") != std::wstring::npos) {
            return 2;  // VR
        }
        if (version >= Version{2, 0, 0}) {
            return 1;  // Remaster
        }
        return 0;  // Original
    }
}
```

## Troubleshooting

### Common Issues

#### Build Error: "REL_DEFAULT_RUNTIME_COUNT not defined"
```
error: "REL_DEFAULT_RUNTIME_COUNT must be defined by the build system"
```
**Solution**: Add to your build configuration:
```lua
add_defines("REL_DEFAULT_RUNTIME_COUNT=1", {public = true})
```

#### Linker Error: "get_runtime_index not found"
```
error: unresolved external symbol "get_runtime_index"
```
**Solution**: Implement the function in your `src/REL/Module.cpp`:
```cpp
namespace REL::detail {
    std::size_t get_runtime_index() noexcept {
        return 0;  // Your implementation
    }
}
```

#### Compilation Error: "ID not found"
```
error: 'ID' is not a member of 'REL'
```
**Solution**: The shared library now provides type aliases. Make sure you're using the latest version with:
```cpp
#include "REL/ID.h"  // Provides REL::ID alias
```

#### Runtime Error: "No Address Library loaded"
**Solution**: Ensure your game's Address Library files are in the correct location and format.

### Performance Validation
Test that performance matches or exceeds previous version:
```cpp
// Benchmark ID resolution
auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 100000; ++i) {
    REL::ID test{12345};
    volatile auto addr = test.address();  // Prevent optimization
}
auto end = std::chrono::high_resolution_clock::now();
// Should be same or faster than before
```

## Benefits After Migration

### For Single Runtime Games
- **Minimal work**: Implement one function that returns 0
- **Better performance**: Everything in shared library is optimized
- **Easier maintenance**: No game-specific IDDB or Module code to maintain

### For Multi-Runtime Games  
- **Automatic ID selection**: RelocationID chooses correct ID based on runtime
- **Smart fallbacks**: Missing IDs automatically fallback to primary
- **Compile-time optimization**: Single-runtime builds have zero overhead

### For All Games
- **Cleaner architecture**: Clear separation between shared and game code
- **Better testing**: Shared library can be tested independently
- **Multi-game support**: Same shared library binary works everywhere
- **Future-proof**: Easy to add new runtime support

## Migration Checklist

- [ ] Copy template files from `templates/commonlib-game/`
- [ ] Replace `GAME` placeholders with your game name
- [ ] Implement `get_runtime_index()` function
- [ ] Update build configuration to define `REL_DEFAULT_RUNTIME_COUNT`
- [ ] Remove old game-specific IDDB/Module implementations
- [ ] Create wrapper singletons for API compatibility (optional)
- [ ] Test with existing plugins
- [ ] Verify performance matches previous version
- [ ] Update documentation

## Need Help?

- **Templates**: Complete templates in `templates/commonlib-game/`
- **Examples**: Real implementations in `docs/examples/MINIMAL_IMPLEMENTATION.md`
- **Reference**: See CommonLibF4 for complete working example
- **Architecture**: Read `docs/ARCHITECTURE.md` for design details

The migration to minimal interface dramatically reduces implementation burden while providing more functionality and better performance!