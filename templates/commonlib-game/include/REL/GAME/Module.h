#pragma once

// GAME-specific Module singleton wrapper around REX::FModule.
// Replace "GAME" with your game name (e.g., F4, SSE, SF).

#include "REX/FModule.h"
#include "REX/FModuleSection.h"
#include "REX/TSingleton.h"

namespace REL::GAME
{
	class Module : public REX::TSingleton<Module>
	{
	public:
		Module() = default;

		[[nodiscard]] std::uintptr_t base() const noexcept
		{
			return REX::FModule::GetExecutingModule().GetBaseAddress();
		}

		[[nodiscard]] std::string filename() const noexcept
		{
			return REX::FModule::GetExecutingModule().GetFileName();
		}

		[[nodiscard]] REX::FModuleSection segment(std::string_view a_section) const
		{
			return REX::FModule::GetExecutingModule().GetSection(a_section);
		}

		[[nodiscard]] Version version() const noexcept
		{
			return REX::FModule::GetExecutingModule().GetFileVersion();
		}

		[[nodiscard]] void* pointer() const noexcept
		{
			return reinterpret_cast<void*>(base());
		}

		template <class T>
		[[nodiscard]] T* pointer() const noexcept
		{
			return reinterpret_cast<T*>(base());
		}
	};
}
