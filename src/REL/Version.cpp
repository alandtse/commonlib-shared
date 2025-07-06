#include "REL/Version.h"

#include "REX/W32/VERSION.h"
#include <charconv>
#include <regex>

namespace REL
{
	std::optional<Version> GetFileVersion(std::string_view a_filename)
	{
		std::uint32_t     dummy;
		std::vector<char> buf(REX::W32::GetFileVersionInfoSizeA(a_filename.data(), std::addressof(dummy)));
		if (buf.empty()) {
			return std::nullopt;
		}

		if (!REX::W32::GetFileVersionInfoA(a_filename.data(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
			return std::nullopt;
		}

		void*         verBuf{ nullptr };
		std::uint32_t verLen{ 0 };
		if (!REX::W32::VerQueryValueA(buf.data(), "\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
			return std::nullopt;
		}

		Version             version;
		std::wistringstream ss(std::wstring(static_cast<const wchar_t*>(verBuf), verLen));
		std::wstring        token;
		for (std::size_t i = 0; i < 4 && std::getline(ss, token, L'.'); ++i) {
			version[i] = static_cast<std::uint16_t>(std::stoi(token));
		}

		return version;
	}

	std::optional<Version> GetFileVersion(std::wstring_view a_filename)
	{
		std::uint32_t     dummy;
		std::vector<char> buf(REX::W32::GetFileVersionInfoSizeW(a_filename.data(), std::addressof(dummy)));
		if (buf.empty()) {
			return std::nullopt;
		}

		if (!REX::W32::GetFileVersionInfoW(a_filename.data(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
			return std::nullopt;
		}

		void*         verBuf{ nullptr };
		std::uint32_t verLen{ 0 };
		if (!REX::W32::VerQueryValueW(buf.data(), L"\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
			return std::nullopt;
		}

		Version             version;
		std::wistringstream ss(std::wstring(static_cast<const wchar_t*>(verBuf), verLen));
		std::wstring        token;
		for (std::size_t i = 0; i < 4 && std::getline(ss, token, L'.'); ++i) {
			version[i] = static_cast<std::uint16_t>(std::stoi(token));
		}

		return version;
	}

	std::optional<Version> ParseVersionString(std::string_view a_versionString)
	{
		// Parse version string like "1.10.163" or "1.10.163.0"
		std::regex version_regex(R"((\d+)\.(\d+)\.(\d+)(?:\.(\d+))?)");
		std::smatch matches;
		std::string versionStr(a_versionString);
		
		if (std::regex_match(versionStr, matches, version_regex)) {
			std::array<std::uint16_t, 4> version{ 0, 0, 0, 0 };
			
			for (std::size_t i = 1; i < matches.size() && i <= 4; ++i) {
				if (matches[i].matched) {
					std::uint16_t value = 0;
					std::from_chars(matches[i].str().data(), 
								   matches[i].str().data() + matches[i].str().size(), 
								   value);
					version[i - 1] = value;
				}
			}
			
			return Version(version);
		}
		
		return std::nullopt;
	}
}
