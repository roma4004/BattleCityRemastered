#include "components/ServerScreen.h"
#include "components/EventSystem.h"
#include "components/UiTable.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
constexpr unsigned int kTextColor{0xffffffffu};
//NOTE: the scoreboard's color - buttons stand out
constexpr unsigned int kButtonColor{0xff00ffffu};
constexpr unsigned int kFullServerColor{0xffa0a0a0u};
constexpr unsigned int kErrorColor{0xffff0000u};

//NOTE: rows kept for the list, so a found server does not push the address down
constexpr std::size_t kListRows{4};

constexpr std::size_t kOctets{4};
constexpr std::size_t kOctetDigits{3};
constexpr int kMaxOctet{255};
constexpr std::size_t kPortDigits{5};
//NOTE: eight groups of four and the seven colons between them
constexpr std::size_t kIPv6Length{39};
//NOTE: the whole address, then the port - a scope id typed after '%' takes a part between them
constexpr std::size_t kIPv6Parts{2};
constexpr std::size_t kScopePart{1};
//NOTE: a little wider than the longest hint - a longer server row goes on to a second line
constexpr std::size_t kRowSymbols{40};
//NOTE: the IPv6 row at its longest - both typed rows are drawn small enough to fit it
constexpr std::size_t kTypedSymbols{std::string_view{"[]:"}.size() + kIPv6Length + kPortDigits};
constexpr std::ptrdiff_t kIPv6Groups{8};
constexpr std::size_t kGroupDigits{4};
//NOTE: a server tells only how many seats are free
constexpr int kSeats{2};
constexpr std::string_view kPlayersHeader{"PLAYERS  "};

//NOTE: a terminal caret's period, fading instead of switching
constexpr auto kCaretBlink{1060ms};

[[nodiscard]] bool IsDigit(const char symbol) { return std::isdigit(static_cast<unsigned char>(symbol)) != 0; }

[[nodiscard]] bool IsHexDigit(const char symbol) { return std::isxdigit(static_cast<unsigned char>(symbol)) != 0; }

[[nodiscard]] bool IsOctet(const std::string& part)
{
	return part.size() <= kOctetDigits && (part.empty() || std::stoi(part) <= kMaxOctet);
}

