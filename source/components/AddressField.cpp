#include "components/AddressField.h"
#include "components/events/InputEvents.h"
#include "network/Endpoints.h"
#include "utils/TextUtils.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t kOctets{4};
constexpr std::size_t kOctetDigits{3};
constexpr int kMaxOctet{255};
//NOTE: the whole address, then the port - a scope id typed after '%' takes a part between them
constexpr std::size_t kIPv6Parts{2};
constexpr std::size_t kScopePart{1};
constexpr std::ptrdiff_t kIPv6Groups{8};
constexpr std::size_t kGroupDigits{4};

[[nodiscard]] bool IsOctet(const std::string& part)
{
	return part.size() <= kOctetDigits && (part.empty() || std::stoi(part) <= kMaxOctet);
}

[[nodiscard]] bool IsPort(const std::string& part)
{
	return part.size() <= AddressField::kPortDigits && (part.empty() || network::ParsePort(part).has_value());
}

[[nodiscard]] std::size_t GroupStart(const std::string_view address, const std::size_t offset)
{
	const std::size_t colon{address.substr(0, offset).rfind(':')};

	return colon == std::string_view::npos ? 0 : colon + 1;
}

[[nodiscard]] std::string_view GroupAt(const std::string_view address, const std::size_t offset)
{
	const std::size_t start{GroupStart(address, offset)};

	return address.substr(start, address.find(':', offset) - start);
}

[[nodiscard]] std::ptrdiff_t GroupCount(const std::string_view address)
{
	return std::ranges::count_if(address | std::views::split(':'),
								 [](const auto& group) { return !std::ranges::empty(group); });
}

//NOTE: eight groups, or seven around "::"
[[nodiscard]] std::ptrdiff_t MaxGroups(const std::string_view address)
{
	return address.contains("::") ? kIPv6Groups - 1 : kIPv6Groups;
}

//NOTE: nothing more can follow
[[nodiscard]] bool IsWholeIPv6(const std::string_view address) { return GroupCount(address) == MaxGroups(address); }

//NOTE: the IPv6 row has a scope id once '%' is typed
[[nodiscard]] bool IsScoped(const std::vector<std::string>& ipv6) { return ipv6.size() == kIPv6Parts + 1; }

//NOTE: '%' with nothing after it yet - erasing it takes the part away
[[nodiscard]] bool IsScopeEmpty(const std::vector<std::string>& ipv6)
{
	return IsScoped(ipv6) && ipv6[kScopePart].empty();
}

//NOTE: the address shares the room of a whole one with its scope id, so the row never outgrows its cell
[[nodiscard]] std::size_t AddressRoom(const std::vector<std::string>& ipv6)
{
	return IsScoped(ipv6) ? AddressField::kIPv6Length - 1 - ipv6[kScopePart].size() : AddressField::kIPv6Length;
}

//NOTE: erased, it would join two groups past four digits - so it is stepped over, as a dot between octets
[[nodiscard]] bool IsKeptColon(const std::string_view address, const std::size_t at)
{
	return address[at] == ':' && GroupAt(address, at).size() + GroupAt(address, at + 1).size() > kGroupDigits;
}

//NOTE: where the typed digit ends up once a group past four hands its last one on; nothing with no room left
[[nodiscard]] std::optional<std::size_t> FlowIPv6(std::string& address, std::size_t typed, const std::size_t room)
{
	for (std::size_t start{GroupStart(address, typed)};;)
	{
		const std::size_t end{std::min(address.find(':', start), address.size())};
		if (end - start <= kGroupDigits)
		{
			break;
		}

		const std::size_t last{end - 1};
		if (end == address.size())
		{
			address.insert(last, 1, ':');
			typed += typed == last ? 1 : 0;
			break;
		}

		const std::size_t next{std::min(address.find_first_not_of(':', end), address.size())};
		const char carried{address[last]};
		address.erase(last, 1);
		address.insert(next - 1, 1, carried);
		typed = typed == last ? next - 1 : typed;
		start = next - 1;
	}

	if (GroupCount(address) > MaxGroups(address) || address.size() > room)
	{
		return std::nullopt;
	}

	return typed;
}
}//namespace

AddressField::AddressField(const AddressFamily family, const AddressInput input)
	: _row{Row{.parts = std::vector<std::string>(family == AddressFamily::IPv6 ? kIPv6Parts : kOctets + 1)}}
	, _family{family}
	, _input{input} {}

bool AddressField::IsIPv6() const noexcept { return _family == AddressFamily::IPv6; }

std::size_t AddressField::FirstTypedPart() const noexcept
{
	return _input == AddressInput::Picked ? _row.parts.size() - 1 : 0;
}

