#pragma once

#include "REX/BASE.h"

#include "REL/IDDB.h"
#include "REL/Module.h"

namespace REL
{
	namespace detail
	{
		// Forward declarations - must be implemented by downstream library
		[[nodiscard]] std::size_t get_runtime_index() noexcept;

		// Generic runtime selection utilities
		template <class T>
		[[nodiscard]] T Relocate(T&& a_first, T&& a_second) noexcept
		{
			const auto runtime_index = get_runtime_index();
			return (runtime_index == 0) ? a_first : a_second;
		}

		template <class T>
		[[nodiscard]] T Relocate(T a_first, T a_second, T a_third) noexcept
		{
			const auto runtime_index = get_runtime_index();
			switch (runtime_index) {
				case 1:
					return a_second;
				case 2:
					return a_third;
				default:
					return a_first;
			}
		}

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
				const auto mod = ModuleBase::GetSingleton();
				return mod->base() + offset();
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

		// Multi-runtime RelocationID with smart fallback logic
		template <std::size_t N = 2>
		class RelocationIDImpl
		{
		public:
			constexpr RelocationIDImpl() noexcept = default;

			// Variadic constructor for convenience - handles any number of parameters
			// For single runtime builds (N == 1), additional parameters are ignored
			// For multi-runtime builds, parameter count must match N
			template <typename... Args>
			constexpr RelocationIDImpl(Args... args) noexcept
				requires(sizeof...(Args) >= 1 && (N == 1 || sizeof...(Args) == N))
			{
				// For single runtime builds, only use the first parameter
				if constexpr (N == 1) {
					m_ids[0] = static_cast<std::uint64_t>(std::get<0>(std::make_tuple(args...)));
					// Additional parameters are ignored
				} else {
					// For multi-runtime builds, all parameters are used
					const std::array<std::uint64_t, sizeof...(Args)> temp{ static_cast<std::uint64_t>(args)... };
					std::copy(temp.begin(), temp.end(), m_ids.begin());
				}
			}

			// Special constructor for 3-runtime mode with 2 parameters (VR fallback)
			// Example: RelocationIDImpl(1243, 0) - VR will automatically fall back to primary
			constexpr RelocationIDImpl(std::uint64_t a_primary, std::uint64_t a_secondary) noexcept
				requires(N == 3)
			{
				m_ids[0] = a_primary;
				m_ids[1] = a_secondary;
				m_ids[2] = 0;  // VR will fall back to primary via fallback logic
			}

			// General constructor for N runtimes
			explicit constexpr RelocationIDImpl(std::array<std::uint64_t, N> a_ids) noexcept :
				m_ids(a_ids)
			{}

			[[nodiscard]] std::uintptr_t address() const
			{
				const auto mod = ModuleBase::GetSingleton();
				return mod->base() + offset();
			}

			[[nodiscard]] std::uint64_t id() const noexcept
			{
				// Use runtime selection logic based on current runtime
				const auto runtime_index = get_current_runtime_index();
				return resolve_id(runtime_index);
			}

			// Smart ID resolution with fallback logic
			[[nodiscard]] std::uint64_t resolve_id(std::size_t a_runtime_index) const noexcept
			{
				if (a_runtime_index >= N) {
					return resolve_fallback();
				}

				auto candidate_id = m_ids[a_runtime_index];

				// If candidate is 0, try fallback strategies
				if (candidate_id == 0) {
					return resolve_fallback_for_runtime(a_runtime_index);
				}

				return candidate_id;
			}

			// Configurable fallback strategy - can be specialized per game
			[[nodiscard]] std::uint64_t resolve_fallback_for_runtime(std::size_t a_runtime_index) const noexcept
			{
				// Default strategy: fall back to primary (index 0)
				if (a_runtime_index > 0 && m_ids[0] != 0) {
					return m_ids[0];
				}

				// If primary is also 0, find first non-zero
				return resolve_fallback();
			}

			// Find first non-zero ID as ultimate fallback
			[[nodiscard]] std::uint64_t resolve_fallback() const noexcept
			{
				for (const auto& id_val : m_ids) {
					if (id_val != 0)
						return id_val;
				}
				return 0;
			}

			[[nodiscard]] std::size_t offset() const
			{
				const auto iddb = IDDB::GetSingleton();
				return iddb->offset(id());
			}

			// Direct access to raw IDs (before fallback resolution)
			[[nodiscard]] constexpr std::uint64_t raw_id(std::size_t a_index) const noexcept
			{
				return a_index < N ? m_ids[a_index] : 0;
			}

			// Check if runtime has explicit ID (not relying on fallback)
			[[nodiscard]] constexpr bool has_explicit_id(std::size_t a_runtime_index) const noexcept
			{
				return a_runtime_index < N && m_ids[a_runtime_index] != 0;
			}

			// Allow accessing specific runtime IDs with fallback resolution
			[[nodiscard]] std::uint64_t operator[](std::size_t a_index) const noexcept
			{
				return resolve_id(a_index);
			}

		private:
			// Runtime detection for current game version
			[[nodiscard]] static std::size_t get_current_runtime_index() noexcept
			{
#ifdef REL_CURRENT_RUNTIME
				// Single runtime build - use compile-time constant
				return REL_CURRENT_RUNTIME;
#else
				// Multi-runtime build - use downstream-provided function
				return get_runtime_index();
#endif
			}

			std::array<std::uint64_t, N> m_ids{};
		};

// Specialization for game-specific fallback logic could be added here
// For example, Skyrim VR might have its own pattern, or other games might need custom fallback logic

// Configuration system for downstream libraries
// Default configuration - can be overridden by downstream
#ifndef REL_DEFAULT_RUNTIME_COUNT
#	define REL_DEFAULT_RUNTIME_COUNT 1
#endif

		// Downstream can define this to set their default
		constexpr std::size_t DEFAULT_RUNTIME_COUNT = REL_DEFAULT_RUNTIME_COUNT;

		// Explicit size aliases for when you need specific counts
		using RelocationID1 = RelocationIDImpl<1>;  // Single runtime (equivalent to REL::ID)
		using RelocationID2 = RelocationIDImpl<2>;  // Two runtimes
		using RelocationID3 = RelocationIDImpl<3>;  // Three runtimes
		using RelocationID4 = RelocationIDImpl<4>;  // Four runtimes (future-proof)
	}  // namespace detail
}