[[nodiscard]] bool IsPort(const std::string& part)
{
	return part.size() <= kPortDigits && (part.empty() || network::ParsePort(part).has_value());
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
	return IsScoped(ipv6) ? kIPv6Length - 1 - ipv6[kScopePart].size() : kIPv6Length;
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

//NOTE: whole when it has just moved, then out and back once a blink
[[nodiscard]] std::uint8_t CaretAlpha(const std::chrono::steady_clock::duration shown)
{
	const double phase{std::chrono::duration<double>{shown} / kCaretBlink};

	return static_cast<std::uint8_t>(std::lround(127.5 * (1.0 + std::cos(2.0 * std::numbers::pi * phase))));
}

[[nodiscard]] UiCell Word(std::string text, const unsigned int color = kTextColor)
{
	return TextCell(std::move(text), color);
}

[[nodiscard]] UiCell Address(std::string text, const unsigned int color = kTextColor)
{
	return UiCell{.text = std::move(text), .color = color, .symbols = kRowSymbols};
}

[[nodiscard]] UiCell Typed(std::string text)
{
	return UiCell{.text = std::move(text), .color = kTextColor, .symbols = kRowSymbols, .fitSymbols = kTypedSymbols};
}

[[nodiscard]] UiCell Centered(std::string text)
{
	return TextCell(std::move(text), kTextColor, UiAlign::Centered);
}

[[nodiscard]] UiTable Caption(std::string text)
{
	return UiTable{.rows = {UiRow{.cells = {Centered(std::move(text))}}}};
}

[[nodiscard]] UiCell Button(const bool isHosting, const bool isIPv6)
{
	return Word(std::string{isHosting ? "CREATE ON " : "CONNECT BY "} + (isIPv6 ? "IPv6" : "IPv4"), kButtonColor);
}

[[nodiscard]] UiTable Keys()
{
	return UiTable{.rows = {UiRow{},
							UiRow{.cells = {Word("Choose"), Word("Up / Down")}},
							UiRow{.cells = {Word("Caret"), Word("Left / Right, Space")}},
							UiRow{.cells = {Word("Word"), Word("Ctrl+Left / Right, Tab")}},
							UiRow{.cells = {Word("Port"), Word("optional, picked automatically")}},
							UiRow{.cells = {Word("Paste"), Word("Ctrl+V")}},
							UiRow{.cells = {Word("Confirm"), Word("Enter")}},
							UiRow{.cells = {Word("Back"), Word("Esc")}}}};
}

//NOTE: an address too long for the row goes on under itself, broken after a colon
[[nodiscard]] std::vector<std::string> ServerLines(const network::FoundServer& server)
{
	const int players{kSeats - server.freeSeats};
	std::string seats{server.freeSeats == 0 ? "FULL" : std::to_string(players) + '/' + std::to_string(kSeats)};
	seats.resize(kPlayersHeader.size(), ' ');
	const std::string host{server.host.contains(':') ? '[' + server.host + ']' : server.host};
	const std::string address{host + ':' + std::to_string(server.gamePort)};
	const std::size_t room{kRowSymbols - kPlayersHeader.size()};
	if (address.size() <= room)
	{
		return {seats + address};
	}

	const std::size_t cut{address.rfind(':', room - 1) + 1};

	return {seats + address.substr(0, cut), std::string(kPlayersHeader.size(), ' ') + address.substr(cut)};
}
//TEMP: fake servers for a visual check of the scrolled list - not for commit
void AddFakeServers(std::vector<network::FoundServer>& servers)
{
	const std::vector<network::FoundServer> fakes{
			{.host = "255.255.255.255", .gamePort = 65535, .freeSeats = 1},
			{.host = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", .gamePort = 65535, .freeSeats = 2},
			{.host = "192.168.0.10", .gamePort = 50001, .freeSeats = 2},
			{.host = "255.255.255.254", .gamePort = 65534, .freeSeats = 0},
			{.host = "2001:db8:85a3:1234:5678:8a2e:370:7334", .gamePort = 65535, .freeSeats = 1},
			{.host = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe", .gamePort = 65534, .freeSeats = 0},
			{.host = "10.0.0.6", .gamePort = 4001, .freeSeats = 0},
			{.host = "fdfd::1a54:4f90", .gamePort = 5000, .freeSeats = 2},
			{.host = "192.168.0.14", .gamePort = 50005, .freeSeats = 1}};
	servers.insert(servers.end(), fakes.begin(), fakes.end());
}

//NOTE: a server with someone waiting first, then an empty one, a full one last
[[nodiscard]] std::vector<network::FoundServer> JoinOrder(std::vector<network::FoundServer> servers)
{
	const auto rank = [](const network::FoundServer& server)
	{
		return server.freeSeats == 0 ? kSeats + 1 : server.freeSeats;
	};
	std::ranges::stable_sort(servers, {}, rank);

	return servers;
}

[[nodiscard]] std::size_t ListRows(const std::span<const network::FoundServer> servers)
{
	const auto rowsOf = [](const network::FoundServer& server) { return ServerLines(server).size(); };

	return std::ranges::fold_left(servers | std::views::transform(rowsOf), std::size_t{}, std::plus{});
}

//NOTE: as far as the list scrolls - from there its last servers fill the rows
[[nodiscard]] std::size_t LastFirstShown(const std::span<const network::FoundServer> servers)
{
	std::size_t first{servers.size()};
	while (first > 0 && ListRows(servers.subspan(first - 1)) <= kListRows)
	{
		--first;
	}

	return first;
}
}//namespace

ServerScreen::ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address)
	: _events{events}
	, _rows{Row{.parts = std::vector<std::string>(kOctets + 1)}, Row{.parts = std::vector<std::string>(kIPv6Parts)}}
{
	Fill(address);
	_rows.back().parts = {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "65535"}; //TEMP

	_subs.push_back(_events->AddListener(this, &ServerScreen::OnMenuShown));
}

void ServerScreen::Open(const GameMode mode)
{
	_mode = mode;
	_isRejected = false;
	_isConfirmHeld = false;
	_servers.clear();
	_firstShown = 0;

	if (!IsHosting())
	{
		_scan = std::make_unique<network::DiscoveryScan>(network::LocalAddress());
		AddFakeServers(_servers); //TEMP
		_servers = JoinOrder(std::move(_servers)); //TEMP
	}

	//NOTE: the caret starts at the address, only a paste puts it on the port
	std::ranges::for_each(_rows, [](Row& row) { row.StepToStart(0); });

	PickFirst();

	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnPreTickUpdate));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextTyped));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextPasted));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextKey));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnRowClicked));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnEnter));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnCancelled));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnDrawUserInterface));
	//NOTE: a pad cannot type, but it can pick a server and confirm
	for (const InputChannel channel: {InputChannel::LocalP1, InputChannel::LocalP2})
	{
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadUp));
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadDown));
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnFire));
	}

	//NOTE: announced first - the menu hidden for the screen then leaves the pause alone
	_events->EmitEvent(ServerScreenShownEvent{.isShown = true});
	_events->EmitEvent(ShowMenuEvent{.isShown = false});
}