void AddressField::Fill(const network::ServerAddress& address, const bool isBracketed)
{
	if (IsIPv6())
	{
		//NOTE: a scope id keeps its part when the address leaves it room
		const std::size_t percent{address.host.find('%')};
		const bool isScoped{percent != std::string::npos && address.host.size() <= kIPv6Length};
		_row.parts = std::vector<std::string>(isScoped ? kIPv6Parts + 1 : kIPv6Parts);
		_row.parts.front() = address.host.substr(0, percent);
		if (isScoped)
		{
			_row.parts[kScopePart] = address.host.substr(percent + 1);
		}

		_isBracketed = isBracketed;
	}
	else
	{
		std::ranges::fill(_row.parts, std::string{});
		for (auto&& [part, octet]: std::views::zip(_row.parts, address.host | std::views::split('.')))
		{
			part = std::ranges::to<std::string>(octet);
		}
	}

	_row.parts.back() = address.port == network::kAnyFreePort ? std::string{} : std::to_string(address.port);
	_row.StepToEnd(_row.parts.size() - 1);
}

void AddressField::Type(const char symbol)
{
	if (_row.part == _row.parts.size() - 1)
	{
		TypePort(symbol);
	}
	else if (IsIPv6() && _row.part == 0)
	{
		TypeIPv6(symbol);
	}
	else if (IsIPv6())
	{
		TypeScope(symbol);
	}
	else
	{
		TypeIPv4(symbol);
	}
}

void AddressField::Press(const TextKey key)
{
	_isColonAdded = false;
	switch (key)
	{
		case TextKey::Erase:
			Erase();
			break;
		case TextKey::EraseRight:
			EraseRight();
			break;
		case TextKey::CaretLeft:
			CaretLeft();
			break;
		case TextKey::CaretRight:
			CaretRight();
			break;
		case TextKey::WordLeft:
			WordLeft();
			break;
		case TextKey::WordRight:
			WordRight();
			break;
		case TextKey::NextField:
		case TextKey::PreviousField:
			break;
	}
}

void AddressField::Focus() { _isColonAdded = false; }

void AddressField::CaretToStart() { _row.StepToStart(FirstTypedPart()); }

bool AddressField::IsCaretAtStart() const noexcept { return _row.part == 0 && _row.offset == 0; }

//NOTE: a digit pushes the rest right, an octet past 255 handing its last one on; with no room it overwrites
void AddressField::TypeIPv4(const char symbol)
{
	const std::size_t port{_row.parts.size() - 1};
	//NOTE: a dot only out of a started octet, and never past the last one
	if (symbol == ':' || (symbol == '.' && _row.offset > 0 && _row.part + 1 < port))
	{
		_row.StepToStart(symbol == ':' ? port : _row.part + 1);

		return;
	}

	if (!TextUtils::IsDigit(symbol))
	{
		return;
	}

	//NOTE: an octet no digit can follow hands the caret on at once
	const auto handOn = [](Row& typed)
	{
		const std::string& octet{typed.parts[typed.part]};
		if (typed.offset == octet.size() && (octet.size() == kOctetDigits || std::stoi(octet) * 10 > kMaxOctet))
		{
			typed.StepToStart(typed.part + 1);
		}
	};

	Row flowed{_row};
	flowed.parts[flowed.part].insert(flowed.offset++, 1, symbol);
	for (std::size_t at{flowed.part}; at + 1 < port; ++at)
	{
		std::string& octet{flowed.parts[at]};
		while (!IsOctet(octet))
		{
			flowed.parts[at + 1].insert(0, 1, octet.back());
			octet.pop_back();
			if (flowed.part == at + 1)
			{
				++flowed.offset;
			}
			else if (flowed.part == at && flowed.offset > octet.size())
			{
				flowed.StepToStart(at + 1);
				++flowed.offset;
			}
		}
	}

	if (IsOctet(flowed.parts[port - 1]))
	{
		_row = std::move(flowed);
		handOn(_row);

		return;
	}

	Row over{_row};
	if (over.offset == over.parts[over.part].size())
	{
		over.StepToStart(over.part + 1);
	}

	if (over.part == port)
	{
		_row = std::move(over);
		TypePort(symbol);

		return;
	}

	over.parts[over.part][over.offset++] = symbol;
	if (IsOctet(over.parts[over.part]))
	{
		_row = std::move(over);
		handOn(_row);
	}
}

