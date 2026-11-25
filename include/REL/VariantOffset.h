#pragma once

#include "REX/BASE.h"
#include "REX/FModule.h"

#include "REL/ID.h"
#include "REL/Offset.h"

namespace REL
{
	namespace detail
	{
		template <std::size_t N = DEFAULT_RUNTIME_COUNT>
		class VariantOffsetImpl
		{
		public:
			static_assert(N >= 1, "VariantOffsetImpl requires at least 1 runtime");

			constexpr VariantOffsetImpl() noexcept = default;

			explicit constexpr VariantOffsetImpl(std::array<std::uint64_t, N> a_offsets) noexcept :
				m_offsets(a_offsets)
			{}

			template <typename... Args>
			constexpr VariantOffsetImpl(Args... a_args) noexcept
				requires(sizeof...(Args) >= 1 && (N == 1 || sizeof...(Args) == N))
			{
				const std::array<std::uint64_t, sizeof...(Args)> temp{ static_cast<std::uint64_t>(a_args)... };
				if constexpr (N == 1) {
					m_offsets[0] = temp[0];
				} else {
					m_offsets = temp;
				}
			}

			constexpr VariantOffsetImpl(std::uint64_t a_primary, std::uint64_t a_secondary) noexcept
				requires(N == 3 || N == 4)
			{
				m_offsets[0] = a_primary;
				m_offsets[1] = a_secondary;
				m_offsets[2] = a_primary;
				if constexpr (N == 4) {
					m_offsets[3] = 0;
				}
			}

			[[nodiscard]] std::uintptr_t address() const
			{
				const auto off = offset();
				return off ? REX::FModule::GetExecutingModule().GetBaseAddress() + off : 0;
			}

			[[nodiscard]] std::size_t offset() const noexcept
			{
				const auto idx = get_current_runtime_index<N>();
				return idx < N ? static_cast<std::size_t>(m_offsets[idx]) : 0;
			}

			[[nodiscard]] explicit operator Offset() const noexcept
			{
				return Offset(offset());
			}

			[[nodiscard]] constexpr std::uint64_t raw(std::size_t a_index) const noexcept
			{
				return a_index < N ? m_offsets[a_index] : 0;
			}

		private:
			std::array<std::uint64_t, N> m_offsets{};
		};
	}

	using VariantOffset = detail::VariantOffsetImpl<detail::DEFAULT_RUNTIME_COUNT>;
	using VariantOffset2 = detail::VariantOffsetImpl<2>;
	using VariantOffset3 = detail::VariantOffsetImpl<3>;
	using VariantOffset4 = detail::VariantOffsetImpl<4>;
}
