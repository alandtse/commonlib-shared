#pragma once

#include "REX/BASE.h"
#include "REX/FModule.h"

#include "REL/ID.h"
#include "REL/IDDB.h"

#ifndef REL_VARIANT_OFFSET_MASK
// Bitmask selecting which runtime slots hold direct RVAs instead of IDDB IDs.
// Default (1 << 2) treats slot 2 (VR) as a direct offset and slots 0/1 (SE/AE) as IDs.
#	define REL_VARIANT_OFFSET_MASK (1ULL << 2)
#endif

namespace REL
{
	namespace detail
	{
		template <std::size_t N = DEFAULT_RUNTIME_COUNT, std::uint64_t OffsetMask = REL_VARIANT_OFFSET_MASK>
		class VariantIDImpl
		{
		public:
			static_assert(N >= 1, "VariantIDImpl requires at least 1 runtime");

			constexpr VariantIDImpl() noexcept = default;

			explicit constexpr VariantIDImpl(std::array<std::uint64_t, N> a_values) noexcept :
				m_values(a_values)
			{}

			template <typename... Args>
			constexpr VariantIDImpl(Args... a_args) noexcept
				requires(sizeof...(Args) >= 1 && (N == 1 || sizeof...(Args) == N))
			{
				const std::array<std::uint64_t, sizeof...(Args)> temp{ static_cast<std::uint64_t>(a_args)... };
				if constexpr (N == 1) {
					m_values[0] = temp[0];
				} else {
					m_values = temp;
				}
			}

			// Do not copy an ID into slot 2 if slot 2 is configured as a direct RVA offset.
			constexpr VariantIDImpl(std::uint64_t a_primary, std::uint64_t a_secondary) noexcept
				requires(N == 3 || N == 4)
			{
				m_values[0] = a_primary;
				m_values[1] = a_secondary;
				m_values[2] = (OffsetMask & (1ULL << 2)) == 0 ? a_primary : 0;
				if constexpr (N == 4) {
					m_values[3] = 0;
				}
			}

			[[nodiscard]] std::uintptr_t address() const
			{
				const auto off = offset();
				return off ? REX::FModule::GetExecutingModule().GetBaseAddress() + off : 0;
			}

			[[nodiscard]] std::size_t offset() const
			{
				const auto idx = get_current_runtime_index<N>();
				if (idx >= N) {
					return 0;
				}

				const auto val = m_values[idx];
				if (val == 0) {
					return 0;
				}

				if constexpr (OffsetMask != 0) {
					if (idx < 64 && (OffsetMask & (1ULL << idx)) != 0) {
						return static_cast<std::size_t>(val);
					}
				}

				return static_cast<std::size_t>(IDDB::GetSingleton()->offset(val));
			}

			[[nodiscard]] constexpr std::uint64_t get(std::size_t a_runtimeIndex) const noexcept
			{
				return a_runtimeIndex < N ? m_values[a_runtimeIndex] : 0;
			}

			[[nodiscard]] constexpr std::uint64_t raw(std::size_t a_index) const noexcept
			{
				return a_index < N ? m_values[a_index] : 0;
			}

		private:
			std::array<std::uint64_t, N> m_values{};
		};
	}

	using VariantID = detail::VariantIDImpl<detail::DEFAULT_RUNTIME_COUNT, REL_VARIANT_OFFSET_MASK>;
	using VariantID2 = detail::VariantIDImpl<2, REL_VARIANT_OFFSET_MASK>;
	using VariantID3 = detail::VariantIDImpl<3, REL_VARIANT_OFFSET_MASK>;
	using VariantID4 = detail::VariantIDImpl<4, REL_VARIANT_OFFSET_MASK>;
}