bool ServerScreen::IsOpen() const noexcept { return !_openSubs.empty(); }

bool ServerScreen::IsHosting() const noexcept { return _mode == GameMode::PlayAsHost; }

void ServerScreen::Close()
{
	if (!IsOpen())
	{
		return;
	}

	_openSubs.clear();
	_scan.reset();
	_events->EmitEvent(ServerScreenShownEvent{.isShown = false});
}

//NOTE: the menu coming back closes the screen, whoever brought it
void ServerScreen::OnMenuShown(const MenuShownEvent& event)
{
	if (event.isShown)
	{
		Close();
	}
}

//NOTE: the focus follows its server as the list shifts, and leaves one gone
void ServerScreen::OnPreTickUpdate(const PreTickUpdateEvent&)
{
	if (!_scan || !_scan->Poll())
	{
		return;
	}

	const std::optional<network::FoundServer> focused{
			_focus.line == Line::Server ? std::optional{_servers[_focus.server]} : std::nullopt};
	const bool wasEmpty{_servers.empty()};
	_servers = JoinOrder(_scan->Servers());
	AddFakeServers(_servers); //TEMP
	_servers = JoinOrder(std::move(_servers)); //TEMP
	_firstShown = std::min(_firstShown, LastFirstShown(_servers));
	if (!focused)
	{
		//NOTE: the first servers found take the focus they would have had at opening, unless it has moved since
		const Row& typed{_rows.front()};
		if (wasEmpty && _focus == Item{.line = Line::IPv4} && typed.part == 0 && typed.offset == 0)
		{
			PickFirst();
		}

		return;
	}

	const auto isFocused = [&focused](const network::FoundServer& server)
	{
		return server.host == focused->host && server.gamePort == focused->gamePort;
	};
	const auto server{std::ranges::find_if(_servers, isFocused)};
	if (server != _servers.end())
	{
		Pick(Item{.line = Line::Server, .server = static_cast<std::size_t>(std::distance(_servers.begin(), server))});

		return;
	}

	PickFirst();
}

void ServerScreen::OnTextTyped(const TextTypedEvent& event)
{
	std::ranges::for_each(event.text, [this](const char symbol) { Type(symbol); });
	_isRejected = false;
	_caretMoved = std::chrono::steady_clock::now();
}

//NOTE: a whole address goes to its own row, anything else is typed in
void ServerScreen::OnTextPasted(const TextPastedEvent& event)
{
	std::string text{event.text};
	std::erase_if(text, [](const char symbol) { return std::isspace(static_cast<unsigned char>(symbol)) != 0; });

	if (const std::optional<network::ServerAddress> address{network::ParseServerAddress(text)})
	{
		Fill(*address);
		if (_focus.line == Line::IPv6)
		{
			_isBracketed = text.starts_with('[');
		}
	}
	else
	{
		std::ranges::for_each(text, [this](const char symbol) { Type(symbol); });
	}

	_isRejected = false;
	_caretMoved = std::chrono::steady_clock::now();
}

void ServerScreen::OnTextKey(const TextKeyEvent& event)
{
	_isRejected = false;
	_isColonAdded = false;
	_caretMoved = std::chrono::steady_clock::now();
	if (event.key == TextKey::NextField || event.key == TextKey::PreviousField)
	{
		Step(event.key == TextKey::NextField);

		return;
	}

	Row* const row{FocusedRow()};
	if (row == nullptr)
	{
		return;
	}

	switch (event.key)
	{
		case TextKey::Erase:
			Erase(*row);
			break;
		case TextKey::EraseRight:
			EraseRight(*row);
			break;
		case TextKey::CaretLeft:
			CaretLeft(*row);
			break;
		case TextKey::CaretRight:
			CaretRight(*row);
			break;
		case TextKey::WordLeft:
			WordLeft(*row);
			break;
		case TextKey::WordRight:
			WordRight(*row);
			break;
		case TextKey::NextField:
		case TextKey::PreviousField:
			break;
	}
}

