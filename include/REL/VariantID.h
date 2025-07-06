#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace REL
{
    /**
     * @brief VariantID allows specifying IDs or offsets for multiple runtimes (e.g., SE/AE/VR or F4/NG/VR).
     *
     * Fallback behavior:
     * 1. If a runtime's ID is 0, it falls back to the first runtime's ID
     * 2. In 3-runtime mode, if only 2 parameters are provided, the first ID is used for both first and last runtime
     *
     * Example usage:
     *   // For SE/AE/VR:
     *   inline constexpr REL::VariantID3 MyFuncID{se_id, ae_id, 0}; // VR falls back to SE
     *   inline constexpr REL::VariantID3 MyFuncID{se_id, ae_id};    // VR uses SE ID
     *   // For F4/NG/VR:
     *   inline constexpr REL::VariantID3 MyFuncID{f4_id, ng_id, 0}; // VR falls back to F4
     *   inline constexpr REL::VariantID3 MyFuncID{f4_id, ng_id};    // VR uses F4 ID
     */
    template <std::size_t N>
    class VariantID
    {
    public:
        constexpr VariantID() noexcept = default;

        // Construct from N IDs/offsets
        constexpr VariantID(const std::array<std::uint64_t, N>& ids) noexcept : _ids(ids) {}

        // Variadic constructor for convenience
        template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
        constexpr VariantID(Args... args) noexcept : _ids{static_cast<std::uint64_t>(args)...} {}

        // Special constructor for 3-runtime mode with 2 parameters
        // First ID is used for both first and last runtime (e.g., F4 for both F4 and VR)
        constexpr VariantID(std::uint64_t first, std::uint64_t second) requires(N == 3) noexcept
            : _ids{first, second, first} {}

        // Get the ID/offset for the given runtime index, with fallback
        [[nodiscard]] std::uint64_t get(std::size_t runtime_index) const noexcept
        {
            if (runtime_index >= N)
                return 0;

            auto id = _ids[runtime_index];
            
            // If this runtime has a valid ID, use it
            if (id != 0)
                return id;
            
            // Otherwise, fall back to the first runtime's ID
            return _ids[0];
        }

        // Access raw value
        [[nodiscard]] constexpr std::uint64_t raw(std::size_t idx) const noexcept { return _ids[idx]; }

    private:
        std::array<std::uint64_t, N> _ids{};
    };

    // Common aliases
    using VariantID2 = VariantID<2>;
    using VariantID3 = VariantID<3>;
} 