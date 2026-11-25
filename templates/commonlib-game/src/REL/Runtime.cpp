#include "REL/GAME/Runtime.h"

#include "REX/FModule.h"

namespace REL::GAME
{
	std::size_t get_runtime_index() noexcept
	{
		// Option 1: Single runtime (most common)
		// Just return 0 for games with only one runtime version
		return RuntimeIndex::Primary;

		// Option 2: Version-based detection
		// Uncomment and customize for multi-runtime games:
		//
		// const auto mod = REX::FModule::GetExecutingModule();
		// const auto version = mod.GetFileVersion();
		// return detect_runtime_from_version(version);

		// Option 3: Custom detection logic
		// Implement your own detection based on:
		// - Module version (mod.GetFileVersion())
		// - Module filename (mod.GetFileName())
		// - File signatures, registry entries, command line, or environment variables
		//
		// Example:
		// const auto version = REX::FModule::GetExecutingModule().GetFileVersion();
		// if (version >= Version{ 1, 10, 984 }) {
		//     return RuntimeIndex::Remaster;
		// }
		// return RuntimeIndex::Original;
	}
}