//NOTE: a click on anything but an address row presses it at once
void ServerScreen::OnRowClicked(const PanelRowClickedEvent& event)
{
	const std::vector<Item> lines{Lines()};
	if (event.row >= lines.size())
	{
		return;
	}

	//NOTE: a server's second line picks the server
	Item item{lines[event.row]};
	item.line = item.line == Line::ServerTail ? Line::Server : item.line;
	if (!IsPickable(item))
	{
		return;
	}

	Pick(item);
	if (_focus.line != Line::IPv4 && _focus.line != Line::IPv6)
	{
		Press();
	}
}

void ServerScreen::OnEnter(const EnterEvent& event) { Confirm(event.isPressed); }

void ServerScreen::OnFire(const FireEvent& event) { Confirm(event.isPressed); }

void ServerScreen::OnPadUp(const MoveUpEvent& event)
{
	if (event.isPressed)
	{
		Step(false);
	}
}

void ServerScreen::OnPadDown(const MoveDownEvent& event)
{
	if (event.isPressed)
	{
		Step(true);
	}
}

void ServerScreen::OnCancelled(const TextInputCancelledEvent&)
{
	Close();
	_events->EmitEvent(ShowMenuEvent{.isShown = true});
}

void ServerScreen::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void ServerScreen::Confirm(const bool isPressed)
{
	if (isPressed)
	{
		_isConfirmHeld = true;

		return;
	}

	if (_isConfirmHeld)
	{
		_isConfirmHeld = false;
		Press();
	}
}

void ServerScreen::Press()
{
	if (_focus.line == Line::Refresh)
	{
		if (_scan)
		{
			_scan->Refresh();
		}

		return;
	}

	if (_focus.line == Line::Server)
	{
		//NOTE: a full server is scrolled through, not joined
		if (IsPickable(_focus) && _servers[_focus.server].freeSeats > 0)
		{
			const network::FoundServer& server{_servers[_focus.server]};
			Choose(network::ServerAddress{.host = server.host, .port = server.gamePort});
		}

		return;
	}

	const Line row{_focus.line == Line::IPv6 || _focus.line == Line::ConfirmIPv6 ? Line::IPv6 : Line::IPv4};
	const std::optional<network::ServerAddress> address{network::ParseServerAddress(Text(row))};
	_isRejected = !address;
	if (address)
	{
		Choose(*address);
	}
}

void ServerScreen::Choose(const network::ServerAddress& address)
{
	const GameMode mode{_mode};
	Close();
	_events->EmitEvent(ServerAddressChosenEvent{.mode = mode, .address = address});
}

std::vector<ServerScreen::Item> ServerScreen::Lines() const
{
	std::vector<Item> lines{};
	if (!IsHosting())
	{
		lines.push_back(Item{.line = Line::ServersCaption});
		lines.push_back(Item{.line = Line::ServersHeader});
		const std::size_t listStart{lines.size()};
		for (std::size_t server{_firstShown}; server < _servers.size(); ++server)
		{
			const std::size_t rows{ServerLines(_servers[server]).size()};
			if (lines.size() - listStart + rows > kListRows)
			{
				break;
			}

			lines.push_back(Item{.line = Line::Server, .server = server});
			if (rows > 1)
			{
				lines.push_back(Item{.line = Line::ServerTail, .server = server});
			}
		}

		if (_servers.empty())
		{
			lines.push_back(Item{.line = Line::NoServers});
		}

		lines.resize(listStart + kListRows, Item{.line = Line::Gap});
		lines.push_back(Item{.line = Line::Refresh});
		lines.push_back(Item{.line = Line::Gap});
		lines.push_back(Item{.line = Line::AddressCaption});
	}

	lines.push_back(Item{.line = Line::IPv4Caption});
	lines.push_back(Item{.line = Line::IPv4});
	lines.push_back(Item{.line = Line::ConfirmIPv4});
	lines.push_back(Item{.line = Line::IPv6Caption});
	lines.push_back(Item{.line = Line::IPv6});
	lines.push_back(Item{.line = Line::ConfirmIPv6});
	//NOTE: kept empty too, so an error does not move the hints
	lines.push_back(Item{.line = Line::Error});

	return lines;
}

bool ServerScreen::IsPickable(const Item& item) const
{
	switch (item.line)
	{
		case Line::ServersCaption:
		case Line::ServersHeader:
		case Line::NoServers:
		case Line::ServerTail:
		case Line::Gap:
		case Line::AddressCaption:
		case Line::IPv4Caption:
		case Line::IPv6Caption:
		case Line::Error:
			return false;
		case Line::Server:
			return item.server < _servers.size();
		case Line::Refresh:
		case Line::IPv4:
		case Line::ConfirmIPv4:
		case Line::IPv6:
		case Line::ConfirmIPv6:
			break;
	}

	return true;
}

