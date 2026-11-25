#pragma once

#include "REX/BASE.h"

#ifndef REL_VERSION_SEPARATOR
#	define REL_VERSION_SEPARATOR "."
#endif

#ifndef REL_VERSION_WSEPARATOR
#	define REL_VERSION_WSEPARATOR L"."
#endif

namespace REL
{
	namespace detail
	{
		[[nodiscard]] constexpr std::optional<std::array<std::uint16_t, 4>> parse_version_parts(std::string_view a_version) noexcept
		{
			if (a_version.empty()) {
				return std::nullopt;
			}

			std::array<std::uint16_t, 4> parts{};
			std::size_t                  index = 0;
			std::size_t                  pos = 0;

			while (pos < a_version.size()) {
				if (index >= parts.size()) {
					return std::nullopt;
				}

				const auto next = a_version.find('.', pos);
				const auto part = next == std::string_view::npos ? a_version.substr(pos) : a_version.substr(pos, next - pos);
				if (part.empty()) {
					return std::nullopt;
				}

				std::uint32_t value = 0;
				for (const char ch : part) {
					if (ch < '0' || ch > '9') {
						return std::nullopt;
					}
					value = value * 10u + static_cast<std::uint32_t>(ch - '0');
					if (value > (std::numeric_limits<std::uint16_t>::max)()) {
						return std::nullopt;
					}
				}

				parts[index++] = static_cast<std::uint16_t>(value);

				if (next == std::string_view::npos) {
					break;
				}
				pos = next + 1;
				if (pos == a_version.size()) {
					return std::nullopt;
				}
			}

			return parts;
		}
	}

	template <typename T>
	class VersionBase
	{
	public:
		using value_type = std::uint16_t;
		using reference = value_type&;
		using const_reference = const value_type&;

		constexpr VersionBase() noexcept = default;

		explicit constexpr VersionBase(std::array<value_type, 4> a_version) noexcept :
			_impl(a_version)
		{}

		constexpr VersionBase(value_type a_v1, value_type a_v2 = 0, value_type a_v3 = 0, value_type a_v4 = 0) noexcept :
			_impl{ a_v1, a_v2, a_v3, a_v4 }
		{}

		explicit constexpr VersionBase(std::string_view a_version)
		{
			const auto parts = detail::parse_version_parts(a_version);
			if (!parts) {
				throw std::invalid_argument("invalid version string");
			}
			_impl = *parts;
		}

		[[nodiscard]] constexpr reference       operator[](std::size_t a_idx) noexcept { return _impl[a_idx]; }
		[[nodiscard]] constexpr const_reference operator[](std::size_t a_idx) const noexcept { return _impl[a_idx]; }

		[[nodiscard]] constexpr decltype(auto) begin() const noexcept { return _impl.begin(); }
		[[nodiscard]] constexpr decltype(auto) cbegin() const noexcept { return _impl.cbegin(); }
		[[nodiscard]] constexpr decltype(auto) end() const noexcept { return _impl.end(); }
		[[nodiscard]] constexpr decltype(auto) cend() const noexcept { return _impl.cend(); }

		[[nodiscard]] std::strong_ordering constexpr compare(const VersionBase& a_rhs) const noexcept
		{
			for (std::size_t i = 0; i < _impl.size(); ++i) {
				if ((*this)[i] != a_rhs[i]) {
					return (*this)[i] < a_rhs[i] ? std::strong_ordering::less : std::strong_ordering::greater;
				}
			}
			return std::strong_ordering::equal;
		}

		[[nodiscard]] constexpr std::uint32_t pack() const noexcept
		{
			return static_cast<std::uint32_t>(
				(_impl[0] & T::AND_MAJOR) << T::SHL_MAJOR |
				(_impl[1] & T::AND_MINOR) << T::SHL_MINOR |
				(_impl[2] & T::AND_PATCH) << T::SHL_PATCH |
				(_impl[3] & T::AND_BUILD) << T::SHL_BUILD);
		}

