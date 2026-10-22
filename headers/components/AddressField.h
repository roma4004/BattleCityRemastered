#pragma once

#include "network/Endpoints.h"
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class TextKey : char8_t;

enum class AddressFamily : char8_t
{
	IPv4,
	IPv6
};

//NOTE: a server's own address is picked from a list - then only its port is typed, and the caret stays in it
enum class AddressInput : char8_t
{
	Typed,
	Picked
};

//NOTE: the row as drawn - placeholders in the empty parts, and where the caret stands in it
struct ShownAddress
{
	std::string text{};
	std::optional<std::size_t> caret{};
};

//NOTE: one typed address - IPv4 or IPv6 plus a port; the separators between parts are drawn, never typed
class AddressField final
{
	struct Row
	{
		std::vector<std::string> parts{};
		std::size_t part{};
		//NOTE: symbols of the part before the caret
		std::size_t offset{};

		void StepToStart(const std::size_t at)
		{
			part = at;
			offset = 0;
		}

		void StepToEnd(const std::size_t at)
		{
			part = at;
			offset = parts[at].size();
		}
	};

	Row _row{};
	AddressFamily _family{};
	AddressInput _input{};
	//NOTE: a full group added its colon itself - a colon typed right after it is not doubled; Press, Focus clear it
	bool _isColonAdded{};
	//NOTE: only drawn - the text never holds the brackets
	bool _isBracketed{};

	[[nodiscard]] bool IsIPv6() const noexcept;
	[[nodiscard]] std::size_t FirstTypedPart() const noexcept;
	void TypeIPv4(char symbol);
	void TypeIPv6(char symbol);
	void TypeScope(char symbol);
	void TypePort(char symbol);
	void CloseFullGroup();
	void Erase();
	void EraseRight();
	void CaretLeft();
	void CaretRight();
	void WordLeft();
	void WordRight();

public:
	//NOTE: eight groups of four and the seven colons between them
	static constexpr std::size_t kIPv6Length{39};
	static constexpr std::size_t kPortDigits{5};
	//NOTE: the IPv6 row at its longest - both rows are drawn small enough to fit it
	static constexpr std::size_t kLongestShown{std::string_view{"[]:"}.size() + kIPv6Length + kPortDigits};

	explicit AddressField(AddressFamily family, AddressInput input = AddressInput::Typed);

	//NOTE: the caret goes on the port; only an IPv6 address shows the brackets it was pasted in
	void Fill(const network::ServerAddress& address, bool isBracketed);
	void Type(char symbol);
	void Press(TextKey key);
	void Focus();
	void CaretToStart();

	[[nodiscard]] bool IsCaretAtStart() const noexcept;
	//NOTE: as the parser reads it
	[[nodiscard]] std::string Text() const;
	[[nodiscard]] ShownAddress Shown(bool isFocused) const;
};