//NOTE: a server found on the network is what a client most likely came for
void ServerScreen::PickFirst()
{
	const auto isFree = [](const network::FoundServer& server) { return server.freeSeats > 0; };
	const auto server{std::ranges::find_if(_servers, isFree)};
	if (server == _servers.end())
	{
		Pick(Item{.line = Line::IPv4});

		return;
	}

	Pick(Item{.line = Line::Server, .server = static_cast<std::size_t>(std::distance(_servers.begin(), server))});
}

void ServerScreen::Pick(const Item& item)
{
	_focus = item;
	_isColonAdded = false;
	_caretMoved = std::chrono::steady_clock::now();
	if (item.line == Line::Server)
	{
		//NOTE: scrolled just far enough to show the server picked
		_firstShown = std::min(_firstShown, item.server);
		while (ListRows(std::span{_servers}.subspan(_firstShown, item.server + 1 - _firstShown)) > kListRows)
		{
			++_firstShown;
		}
	}
}

//NOTE: round and round, past the servers scrolled off the list too
void ServerScreen::Step(const bool isForward)
{
	std::vector<Item> pickable{std::views::iota(std::size_t{}, _servers.size())
							   | std::views::transform([](const std::size_t server)
							   {
								   return Item{.line = Line::Server, .server = server};
							   })
							   | std::ranges::to<std::vector>()};
	const auto isNoServer = [](const Item& item) { return item.line != Line::Server; };
	std::ranges::copy_if(Lines(), std::back_inserter(pickable), isNoServer);
	std::erase_if(pickable, [this](const Item& item) { return !IsPickable(item); });

	const auto at{std::ranges::find(pickable, _focus)};
	const std::size_t index{at == pickable.end() ? 0 : static_cast<std::size_t>(std::distance(pickable.begin(), at))};
	const std::size_t count{pickable.size()};
	Pick(pickable[(index + (isForward ? 1 : count - 1)) % count]);
}

ServerScreen::Row* ServerScreen::FocusedRow()
{
	if (_focus.line == Line::IPv4)
	{
		return &_rows.front();
	}

	return _focus.line == Line::IPv6 ? &_rows.back() : nullptr;
}

//NOTE: the address goes to the row of its kind with the caret on the port; the other row keeps its text
void ServerScreen::Fill(const network::ServerAddress& address)
{
	const bool isIPv6{address.host.contains(':')};
	Pick(Item{.line = isIPv6 ? Line::IPv6 : Line::IPv4});
	Row& row{*FocusedRow()};
	if (isIPv6)
	{
		//NOTE: a scope id keeps its part when the address leaves it room
		const std::size_t percent{address.host.find('%')};
		const bool isScoped{percent != std::string::npos && address.host.size() <= kIPv6Length};
		row.parts = std::vector<std::string>(isScoped ? kIPv6Parts + 1 : kIPv6Parts);
		row.parts.front() = address.host.substr(0, percent);
		if (isScoped)
		{
			row.parts[kScopePart] = address.host.substr(percent + 1);
		}
	}
	else
	{
		std::ranges::fill(row.parts, std::string{});
		for (auto&& [part, octet]: std::views::zip(row.parts, address.host | std::views::split('.')))
		{
			part = std::ranges::to<std::string>(octet);
		}
	}

	row.parts.back() = address.port == network::kAnyFreePort ? std::string{} : std::to_string(address.port);
	row.StepToEnd(row.parts.size() - 1);
}

void ServerScreen::Type(const char symbol)
{
	Row* const focused{FocusedRow()};
	if (focused == nullptr)
	{
		return;
	}

	Row& row{*focused};
	if (row.part == row.parts.size() - 1)
	{
		TypePort(row, symbol);
	}
	else if (_focus.line == Line::IPv6 && row.part == 0)
	{
		TypeIPv6(row, symbol);
	}
	else if (_focus.line == Line::IPv6)
	{
		TypeScope(row, symbol);
	}
	else
	{
		TypeIPv4(row, symbol);
	}
}

