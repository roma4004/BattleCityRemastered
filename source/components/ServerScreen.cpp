#include "components/ServerScreen.h"
#include "components/AddressField.h"
#include "components/EventSystem.h"
#include "components/UiTable.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <ranges>
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
constexpr std::size_t kListRows{5};

//NOTE: a little wider than the longest hint
constexpr std::size_t kRowSymbols{40};
constexpr std::string_view kPlayersHeader{"PLAYERS  "};
constexpr std::string_view kModeHeader{"MODE     "};
//NOTE: discovery is IPv4 only - the longest address a found server has
constexpr std::size_t kFoundAddressSymbols{std::string_view{"255.255.255.255:65535"}.size()};
static_assert(kPlayersHeader.size() + kModeHeader.size() + kFoundAddressSymbols <= kRowSymbols,
			  "a found server has to fit in one row");

//NOTE: a terminal caret's period, fading instead of switching
constexpr auto kCaretBlink{1060ms};

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
	return UiCell{.text = std::move(text),
				  .color = kTextColor,
				  .symbols = kRowSymbols,
				  .fitSymbols = AddressField::kLongestShown};
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
	return UiTable{.rows = {UiRow{.cells = {Word("Choose"), Word("Up / Down")}},
							UiRow{.cells = {Word("Caret"), Word("Left / Right, Space")}},
							UiRow{.cells = {Word("Word"), Word("Ctrl+Left / Right, Tab")}},
							UiRow{.cells = {Word("Port"), Word("optional, picked automatically")}},
							UiRow{.cells = {Word("Paste"), Word("Ctrl+V")}},
							UiRow{.cells = {Word("Confirm"), Word("Enter")}},
							UiRow{.cells = {Word("Back"), Word("Esc")}}}};
}

[[nodiscard]] std::string ServerLine(const network::FoundServer& server)
{
	const int players{server.seats - server.freeSeats};
	std::string seats{server.IsFull() ? "FULL" : std::to_string(players) + '/' + std::to_string(server.seats)};
	seats.resize(kPlayersHeader.size(), ' ');
	std::string mode{server.rules == MatchRules::FreeForAll ? "FFA" : "CLASSIC"};
	mode.resize(kModeHeader.size(), ' ');
	const std::string& host{server.address.host};

	return seats + mode + (host.contains(':') ? '[' + host + ']' : host) + ':'
		   + std::to_string(server.address.port);
}

//NOTE: a server with someone waiting first, then an empty one, a full one last
[[nodiscard]] std::vector<network::FoundServer> JoinOrder(std::vector<network::FoundServer> servers)
{
	const auto rank = [](const network::FoundServer& server)
	{
		return server.IsFull() ? 2 : (server.freeSeats < server.seats ? 0 : 1);
	};
	std::ranges::stable_sort(servers, {}, rank);

	return servers;
}

//NOTE: as far as the list scrolls - from there its last servers fill the rows
[[nodiscard]] constexpr std::size_t LastFirstShown(const std::size_t servers) noexcept
{
	return servers - std::min(servers, kListRows);
}
//TEMP: fake servers for a visual check of the scrolled list, after the real ones - not for commit
std::vector<network::FoundServer> FakeServers()
{
	constexpr MatchRules ffa{MatchRules::FreeForAll};
	return JoinOrder({{.address = {.host = "255.255.255.255", .port = 65535}, .seats = 2, .freeSeats = 1},
					  {.address = {.host = "172.16.254.1", .port = 65535}, .seats = 4, .freeSeats = 4, .rules = ffa},
					  {.address = {.host = "192.168.0.10", .port = 50001}, .seats = 2, .freeSeats = 2},
					  {.address = {.host = "255.255.255.254", .port = 65534}, .seats = 2, .freeSeats = 0},
					  {.address = {.host = "10.20.30.40", .port = 65535}, .seats = 3, .freeSeats = 1, .rules = ffa},
					  {.address = {.host = "255.255.255.253", .port = 65534}, .seats = 4, .freeSeats = 0, .rules = ffa},
					  {.address = {.host = "10.0.0.6", .port = 4001}, .seats = 2, .freeSeats = 0},
					  {.address = {.host = "192.168.100.200", .port = 5000}, .seats = 4, .freeSeats = 2},
					  {.address = {.host = "192.168.0.14", .port = 50005}, .seats = 2, .freeSeats = 1}});
}
}//namespace