//NOTE: a digit pushes the rest right, a group past four handing its last one on; with no room it overwrites
void AddressField::TypeIPv6(const char symbol)
{
	const bool isAfterAddedColon{std::exchange(_isColonAdded, false)};
	const std::size_t port{_row.parts.size() - 1};
	std::string& address{_row.parts.front()};
	const bool isAtEnd{_row.offset == address.size()};
	//NOTE: the closing bracket ends the address, as a colon does in IPv4
	if (symbol == '[' || symbol == ']')
	{
		_isBracketed = true;
		if (symbol == ']')
		{
			if (isAfterAddedColon)
			{
				address.pop_back();
			}

			_row.StepToStart(port);
		}

		return;
	}

	//NOTE: a percent sign ends the address with a scope id - the number of the interface to go out by
	if (symbol == '%')
	{
		if (isAfterAddedColon)
		{
			address.pop_back();
		}

		if (!IsScoped(_row.parts) && address.size() + 1 < kIPv6Length)
		{
			_row.parts.insert(std::prev(_row.parts.end()), std::string{});
		}

		if (IsScoped(_row.parts))
		{
			_row.StepToEnd(kScopePart);
		}

		return;
	}

	//NOTE: a colon right of the caret is stepped over, and so is one typed right after the added one
	if (symbol == ':')
	{
		if (!isAtEnd && address[_row.offset] == ':')
		{
			++_row.offset;
		}
		else if (isAtEnd && IsWholeIPv6(address))
		{
			_row.StepToStart(port);
		}
		else if (!isAfterAddedColon && address.size() < AddressRoom(_row.parts))
		{
			address.insert(_row.offset++, 1, ':');
		}

		return;
	}

	if (!TextUtils::IsHexDigit(symbol))
	{
		return;
	}

	std::string flowed{address};
	flowed.insert(_row.offset, 1, symbol);
	if (const std::optional<std::size_t> typed{FlowIPv6(flowed, _row.offset, AddressRoom(_row.parts))})
	{
		address = std::move(flowed);
		_row.offset = *typed + 1;
		CloseFullGroup();

		return;
	}

	//NOTE: past a whole address only the port is left
	if (isAtEnd)
	{
		if (IsWholeIPv6(address))
		{
			_row.StepToStart(port);
			TypePort(symbol);
		}

		return;
	}

	const std::size_t next{address.find_first_not_of(':', _row.offset)};
	if (next != std::string::npos)
	{
		address[next] = symbol;
		_row.offset = next + 1;
		CloseFullGroup();
	}
}

//NOTE: Windows names an interface by its number; a colon or the closing bracket goes on to the port
void AddressField::TypeScope(const char symbol)
{
	if (symbol == ':' || symbol == ']')
	{
		_isBracketed = _isBracketed || symbol == ']';
		_row.StepToStart(_row.parts.size() - 1);

		return;
	}

	if (TextUtils::IsDigit(symbol) && _row.parts.front().size() < AddressRoom(_row.parts))
	{
		_row.parts[kScopePart].insert(_row.offset++, 1, symbol);
	}
}

//NOTE: a digit pushes the rest right, or with no room goes over the digit right of the caret
void AddressField::TypePort(const char symbol)
{
	if (!TextUtils::IsDigit(symbol))
	{
		return;
	}

	std::string& port{_row.parts.back()};
	std::string typed{port};
	typed.insert(_row.offset, 1, symbol);
	if (!IsPort(typed) && _row.offset < port.size())
	{
		typed = port;
		typed[_row.offset] = symbol;
	}

	if (IsPort(typed))
	{
		port = std::move(typed);
		++_row.offset;
	}
}

//NOTE: a group filled at the end gets its colon, like a full octet; past a whole address only the port is left
void AddressField::CloseFullGroup()
{
	std::string& address{_row.parts.front()};
	if (_row.offset < address.size() || GroupAt(address, _row.offset).size() < kGroupDigits)
	{
		return;
	}

	if (IsWholeIPv6(address))
	{
		_row.StepToStart(_row.parts.size() - 1);
	}
	else if (address.size() < AddressRoom(_row.parts))
	{
		address.push_back(':');
		++_row.offset;
		_isColonAdded = true;
	}
}

//NOTE: at the start of a part it reaches back over the separator, at the start of IPv6 over the bracket
void AddressField::Erase()
{
	if (IsIPv6() && _row.part == 0 && _row.offset == 0)
	{
		_isBracketed = false;
	}

	//NOTE: right after the '%' of an empty scope id it is the '%' that goes
	if (_row.part == kScopePart && IsScopeEmpty(_row.parts))
	{
		_row.parts.erase(std::next(_row.parts.begin()));
		_row.StepToEnd(0);

		return;
	}

	if (_row.offset == 0 && _row.part > FirstTypedPart())
	{
		_row.StepToEnd(_row.part - 1);
	}

	if (_row.offset > 0)
	{
		std::string& part{_row.parts[_row.part]};
		--_row.offset;
		if (IsKeptColon(part, _row.offset))
		{
			--_row.offset;
		}

		part.erase(_row.offset, 1);
	}
}