//NOTE: a digit pushes the rest right, an octet past 255 handing its last one on; with no room it overwrites
void ServerScreen::TypeIPv4(Row& row, const char symbol)
{
	const std::size_t port{row.parts.size() - 1};
	//NOTE: a dot only out of a started octet, and never past the last one
	if (symbol == ':' || (symbol == '.' && row.offset > 0 && row.part + 1 < port))
	{
		row.StepToStart(symbol == ':' ? port : row.part + 1);

		return;
	}

	if (!IsDigit(symbol))
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

	Row flowed{row};
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
		row = std::move(flowed);
		handOn(row);

		return;
	}

	Row over{row};
	if (over.offset == over.parts[over.part].size())
	{
		over.StepToStart(over.part + 1);
	}

	if (over.part == port)
	{
		row = std::move(over);
		TypePort(row, symbol);

		return;
	}

	over.parts[over.part][over.offset++] = symbol;
	if (IsOctet(over.parts[over.part]))
	{
		row = std::move(over);
		handOn(row);
	}
}

//NOTE: a digit pushes the rest right, a group past four handing its last one on; with no room it overwrites
void ServerScreen::TypeIPv6(Row& row, const char symbol)
{
	const bool isAfterAddedColon{std::exchange(_isColonAdded, false)};
	const std::size_t port{row.parts.size() - 1};
	std::string& address{row.parts.front()};
	const bool isAtEnd{row.offset == address.size()};
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

			row.StepToStart(port);
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

		if (!IsScoped(row.parts) && address.size() + 1 < kIPv6Length)
		{
			row.parts.insert(std::prev(row.parts.end()), std::string{});
		}

		if (IsScoped(row.parts))
		{
			row.StepToEnd(kScopePart);
		}

		return;
	}

	//NOTE: a colon right of the caret is stepped over, and so is one typed right after the added one
	if (symbol == ':')
	{
		if (!isAtEnd && address[row.offset] == ':')
		{
			++row.offset;
		}
		else if (isAtEnd && IsWholeIPv6(address))
		{
			row.StepToStart(port);
		}
		else if (!isAfterAddedColon && address.size() < AddressRoom(row.parts))
		{
			address.insert(row.offset++, 1, ':');
		}

		return;
	}

	if (!IsHexDigit(symbol))
	{
		return;
	}

	std::string flowed{address};
	flowed.insert(row.offset, 1, symbol);
	if (const std::optional<std::size_t> typed{FlowIPv6(flowed, row.offset, AddressRoom(row.parts))})
	{
		address = std::move(flowed);
		row.offset = *typed + 1;
		CloseFullGroup(row);

		return;
	}

	//NOTE: past a whole address only the port is left
	if (isAtEnd)
	{
		if (IsWholeIPv6(address))
		{
			row.StepToStart(port);
			TypePort(row, symbol);
		}

		return;
	}

	const std::size_t next{address.find_first_not_of(':', row.offset)};
	if (next != std::string::npos)
	{
		address[next] = symbol;
		row.offset = next + 1;
		CloseFullGroup(row);
	}
}

//NOTE: Windows names an interface by its number; a colon or the closing bracket goes on to the port
void ServerScreen::TypeScope(Row& row, const char symbol)
{
	if (symbol == ':' || symbol == ']')
	{
		_isBracketed = _isBracketed || symbol == ']';
		row.StepToStart(row.parts.size() - 1);

		return;
	}

	if (IsDigit(symbol) && row.parts.front().size() < AddressRoom(row.parts))
	{
		row.parts[kScopePart].insert(row.offset++, 1, symbol);
	}
}

//NOTE: a digit pushes the rest right, or with no room goes over the digit right of the caret
void ServerScreen::TypePort(Row& row, const char symbol)
{
	if (!IsDigit(symbol))
	{
		return;
	}

	std::string& port{row.parts.back()};
	std::string typed{port};
	typed.insert(row.offset, 1, symbol);
	if (!IsPort(typed) && row.offset < port.size())
	{
		typed = port;
		typed[row.offset] = symbol;
	}

	if (IsPort(typed))
	{
		port = std::move(typed);
		++row.offset;
	}
}

//NOTE: a group filled at the end gets its colon, like a full octet; past a whole address only the port is left
void ServerScreen::CloseFullGroup(Row& row)
{
	std::string& address{row.parts.front()};
	if (row.offset < address.size() || GroupAt(address, row.offset).size() < kGroupDigits)
	{
		return;
	}

	if (IsWholeIPv6(address))
	{
		row.StepToStart(row.parts.size() - 1);
	}
	else if (address.size() < AddressRoom(row.parts))
	{
		address.push_back(':');
		++row.offset;
		_isColonAdded = true;
	}
}

