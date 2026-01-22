#include "REL/GAME/Runtime.h"
#include "REL/Module.h"

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
		// const auto module = detail::ModuleBase::GetSingleton();
		// const auto version = module->version();
		// return detect_runtime_from_version(version);

		// Option 3: Custom detection logic
		// Implement your own detection based on:
		// - Module version
		// - File signatures  
		// - Registry entries
		// - Command line arguments
		// - Environment variables
		// - etc.
		//
		// Example:
		// const auto module = detail::ModuleBase::GetSingleton();
		// const auto version = module->version();
		// 
		// if (version >= Version{1, 10, 984}) {
		//     return RuntimeIndex::Remaster;  // Next-Gen/Anniversary Edition
		// }
		// return RuntimeIndex::Original;  // Classic/Legacy Edition
	}
}