//NOTE: at the end of a part it reaches over the separator into the next one
void AddressField::EraseRight()
{
	if (_row.part == 0 && _row.offset == _row.parts.front().size() && IsScopeEmpty(_row.parts))
	{
		_row.parts.erase(std::next(_row.parts.begin()));

		return;
	}

	if (_row.offset == _row.parts[_row.part].size() && _row.part + 1 < _row.parts.size())
	{
		_row.StepToStart(_row.part + 1);
	}

	std::string& part{_row.parts[_row.part]};
	if (_row.offset < part.size())
	{
		if (IsKeptColon(part, _row.offset))
		{
			++_row.offset;
		}

		part.erase(_row.offset, 1);
	}
}

//NOTE: a separator is stepped over like a symbol
void AddressField::CaretLeft()
{
	if (_row.offset > 0)
	{
		--_row.offset;
	}
	else if (_row.part > FirstTypedPart())
	{
		_row.StepToEnd(_row.part - 1);
	}
}

void AddressField::CaretRight()
{
	if (_row.offset < _row.parts[_row.part].size())
	{
		++_row.offset;
	}
	else if (_row.part + 1 < _row.parts.size())
	{
		_row.StepToStart(_row.part + 1);
	}
}

//NOTE: to the start of the word, or of the previous one - octets, IPv6 groups and the port are words
void AddressField::WordLeft()
{
	if (_row.offset == 0 && _row.part > FirstTypedPart())
	{
		_row.StepToEnd(_row.part - 1);
	}

	const std::string& part{_row.parts[_row.part]};
	if (!IsIPv6() || _row.part != 0)
	{
		_row.offset = 0;

		return;
	}

	while (_row.offset > 0 && part[_row.offset - 1] == ':')
	{
		--_row.offset;
	}

	while (_row.offset > 0 && part[_row.offset - 1] != ':')
	{
		--_row.offset;
	}
}

//NOTE: to the end of the word, or of the next one
void AddressField::WordRight()
{
	const std::size_t port{_row.parts.size() - 1};
	const std::string& part{_row.parts[_row.part]};
	if (IsIPv6() && _row.part == 0 && _row.offset < part.size())
	{
		while (_row.offset < part.size() && part[_row.offset] == ':')
		{
			++_row.offset;
		}

		while (_row.offset < part.size() && part[_row.offset] != ':')
		{
			++_row.offset;
		}

		return;
	}

	if (_row.offset < part.size())
	{
		_row.offset = part.size();
	}
	else if (_row.part < port)
	{
		_row.StepToEnd(_row.part + 1);
	}
}

std::string AddressField::Text() const
{
	const std::string& port{_row.parts.back()};

	std::string host{};
	if (IsIPv6())
	{
		//NOTE: the colon a full group gets only leads on to the next one - the address ends before it
		std::string_view address{_row.parts.front()};
		if (address.ends_with(':') && !address.ends_with("::"))
		{
			address.remove_suffix(1);
		}

		const bool hasScope{IsScoped(_row.parts) && !_row.parts[kScopePart].empty()};
		host = '[' + std::string{address} + (hasScope ? '%' + _row.parts[kScopePart] : std::string{}) + ']';
	}
	else
	{
		//NOTE: an octet typed with leading zeros goes out as its number - the parser takes "010" for no octet
		const auto octet = [](const std::string& part)
		{
			return part.empty() ? part : std::to_string(std::stoi(part));
		};
		host = _row.parts | std::views::take(kOctets) | std::views::transform(octet) | std::views::join_with('.')
			   | std::ranges::to<std::string>();
	}

	return port.empty() ? host : host + ':' + port;
}

ShownAddress AddressField::Shown(const bool isFocused) const
{
	ShownAddress shown{};
	const auto append = [this, &shown, isFocused](const std::size_t at, const std::string_view placeholder)
	{
		if (isFocused && at == _row.part)
		{
			shown.caret = shown.text.size() + _row.offset;
		}

		const std::string& text{_row.parts[at]};
		shown.text += text.empty() ? placeholder : text;
	};

	const std::size_t port{_row.parts.size() - 1};
	if (IsIPv6())
	{
		//NOTE: the brackets have their places either way, so showing them does not move the address
		shown.text += _isBracketed ? '[' : ' ';
		append(0, "____");
		if (IsScoped(_row.parts))
		{
			shown.text += '%';
			append(kScopePart, "");
		}

		shown.text += _isBracketed ? ']' : ' ';
	}
	else
	{
		for (std::size_t at{}; at < port; ++at)
		{
			if (at > 0)
			{
				shown.text += '.';
			}

			append(at, "___");
		}
	}

	shown.text += ':';
	append(port, "auto");

	return shown;
}