//NOTE: at the start of a part it reaches back over the separator, at the start of IPv6 over the bracket
void ServerScreen::Erase(Row& row)
{
	if (_focus.line == Line::IPv6 && row.part == 0 && row.offset == 0)
	{
		_isBracketed = false;
	}

	//NOTE: right after the '%' of an empty scope id it is the '%' that goes
	if (row.part == kScopePart && IsScopeEmpty(row.parts))
	{
		row.parts.erase(std::next(row.parts.begin()));
		row.StepToEnd(0);

		return;
	}

	if (row.offset == 0 && row.part > 0)
	{
		row.StepToEnd(row.part - 1);
	}

	if (row.offset > 0)
	{
		std::string& part{row.parts[row.part]};
		--row.offset;
		if (IsKeptColon(part, row.offset))
		{
			--row.offset;
		}

		part.erase(row.offset, 1);
	}
}

//NOTE: at the end of a part it reaches over the separator into the next one
void ServerScreen::EraseRight(Row& row)
{
	if (row.part == 0 && row.offset == row.parts.front().size() && IsScopeEmpty(row.parts))
	{
		row.parts.erase(std::next(row.parts.begin()));

		return;
	}

	if (row.offset == row.parts[row.part].size() && row.part + 1 < row.parts.size())
	{
		row.StepToStart(row.part + 1);
	}

	std::string& part{row.parts[row.part]};
	if (row.offset < part.size())
	{
		if (IsKeptColon(part, row.offset))
		{
			++row.offset;
		}

		part.erase(row.offset, 1);
	}
}

//NOTE: a separator is stepped over like a symbol
void ServerScreen::CaretLeft(Row& row)
{
	if (row.offset > 0)
	{
		--row.offset;
	}
	else if (row.part > 0)
	{
		row.StepToEnd(row.part - 1);
	}
}

void ServerScreen::CaretRight(Row& row)
{
	if (row.offset < row.parts[row.part].size())
	{
		++row.offset;
	}
	else if (row.part + 1 < row.parts.size())
	{
		row.StepToStart(row.part + 1);
	}
}

//NOTE: to the start of the word, or of the previous one - octets, IPv6 groups and the port are words
void ServerScreen::WordLeft(Row& row)
{
	if (row.offset == 0 && row.part > 0)
	{
		row.StepToEnd(row.part - 1);
	}

	const std::string& part{row.parts[row.part]};
	if (_focus.line != Line::IPv6 || row.part != 0)
	{
		row.offset = 0;

		return;
	}

	while (row.offset > 0 && part[row.offset - 1] == ':')
	{
		--row.offset;
	}

	while (row.offset > 0 && part[row.offset - 1] != ':')
	{
		--row.offset;
	}
}

//NOTE: to the end of the word, or of the next one
void ServerScreen::WordRight(Row& row)
{
	const std::size_t port{row.parts.size() - 1};
	const std::string& part{row.parts[row.part]};
	if (_focus.line == Line::IPv6 && row.part == 0 && row.offset < part.size())
	{
		while (row.offset < part.size() && part[row.offset] == ':')
		{
			++row.offset;
		}

		while (row.offset < part.size() && part[row.offset] != ':')
		{
			++row.offset;
		}

		return;
	}

	if (row.offset < part.size())
	{
		row.offset = part.size();
	}
	else if (row.part < port)
	{
		row.StepToEnd(row.part + 1);
	}
}

std::string ServerScreen::Text(const Line line) const
{
	const Row& row{line == Line::IPv6 ? _rows.back() : _rows.front()};
	const std::string& port{row.parts.back()};

	std::string host{};
	if (line == Line::IPv6)
	{
		//NOTE: the colon a full group gets only leads on to the next one - the address ends before it
		std::string_view address{row.parts.front()};
		if (address.ends_with(':') && !address.ends_with("::"))
		{
			address.remove_suffix(1);
		}

		const bool hasScope{IsScoped(row.parts) && !row.parts[kScopePart].empty()};
		host = '[' + std::string{address} + (hasScope ? '%' + row.parts[kScopePart] : std::string{}) + ']';
	}
	else
	{
		//NOTE: an octet typed with leading zeros goes out as its number - the parser takes "010" for no octet
		const auto octet = [](const std::string& part)
		{
			return part.empty() ? part : std::to_string(std::stoi(part));
		};
		host = row.parts | std::views::take(kOctets) | std::views::transform(octet) | std::views::join_with('.')
			   | std::ranges::to<std::string>();
	}

	return port.empty() ? host : host + ':' + port;
}

