#pragma once

// Core types first
#include "REL/ID.h"
#include "REL/Module.h"

namespace REL
{
	// Backward compatibility aliases for shared library internal use
	using ID = detail::ID;
	using Module = detail::ModuleBase;

	// Primary RelocationID alias - configured by downstream
	using RelocationID = detail::RelocationIDImpl<detail::DEFAULT_RUNTIME_COUNT>;
}

// Rest of includes that depend on the aliases
#include "REL/ASM.h"
#include "REL/Hook.h"
#include "REL/HookObject.h"
#include "REL/HookStore.h"
#include "REL/IAT.h"
#include "REL/IDDB.h"
#include "REL/Offset.h"
#include "REL/Offset2ID.h"
#include "REL/Pattern.h"
#include "REL/Relocation.h"
#include "REL/Segment.h"
#include "REL/Trampoline.h"
#include "REL/Utility.h"
#include "REL/Version.h"