		[[nodiscard]] constexpr value_type major() const noexcept { return _impl[0]; }
		[[nodiscard]] constexpr value_type minor() const noexcept { return _impl[1]; }
		[[nodiscard]] constexpr value_type patch() const noexcept { return _impl[2]; }
		[[nodiscard]] constexpr value_type build() const noexcept { return _impl[3]; }

		[[nodiscard]] constexpr std::string string(const std::string_view a_separator = std::string_view(REL_VERSION_SEPARATOR)) const
		{
			std::string result;
			for (auto&& ver : _impl) {
				result += std::to_string(ver);
				result.append(a_separator.data(), a_separator.size());
			}
			result.erase(result.size() - a_separator.size(), a_separator.size());
			return result;
		}

		[[nodiscard]] constexpr std::wstring wstring(const std::wstring_view a_separator = std::wstring_view(REL_VERSION_WSEPARATOR)) const
		{
			std::wstring result;
			for (auto&& ver : _impl) {
				result += std::to_wstring(ver);
				result.append(a_separator.data(), a_separator.size());
			}
			result.erase(result.size() - a_separator.size(), a_separator.size());
			return result;
		}

		[[nodiscard]] static constexpr VersionBase unpack(const std::uint32_t a_packedVersion) noexcept
		{
			return VersionBase{
				static_cast<value_type>((a_packedVersion >> T::SHL_MAJOR) & T::AND_MAJOR),
				static_cast<value_type>((a_packedVersion >> T::SHL_MINOR) & T::AND_MINOR),
				static_cast<value_type>((a_packedVersion >> T::SHL_PATCH) & T::AND_PATCH),
				static_cast<value_type>((a_packedVersion >> T::SHL_BUILD) & T::AND_BUILD)
			};
		}

		[[nodiscard]] friend constexpr bool operator==(const VersionBase& a_lhs, const VersionBase& a_rhs) noexcept
		{
			return a_lhs.compare(a_rhs) == 0;
		}

		[[nodiscard]] friend constexpr std::strong_ordering operator<=>(const VersionBase& a_lhs, const VersionBase& a_rhs) noexcept
		{
			return a_lhs.compare(a_rhs);
		}

	private:
		std::array<value_type, 4> _impl{ 0, 0, 0, 0 };
	};

	struct VersionPackInfo
	{
		static constexpr auto AND_MAJOR{ 0x0FF };
		static constexpr auto AND_MINOR{ 0x0FF };
		static constexpr auto AND_PATCH{ 0xFFF };
		static constexpr auto AND_BUILD{ 0x00F };
		static constexpr auto SHL_MAJOR{ 8 * 3 };
		static constexpr auto SHL_MINOR{ 8 * 2 };
		static constexpr auto SHL_PATCH{ 8 / 2 };
		static constexpr auto SHL_BUILD{ 8 * 0 };
	};

	using Version = VersionBase<VersionPackInfo>;

	[[nodiscard]] std::optional<Version> ParseVersionString(std::string_view a_version);
	[[nodiscard]] std::optional<Version> GetFileVersion(std::string_view a_filename);
	[[nodiscard]] std::optional<Version> GetFileVersion(std::wstring_view a_filename);
}

template <typename T, class CharT>
struct std::formatter<REL::VersionBase<T>, CharT> :
	formatter<std::basic_string<CharT>, CharT>
{
	template <class FormatContext>
	constexpr auto format(const REL::VersionBase<T>& a_version, FormatContext& a_ctx) const
	{
		if constexpr (std::same_as<CharT, wchar_t>) {
			return formatter<std::basic_string<CharT>, CharT>::format(a_version.wstring(), a_ctx);
		} else {
			return formatter<std::basic_string<CharT>, CharT>::format(a_version.string(), a_ctx);
		}
	}
};
