#pragma once

#include "REL/Version.h"

#include "REX/BASE.h"

#include "REX/FMemoryMap.h"
#include "REX/TSingleton.h"

#include <charconv>

namespace REL
{
	class IDDB :
	    public REX::TSingleton<IDDB>
	{
	public:
		enum class Loader : std::uint32_t
		{
			None,
			SKSE,
			F4SE,
			SFSE,
			OBSE,
		};

		enum class Format : std::int32_t
		{
			CSV = -100,
			None = std::numeric_limits<std::int32_t>::max(),
			V0 = 0,
			V1 = 1,
			V2 = 2,
			V5 = 5
		};

		struct MAPPING
		{
			std::uint64_t id;
			std::uint64_t offset;
		};

		IDDB();

		std::uint64_t offset(std::uint64_t a_id) const;

		[[nodiscard]] const std::optional<Version>& csv_version() const noexcept { return m_csvVersion; }
		[[nodiscard]] bool                          is_csv_at_least_version(std::string_view a_version) const;

	private:
		class STREAM;
		class HEADER_V2;
		class HEADER_V5;

		void load_v0();
		void load_v2(STREAM& a_stream);
		void load_v5(STREAM& a_stream);
		void load_csv();
		void unpack_file(STREAM& a_stream, const HEADER_V2& a_header);
		void validate_file();

	protected:
		friend class Offset2ID;

		// clang-format off
		template <class T> std::span<T>      get_id2offset() const noexcept;
		template <> std::span<MAPPING>       get_id2offset() const noexcept { return m_v0; }
		template <> std::span<std::uint32_t> get_id2offset() const noexcept { return m_v5; }
		// clang-format on

	private:
		std::string              m_name;
		std::filesystem::path    m_path;
		Loader                   m_loader{ Loader::None };
		Format                   m_format{ Format::None };
		REX::FMemoryMap          m_mmap;
		std::span<MAPPING>       m_v0;
		std::span<std::uint32_t> m_v5;
		std::optional<Version>   m_csvVersion;
	};

	namespace detail
	{
		[[nodiscard]] constexpr std::string_view trim_csv_field(std::string_view a_sv) noexcept
		{
			while (!a_sv.empty() && (a_sv.front() == ' ' || a_sv.front() == '\t' || a_sv.front() == '\r')) {
				a_sv.remove_prefix(1);
			}
			while (!a_sv.empty() && (a_sv.back() == ' ' || a_sv.back() == '\t' || a_sv.back() == '\r')) {
				a_sv.remove_suffix(1);
			}
			return a_sv;
		}

		[[nodiscard]] inline std::pair<std::uint64_t, std::string_view> parse_csv_metadata(std::string_view a_line) noexcept
		{
			a_line = trim_csv_field(a_line);
			const auto comma = a_line.find(',');
			if (comma == std::string_view::npos) {
				std::uint64_t count = 0;
				const auto    res = std::from_chars(a_line.data(), a_line.data() + a_line.size(), count, 10);
				if (res.ec != std::errc{} || res.ptr != a_line.data() + a_line.size()) {
					count = 0;
				}
				return { count, {} };
			}

			const auto countField = trim_csv_field(a_line.substr(0, comma));
			const auto versionField = trim_csv_field(a_line.substr(comma + 1));

			std::uint64_t count = 0;
			if (!countField.empty()) {
				const auto res = std::from_chars(countField.data(), countField.data() + countField.size(), count, 10);
				if (res.ec != std::errc{} || res.ptr != countField.data() + countField.size()) {
					count = 0;
				}
			}

			return { count, versionField };
		}

		[[nodiscard]] inline std::optional<IDDB::MAPPING> parse_csv_line(std::string_view a_line) noexcept
		{
			a_line = trim_csv_field(a_line);
			if (a_line.empty()) {
				return std::nullopt;
			}

			const auto comma = a_line.find(',');
			if (comma == std::string_view::npos) {
				return std::nullopt;
			}

			const auto idField = trim_csv_field(a_line.substr(0, comma));
			auto       offsetField = trim_csv_field(a_line.substr(comma + 1));
			if (idField.empty() || offsetField.empty()) {
				return std::nullopt;
			}

			if (offsetField.starts_with("0x") || offsetField.starts_with("0X")) {
				offsetField.remove_prefix(2);
				if (offsetField.empty()) {
					return std::nullopt;
				}
			}

			std::uint64_t id = 0;
			const auto    idRes = std::from_chars(idField.data(), idField.data() + idField.size(), id, 10);
			if (idRes.ec != std::errc{} || idRes.ptr != idField.data() + idField.size()) {
				return std::nullopt;
			}

			std::uint64_t offset = 0;
			const auto    offsetRes = std::from_chars(offsetField.data(), offsetField.data() + offsetField.size(), offset, 16);
			if (offsetRes.ec != std::errc{} || offsetRes.ptr != offsetField.data() + offsetField.size()) {
				return std::nullopt;
			}

			return IDDB::MAPPING{ id, offset };
		}
	}
}
