#pragma once

// GAME-specific Module singleton implementation
// This header provides a GAME-specific Module singleton that delegates to the shared library Module
// Replace "GAME" with your game name (e.g., F4, SSE, SF)

#include <REL/Module.h>  // Include shared Module
#include "REX/REX/Singleton.h"

namespace REL
{
	namespace GAME
	{
		// GAME-specific Module singleton that delegates to the shared library Module
		class Module : public REX::Singleton<Module>
		{
		public:
			Module() = default;

			[[nodiscard]] constexpr std::uintptr_t base() const noexcept 
			{ 
				return REL::detail::ModuleBase::GetSingleton()->base(); 
			}
			
			[[nodiscard]] std::wstring_view filename() const noexcept 
			{ 
				return REL::detail::ModuleBase::GetSingleton()->filename(); 
			}
			
			[[nodiscard]] constexpr Segment segment(Segment::Name a_segment) const noexcept 
			{ 
				return REL::detail::ModuleBase::GetSingleton()->segment(a_segment); 
			}

			[[nodiscard]] constexpr Version version() const noexcept
			{
				return REL::detail::ModuleBase::GetSingleton()->version();
			}

			constexpr void version(Version a_version) noexcept
			{
				REL::detail::ModuleBase::GetSingleton()->version(a_version);
			}

			[[nodiscard]] void* pointer() const noexcept 
			{ 
				return REL::detail::ModuleBase::GetSingleton()->pointer(); 
			}

			template <class T>
			[[nodiscard]] T* pointer() const noexcept
			{
				return REL::detail::ModuleBase::GetSingleton()->template pointer<T>();
			}
		};
	}
}