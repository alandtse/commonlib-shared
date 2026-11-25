#include "REL/Version.h"

#include "REX/W32/VERSION.h"

namespace REL
{
	std::optional<Version> ParseVersionString(std::string_view a_version)
	{
		if (const auto parts = detail::parse_version_parts(a_version)) {
			return Version(*parts);
		}
		return std::nullopt;
	}

	std::optional<Version> GetFileVersion(std::string_view a_filename)
	{
		const std::string filename(a_filename);
		std::uint32_t     dummy;
		std::vector<char> buf(REX::W32::GetFileVersionInfoSizeA(filename.c_str(), std::addressof(dummy)));
		if (buf.empty()) {
			return std::nullopt;
		}

		if (!REX::W32::GetFileVersionInfoA(filename.c_str(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
			return std::nullopt;
		}

		void*         verBuf{ nullptr };
		std::uint32_t verLen{ 0 };
		if (!REX::W32::VerQueryValueA(buf.data(), "\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
			return std::nullopt;
		}

		std::string_view verStr(static_cast<const char*>(verBuf), verLen);
		if (const auto nul = verStr.find('\0'); nul != std::string_view::npos) {
			verStr = verStr.substr(0, nul);
		}

		return ParseVersionString(verStr);
	}

	std::optional<Version> GetFileVersion(std::wstring_view a_filename)
	{
		const std::wstring filename(a_filename);
		std::uint32_t      dummy;
		std::vector<char>  buf(REX::W32::GetFileVersionInfoSizeW(filename.c_str(), std::addressof(dummy)));
		if (buf.empty()) {
			return std::nullopt;
		}

		if (!REX::W32::GetFileVersionInfoW(filename.c_str(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
			return std::nullopt;
		}

		void*         verBuf{ nullptr };
		std::uint32_t verLen{ 0 };
		if (!REX::W32::VerQueryValueW(buf.data(), L"\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
			return std::nullopt;
		}

		std::wstring_view wverStr(static_cast<const wchar_t*>(verBuf), verLen);
		if (const auto nul = wverStr.find(L'\0'); nul != std::wstring_view::npos) {
			wverStr = wverStr.substr(0, nul);
		}

		std::string ascii;
		ascii.reserve(wverStr.size());
		for (const wchar_t ch : wverStr) {
			if (ch < 0 || ch > 0x7F) {
				return std::nullopt;
			}
			ascii.push_back(static_cast<char>(ch));
		}

		return ParseVersionString(ascii);
	}
}
