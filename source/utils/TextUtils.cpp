#include "utils/TextUtils.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>

namespace
{
[[nodiscard]] bool IsSymbolStart(const char byte) { return (static_cast<unsigned char>(byte) & 0xc0u) != 0x80u; }
}//namespace

std::size_t TextUtils::ByteOf(const std::string_view text, const std::size_t symbol)
{
	auto starts{text | std::views::filter(IsSymbolStart)};
	const auto at{std::ranges::next(starts.begin(), static_cast<std::ptrdiff_t>(symbol), starts.end())};

	return static_cast<std::size_t>(std::distance(text.begin(), at.base()));
}

std::size_t TextUtils::SymbolCount(const std::string_view text)
{
	return static_cast<std::size_t>(std::ranges::count_if(text, IsSymbolStart));
}

std::string TextUtils::Fitted(std::string text, const std::size_t symbols)
{
	text.resize(ByteOf(text, symbols));
	text.append(symbols - SymbolCount(text), ' ');

	return text;
}

bool TextUtils::IsDigit(const char symbol) { return std::isdigit(static_cast<unsigned char>(symbol)) != 0; }

bool TextUtils::IsHexDigit(const char symbol) { return std::isxdigit(static_cast<unsigned char>(symbol)) != 0; }

bool TextUtils::IsSpace(const char symbol) { return std::isspace(static_cast<unsigned char>(symbol)) != 0; }

bool TextUtils::IsAlnum(const char symbol) { return std::isalnum(static_cast<unsigned char>(symbol)) != 0; }

char TextUtils::ToLower(const char symbol)
{
	return static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
}
