#pragma once

#include "REX/BASE.h"
#include "REX/FModule.h"

#include "REL/IDDB.h"

#ifndef REL_DEFAULT_RUNTIME_COUNT
#	define REL_DEFAULT_RUNTIME_COUNT 1
#endif

namespace REL
{
	class ID
	{
	public:
		constexpr ID() noexcept = default;

		explicit constexpr ID(std::uint64_t a_id) noexcept :
			m_id(a_id)
		{}

		constexpr ID& operator=(std::uint64_t a_id) noexcept
		{
			m_id = a_id;
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const
		{
			const auto mod = REX::FModule::GetExecutingModule();
			return mod.GetBaseAddress() + offset();
		}

		[[nodiscard]] constexpr std::uint64_t id() const noexcept
		{
			return m_id;
		}

		[[nodiscard]] std::size_t offset() const
		{
			const auto iddb = IDDB::GetSingleton();
			return iddb->offset(m_id);
		}

	private:
		std::uint64_t m_id{ 0 };
	};

	namespace detail
	{
		inline constexpr std::size_t DEFAULT_RUNTIME_COUNT = REL_DEFAULT_RUNTIME_COUNT;

		// Downstream multi-runtime libraries (REL_DEFAULT_RUNTIME_COUNT > 1 without
		// REL_CURRENT_RUNTIME) provide the definition returning the active runtime slot.
		[[nodiscard]] std::size_t get_runtime_index() noexcept;

		template <std::size_t N>
		[[nodiscard]] inline std::size_t get_current_runtime_index() noexcept
		{
#ifdef REL_CURRENT_RUNTIME
			return static_cast<std::size_t>(REL_CURRENT_RUNTIME);
#else
			if constexpr (N == 1) {
				return 0;
			} else {
				return get_runtime_index();
			}
#endif
		}

		template <class T>
		[[nodiscard]] T Relocate(T&& a_first, T&& a_second) noexcept
		{
			return get_runtime_index() == 0 ? std::forward<T>(a_first) : std::forward<T>(a_second);
		}

		template <class T>
		[[nodiscard]] T Relocate(T a_first, T a_second, T a_third) noexcept
		{
			switch (get_runtime_index()) {
				case 1:
					return a_second;
				case 2:
					return a_third;
				default:
					return a_first;
			}
		}

		template <std::size_t N = DEFAULT_RUNTIME_COUNT>
		class RelocationIDImpl
		{
		public:
			static_assert(N >= 1, "RelocationIDImpl requires at least 1 runtime");

			constexpr RelocationIDImpl() noexcept = default;

			explicit constexpr RelocationIDImpl(std::array<std::uint64_t, N> a_ids) noexcept :
				m_ids(a_ids)
			{}

			template <typename... Args>
			constexpr RelocationIDImpl(Args... a_args) noexcept
				requires(sizeof...(Args) >= 1 && (N == 1 || sizeof...(Args) == N))
			{
				const std::array<std::uint64_t, sizeof...(Args)> temp{ static_cast<std::uint64_t>(a_args)... };
				if constexpr (N == 1) {
					m_ids[0] = temp[0];
				} else {
					m_ids = temp;
				}
			}

			// 2-arg convenience constructor for 3/4-runtime builds where VR defaults to primary.
			// Explicit 0 in a runtime slot stays 0 (unmapped for that runtime).
			constexpr RelocationIDImpl(std::uint64_t a_primary, std::uint64_t a_secondary) noexcept
				requires(N == 3 || N == 4)
			{
				m_ids[0] = a_primary;
				m_ids[1] = a_secondary;
				m_ids[2] = a_primary;
				if constexpr (N == 4) {
					m_ids[3] = 0;
				}
			}

			[[nodiscard]] std::uintptr_t address() const
			{
				const auto off = offset();
				return off ? REX::FModule::GetExecutingModule().GetBaseAddress() + off : 0;
			}

			[[nodiscard]] std::uint64_t id() const noexcept
			{
				const auto idx = get_current_runtime_index<N>();
				return idx < N ? m_ids[idx] : 0;
			}

			[[nodiscard]] std::size_t offset() const
			{
				const auto resolved = id();
				return resolved ? IDDB::GetSingleton()->offset(resolved) : 0;
			}

			[[nodiscard]] explicit operator ID() const noexcept
			{
				return ID(id());
			}

			[[nodiscard]] constexpr std::uint64_t raw_id(std::size_t a_index) const noexcept
			{
				return a_index < N ? m_ids[a_index] : 0;
			}

			[[nodiscard]] constexpr bool has_explicit_id(std::size_t a_runtimeIndex) const noexcept
			{
				return a_runtimeIndex < N && m_ids[a_runtimeIndex] != 0;
			}

			[[nodiscard]] constexpr std::uint64_t operator[](std::size_t a_index) const noexcept
			{
				return raw_id(a_index);
			}

		private:
			std::array<std::uint64_t, N> m_ids{};
		};
	}

	using RelocationID = detail::RelocationIDImpl<detail::DEFAULT_RUNTIME_COUNT>;
	using RelocationID1 = detail::RelocationIDImpl<1>;
	using RelocationID2 = detail::RelocationIDImpl<2>;
	using RelocationID3 = detail::RelocationIDImpl<3>;
	using RelocationID4 = detail::RelocationIDImpl<4>;
}
