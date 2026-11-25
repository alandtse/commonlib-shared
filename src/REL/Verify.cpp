#include "REL/Verify.h"

#include "REX/LOG.h"

namespace REL
{
	std::uint64_t HashBytes(const std::uintptr_t a_addr, const std::size_t a_len) noexcept
	{
		constexpr std::uint64_t offsetBasis = 0xCBF29CE484222325ULL;
		constexpr std::uint64_t prime = 0x00000100000001B3ULL;

		auto       hash = offsetBasis;
		const auto bytes = reinterpret_cast<const std::uint8_t*>(a_addr);
		for (std::size_t i = 0; i < a_len; ++i) {
			hash ^= bytes[i];
			hash *= prime;
		}
		return hash;
	}

	VerifyResult VerifyBytes(
		const std::uintptr_t       a_addr,
		const std::size_t          a_len,
		const std::uint64_t        a_expectedHash,
		const std::string_view     a_context,
		const std::source_location a_loc)
	{
		const auto file = detail::basename(a_loc);
		const auto hash = HashBytes(a_addr, a_len);

		if (a_expectedHash == 0) {
			REX::DEBUG(
				"hook verification [{}:{}]: {} at 0x{:X} ({} bytes) hash 0x{:X} -- pass "
				"a_expectedHash=0x{:X} to pin this state (re-checked every launch).",
				file, a_loc.line(), a_context, a_addr, a_len, hash, hash);
			return VerifyResult::kUnset;
		}

		if (a_expectedHash == hash) {
			REX::TRACE(
				"hook verification [{}:{}]: {} at 0x{:X} matches its pinned hash 0x{:X} -- "
				"already verified, nothing changed.",
				file, a_loc.line(), a_context, a_addr, hash);
			return VerifyResult::kMatch;
		}

		REX::WARN(
			"hook verification [{}:{}]: {} at 0x{:X} pinned hash 0x{:X} no longer matches the "
			"live bytes (now 0x{:X}) -- the verification this relied on may no longer hold (a "
			"different game binary, or another patch now overlapping this range).",
			file, a_loc.line(), a_context, a_addr, a_expectedHash, hash);
		return VerifyResult::kMismatch;
	}
}
