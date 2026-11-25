#pragma once

// GAME-specific runtime detection and configuration
// Replace "GAME" with your game name (e.g., F4, SSE, SF)

#include "REX/BASE.h"
#include "REL/Version.h"

namespace REL
{
	namespace GAME
	{
		// Runtime indices for multi-runtime games
		// Customize these for your specific game versions
		enum RuntimeIndex : std::size_t
		{
			// Example for single runtime game:
			Primary = 0,
			
			// Example for dual runtime game (uncomment and customize):
			// Original = 0,
			// Remaster = 1,
			
			// Example for triple runtime game (uncomment and customize):
			// Original = 0,
			// Remaster = 1,
			// VR = 2,
		};

		// Get current runtime index
		// This function implements the game-specific runtime detection logic
		[[nodiscard]] std::size_t get_runtime_index() noexcept;

		// Optional: Version-based runtime detection helper
		// Customize this for your game's version scheme
		[[nodiscard]] inline std::size_t detect_runtime_from_version(const Version& version) noexcept
		{
			// Example implementation for single runtime:
			return RuntimeIndex::Primary;

			// Example implementation for dual runtime:
			// if (version >= Version{1, 6, 640}) {  // Example version threshold
			//     return RuntimeIndex::Remaster;
			// }
			// return RuntimeIndex::Original;

			// Example implementation for triple runtime:
			// if (version >= Version{1, 4, 15}) {  // VR version threshold
			//     return RuntimeIndex::VR;
			// }
			// if (version >= Version{1, 6, 640}) {  // Remaster version threshold
			//     return RuntimeIndex::Remaster;
			// }
			// return RuntimeIndex::Original;
		}

		// Optional: Runtime name for debugging/logging
		[[nodiscard]] inline std::string_view get_runtime_name(std::size_t runtime_index) noexcept
		{
			switch (runtime_index) {
				case RuntimeIndex::Primary:
					return "Primary";
				// Uncomment and customize for multi-runtime:
				// case RuntimeIndex::Original:
				//     return "Original";
				// case RuntimeIndex::Remaster:
				//     return "Remaster";
				// case RuntimeIndex::VR:
				//     return "VR";
				default:
					return "Unknown";
			}
		}

		// Optional: Get expected runtime count (for validation)
		[[nodiscard]] constexpr std::size_t get_runtime_count() noexcept
		{
#ifdef REL_DEFAULT_RUNTIME_COUNT
			return REL_DEFAULT_RUNTIME_COUNT;
#else
			return 1;  // Default to single runtime
#endif
		}
	}
}