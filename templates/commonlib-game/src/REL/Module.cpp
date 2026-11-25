#include "REL/GAME/Module.h"
#include "REL/GAME/Runtime.h"

// This is the ONLY function that MUST be implemented by game libraries
// Everything else is provided by the shared library

namespace REL::detail
{
	std::size_t get_runtime_index() noexcept
	{
		// Delegate to game-specific runtime detection
		return GAME::get_runtime_index();
	}
}