ServerScreen::ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address)
	: _events{events}
{
	Fill(address, false);
	_ipv6.Fill(network::ServerAddress{.host = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", .port = 65535}, false); //TEMP

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
	}

	//NOTE: the caret starts at the address, only a paste puts it on the port
	_ipv4.CaretToStart();
	_ipv6.CaretToStart();

	PickFirst();
	std::ranges::copy(FakeServers(), std::back_inserter(_servers)); //TEMP

	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnPreTickUpdate));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextTyped));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextPasted));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextKey));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnRowClicked));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnRowHovered));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnEnter));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnCancelled));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnDrawUserInterface));
	//NOTE: a pad cannot type, but it can pick a server and confirm
	for (const InputChannel channel: kSlots | std::views::transform(LocalInput))
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

	const std::optional<network::ServerAddress> focused{
			_focus.line == Line::Server ? std::optional{_servers[_focus.server].address} : std::nullopt};
	_servers.resize(_servers.size() - FakeServers().size()); //TEMP
	const bool wasEmpty{_servers.empty()};
	_servers = JoinOrder(_scan->Servers());
	std::ranges::copy(FakeServers(), std::back_inserter(_servers)); //TEMP
	_firstShown = std::min(_firstShown, LastFirstShown(_servers.size()));
	if (!focused)
	{
		//NOTE: the first servers found take the focus they would have had at opening, unless it has moved since
		if (wasEmpty && _focus == Item{.line = Line::IPv4} && _ipv4.IsCaretAtStart())
		{
			PickFirst();
		}

		return;
	}

	const auto server{std::ranges::find(_servers, *focused, &network::FoundServer::address)};
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
		Fill(*address, text.starts_with('['));
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
	_caretMoved = std::chrono::steady_clock::now();
	if (event.key == TextKey::NextField || event.key == TextKey::PreviousField)
	{
		Step(event.key == TextKey::NextField);

		return;
	}

	if (AddressField* const field{FocusedField()})
	{
		field->Press(event.key);
	}
}

//NOTE: a click on anything but an address row presses it at once
void ServerScreen::OnRowClicked(const PanelRowClickedEvent& event)
{
	const std::optional<Item> item{PickableAt(event.row)};
	if (!item)
	{
		return;
	}

	Pick(*item);
	if (_focus.line != Line::IPv4 && _focus.line != Line::IPv6)
	{
		Press();
	}
}

