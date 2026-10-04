#pragma once

#include <cstddef>
#include <string>
#include <string_view>

class TextUtils final
{
public:
	//NOTE: where a UTF-8 text's symbol starts - the bytes continuing one are 10xxxxxx; past the end is the size
	[[nodiscard]] static std::size_t ByteOf(std::string_view text, std::size_t symbol);
	[[nodiscard]] static std::size_t SymbolCount(std::string_view text);
	//NOTE: cut or filled with spaces to this many symbols - a column of the monospaced font
	[[nodiscard]] static std::string Fitted(std::string text, std::size_t symbols);

	//NOTE: <cctype> is undefined for a negative char, and a UTF-8 byte is one - these read it as unsigned
	[[nodiscard]] static bool IsDigit(char symbol);
	[[nodiscard]] static bool IsHexDigit(char symbol);
	[[nodiscard]] static bool IsSpace(char symbol);
	[[nodiscard]] static bool IsAlnum(char symbol);
	[[nodiscard]] static char ToLower(char symbol);
};