ServerScreen::ShownRow ServerScreen::Shown(const Line line) const
{
	const Row& row{line == Line::IPv6 ? _rows.back() : _rows.front()};
	ShownRow shown{};
	const auto append = [this, &row, &shown, line](const std::size_t at, const std::string_view placeholder)
	{
		if (_focus.line == line && at == row.part)
		{
			shown.caret = shown.text.size() + row.offset;
		}

		const std::string& text{row.parts[at]};
		shown.text += text.empty() ? placeholder : text;
	};

	const std::size_t port{row.parts.size() - 1};
	if (line == Line::IPv6)
	{
		//NOTE: the brackets have their places either way, so showing them does not move the address
		shown.text += _isBracketed ? '[' : ' ';
		append(0, "____");
		if (IsScoped(row.parts))
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

void ServerScreen::Draw() const
{
	const std::vector<Item> lines{Lines()};
	UiTable picked{};
	std::optional<PanelCaret> caret{};
	for (const Item& item: lines)
	{
		switch (item.line)
		{
			case Line::ServersCaption:
				picked.rows.push_back(UiRow{.cells = {Centered("Server List:")}});
				break;
			case Line::ServersHeader:
				picked.rows.push_back(UiRow{.cells = {Word(std::string{kPlayersHeader} + "ADDRESS")}});
				break;
			case Line::NoServers:
				picked.rows.push_back(UiRow{.cells = {Centered(_scan->IsSearching() ? "SEARCHING..." : "NONE FOUND")}});
				break;
			case Line::Server:
			case Line::ServerTail:
			{
				const network::FoundServer& server{_servers[item.server]};
				const unsigned int color{server.freeSeats > 0 ? kTextColor : kFullServerColor};
				std::string text{ServerLines(server)[item.line == Line::Server ? 0 : 1]};
				picked.rows.push_back(UiRow{.cells = {Address(std::move(text), color)}});
				break;
			}
			case Line::Refresh:
				picked.rows.push_back(UiRow{.cells = {Word("REFRESH", kButtonColor)}});
				break;
			case Line::Gap:
				picked.rows.push_back(UiRow{});
				break;
			case Line::AddressCaption:
				picked.rows.push_back(UiRow{.cells = {Centered("Connect via IP:")}});
				break;
			case Line::IPv4Caption:
				picked.rows.push_back(UiRow{.cells = {Word("IPv4:")}});
				break;
			case Line::IPv6Caption:
				picked.rows.push_back(UiRow{.cells = {Word("IPv6:")}});
				break;
			case Line::IPv4:
			case Line::IPv6:
			{
				ShownRow shown{Shown(item.line)};
				if (shown.caret)
				{
					caret = PanelCaret{.row = picked.rows.size(),
									   .symbol = *shown.caret,
									   .alpha = CaretAlpha(std::chrono::steady_clock::now() - _caretMoved)};
				}

				picked.rows.push_back(UiRow{.cells = {Typed(std::move(shown.text))}});
				break;
			}
			case Line::ConfirmIPv4:
			case Line::ConfirmIPv6:
				picked.rows.push_back(UiRow{.cells = {Button(IsHosting(), item.line == Line::ConfirmIPv6)}});
				break;
			case Line::Error:
				picked.rows.push_back(_isRejected ? UiRow{.cells = {Word("NOT AN ADDRESS", kErrorColor)}} : UiRow{});
				break;
		}
	}

	std::vector<UiTable> tables{};
	tables.push_back(Caption(IsHosting() ? "PLAY AS HOST" : "PLAY AS CLIENT"));

	const std::size_t pickedTable{tables.size()};
	tables.push_back(std::move(picked));
	tables.push_back(Keys());

	//NOTE: sent for a short list too - the bar's room is kept, so the block does not move as servers come
	std::optional<PanelScroll> scroll{};
	if (!IsHosting())
	{
		const auto header{std::ranges::find(lines, Line::ServersHeader, &Item::line)};
		const auto shown{std::ranges::count(lines, Line::Server, &Item::line)};
		scroll = PanelScroll{.firstRow = static_cast<std::size_t>(std::distance(lines.begin(), header)) + 1,
							 .rowCount = kListRows,
							 .firstShown = _firstShown,
							 .shownCount = static_cast<std::size_t>(shown),
							 .total = _servers.size()};
	}

	const auto focus{std::ranges::find(lines, _focus)};
	_events->EmitEvent(RenderMenuBackgroundEvent{});
	_events->EmitEvent(RenderPanelTablesEvent{
			.tables = std::move(tables),
			.pickedTable = pickedTable,
			.selectedRow = static_cast<std::size_t>(std::distance(lines.begin(), focus)),
			.scroll = scroll,
			.caret = caret});
}