//NOTE: the pointer picks what it passes over, as in the menu - only a click presses
void ServerScreen::OnRowHovered(const PanelRowHoveredEvent& event)
{
	if (const std::optional<Item> item{PickableAt(event.row)}; item && *item != _focus)
	{
		Pick(*item);
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
		if (IsPickable(_focus) && !_servers[_focus.server].IsFull())
		{
			Choose(_servers[_focus.server].address);
		}

		return;
	}

	const std::optional<network::ServerAddress> address{network::ParseServerAddress(FieldOf(_focus.line).Text())};
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
		for (std::size_t server{_firstShown}; server < std::min(_servers.size(), _firstShown + kListRows); ++server)
		{
			lines.push_back(Item{.line = Line::Server, .server = server});
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

std::optional<ServerScreen::Item> ServerScreen::PickableAt(const std::size_t row) const
{
	const std::vector<Item> lines{Lines()};
	if (row >= lines.size())
	{
		return std::nullopt;
	}

	const Item& item{lines[row]};

	return IsPickable(item) ? std::optional{item} : std::nullopt;
}

//NOTE: a server found on the network is what a client most likely came for
void ServerScreen::PickFirst()
{
	const auto server{std::ranges::find(_servers, false, &network::FoundServer::IsFull)};
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
	if (AddressField* const field{FocusedField()})
	{
		field->Focus();
	}

	_caretMoved = std::chrono::steady_clock::now();
	if (item.line == Line::Server)
	{
		//NOTE: scrolled just far enough to show the server picked
		_firstShown = std::min(_firstShown, item.server);
		if (item.server >= _firstShown + kListRows)
		{
			_firstShown = item.server + 1 - kListRows;
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

AddressField* ServerScreen::FocusedField()
{
	if (_focus.line == Line::IPv4)
	{
		return &_ipv4;
	}

	return _focus.line == Line::IPv6 ? &_ipv6 : nullptr;
}

//NOTE: the address goes to the row of its kind with the caret on the port; the other row keeps its text
void ServerScreen::Fill(const network::ServerAddress& address, const bool isBracketed)
{
	Pick(Item{.line = address.host.contains(':') ? Line::IPv6 : Line::IPv4});
	FocusedField()->Fill(address, isBracketed);
}

void ServerScreen::Type(const char symbol)
{
	if (AddressField* const field{FocusedField()})
	{
		field->Type(symbol);
	}
}

const AddressField& ServerScreen::FieldOf(const Line line) const
{
	return line == Line::IPv6 || line == Line::ConfirmIPv6 ? _ipv6 : _ipv4;
}

UiRow ServerScreen::LineRow(const Item& item) const
{
	switch (item.line)
	{
		case Line::ServersCaption:
			return UiRow{.cells = {Centered("Server List:")}};
		case Line::ServersHeader:
			return UiRow{.cells = {Word(std::string{kPlayersHeader} + std::string{kModeHeader} + "ADDRESS")}};
		case Line::NoServers:
			return UiRow{.cells = {Centered(_scan->IsSearching() ? "SEARCHING..." : "NONE FOUND")}};
		case Line::Server:
		{
			const network::FoundServer& server{_servers[item.server]};

			return UiRow{.cells = {Address(ServerLine(server), server.IsFull() ? kFullServerColor : kTextColor)}};
		}
		case Line::Refresh:
			return UiRow{.cells = {Word("REFRESH", kButtonColor)}};
		case Line::Gap:
			return UiRow{};
		case Line::AddressCaption:
			return UiRow{.cells = {Centered("Connect via IP:")}};
		case Line::IPv4Caption:
			return UiRow{.cells = {Word("IPv4:")}};
		case Line::IPv6Caption:
			return UiRow{.cells = {Word("IPv6:")}};
		case Line::IPv4:
		case Line::IPv6:
			return UiRow{.cells = {Typed(FieldOf(item.line).Shown(false).text)}};
		case Line::ConfirmIPv4:
		case Line::ConfirmIPv6:
			return UiRow{.cells = {Button(IsHosting(), item.line == Line::ConfirmIPv6)}};
		case Line::Error:
			break;
	}

	return _isRejected ? UiRow{.cells = {Word("NOT AN ADDRESS", kErrorColor)}} : UiRow{};
}

void ServerScreen::Draw() const
{
	const std::vector<Item> lines{Lines()};
	UiTable picked{.rows = lines | std::views::transform([this](const Item& item) { return LineRow(item); })
						   | std::ranges::to<std::vector>()};

	//NOTE: a row a line, so the focused line's place in the lines is its row
	const auto focus{std::ranges::find(lines, _focus)};
	const auto focusRow{static_cast<std::size_t>(std::distance(lines.begin(), focus))};
	const bool isTyping{_focus.line == Line::IPv4 || _focus.line == Line::IPv6};
	const std::optional<std::size_t> symbol{isTyping ? FieldOf(_focus.line).Shown(true).caret : std::nullopt};
	const std::optional<PanelCaret> caret{symbol.transform([this, focusRow](const std::size_t at)
	{
		return PanelCaret{.row = focusRow,
						  .symbol = at,
						  .alpha = CaretAlpha(std::chrono::steady_clock::now() - _caretMoved)};
	})};

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

	_events->EmitEvent(RenderMenuBackgroundEvent{});
	_events->EmitEvent(RenderPanelTablesEvent{
			.tables = std::move(tables),
			.pick = PanelPick{.table = pickedTable,
							  .selectedRow = focusRow,
							  .scroll = scroll,
							  .caret = caret}});
}
