#include "components/ServerScreen.h"
#include "components/AddressField.h"
#include "components/EventSystem.h"
#include "components/LevelRotation.h"
#include "components/MatchSettings.h"
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
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <array>
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
constexpr std::string_view kNotAnAddress{"NOT AN ADDRESS"};
constexpr std::string_view kNotThisMachine{"NOT THIS PC'S ADDRESS"};

//NOTE: rows kept for the list, so a found server does not push the address down
constexpr std::size_t kListRows{5};

//NOTE: as wide as the longest typed address, so it is drawn at the size of the rest
constexpr std::size_t kRowSymbols{AddressField::kLongestShown};
//NOTE: in the order of ServerScreen::Column
constexpr std::array<std::string_view, 3> kHeaders{"PLAYERS", "MODE", "ADDRESS"};
//NOTE: the players and the mode - "PLAYERS" with its sort arrow, or "CLASSIC", and a gap
constexpr std::size_t kColumnSymbols{9};
//NOTE: discovery is IPv4 only - the longest address a found server has
constexpr std::size_t kFoundAddressSymbols{std::string_view{"255.255.255.255:65535"}.size()};
static_assert(2 * kColumnSymbols + kFoundAddressSymbols <= kRowSymbols, "a found server has to fit in one row");
//NOTE: the up and down triangles in UTF-8 - spelled in bytes, so no compiler reads them in its own code page
constexpr std::string_view kAscending{"\xE2\x96\xB2"};
constexpr std::string_view kDescending{"\xE2\x96\xBC"};

//NOTE: a network match of one would be a local one
constexpr std::uint8_t kMinNetworkSeats{2u};
//NOTE: a setting's name, then its value between the arrows that say left and right turn it
constexpr std::size_t kSettingNameSymbols{10};

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

//NOTE: the IPv4 row gets spaces where the IPv6 row has its brackets, so both addresses start in one column
[[nodiscard]] ShownAddress Padded(ShownAddress shown, const bool isIPv4)
{
	if (!isIPv4)
	{
		return shown;
	}

	const std::size_t port{shown.text.rfind(':')};
	shown.text.insert(port, 1, ' ');
	shown.text.insert(0, 1, ' ');
	shown.caret = shown.caret.transform([port](const std::size_t symbol) { return symbol + (symbol > port ? 2 : 1); });

	return shown;
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

[[nodiscard]] UiTable Keys(const bool isHosting)
{
	UiTable keys{.rows = {UiRow{.cells = {Word("Choose"), Word("Up / Down")}},
						  UiRow{.cells = {Word("Caret"), Word("Left / Right, Space")}},
						  UiRow{.cells = {Word("Word"), Word("Ctrl+Left / Right, Tab")}},
						  UiRow{.cells = {Word("Port"), Word("optional, picked automatically")}},
						  UiRow{.cells = {Word("Paste"), Word("Ctrl+V")}},
						  UiRow{.cells = {Word("Confirm"), Word("Enter")}},
						  UiRow{.cells = {Word("Back"), Word("Esc")}}}};
	keys.rows.insert(std::next(keys.rows.begin()),
					 isHosting ? UiRow{.cells = {Word("Setting"), Word("Left / Right")}}
							   : UiRow{.cells = {Word("Sort"), Word("header, Left / Right, Enter")}});

	return keys;
}

[[nodiscard]] UiRow SettingRow(const std::string_view name, const std::string& value, const bool isChangeable)
{
	std::string text{name};
	text.resize(kSettingNameSymbols, ' ');
	if (!isChangeable)
	{
		return UiRow{.cells = {Word(text + value, kFullServerColor)}};
	}

	return UiRow{.cells = {Word(text + "< " + value + " >")}};
}

//NOTE: one step round - forward past the last value is the first one again
[[nodiscard]] std::size_t Turned(const std::size_t at, const std::size_t count, const bool isForward)
{
	return (at + (isForward ? 1u : count - 1u)) % count;
}

[[nodiscard]] int PlayersOf(const network::FoundServer& server) { return server.seats - server.freeSeats; }

//NOTE: by number, not by text - "10.0.0.9" comes before "10.0.0.10"
[[nodiscard]] std::pair<boost::asio::ip::address, std::uint16_t> AddressOrder(const network::FoundServer& server)
{
	boost::system::error_code ec;

	return {boost::asio::ip::make_address(server.address.host, ec), server.address.port};
}

[[nodiscard]] std::string ServerLine(const network::FoundServer& server)
{
	std::string seats{server.IsFull() ? "FULL"
									  : std::to_string(PlayersOf(server)) + '/' + std::to_string(server.seats)};
	seats.resize(kColumnSymbols, ' ');
	std::string mode{server.rules == MatchRules::FreeForAll ? "FFA" : "CLASSIC"};
	mode.resize(kColumnSymbols, ' ');
	const std::string& host{server.address.host};

	return seats + mode + (host.contains(':') ? '[' + host + ']' : host) + ':'
		   + std::to_string(server.address.port);
}

//NOTE: the players column's order - a server with someone waiting first, then an empty one, a full one last
[[nodiscard]] int JoinRank(const network::FoundServer& server)
{
	return server.IsFull() ? 2 : (server.freeSeats < server.seats ? 0 : 1);
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
	return {{.address = {.host = "255.255.255.255", .port = 65535}, .seats = 2, .freeSeats = 1},
			{.address = {.host = "172.16.254.1", .port = 65535}, .seats = 4, .freeSeats = 4, .rules = ffa},
			{.address = {.host = "192.168.0.10", .port = 50001}, .seats = 2, .freeSeats = 2},
			{.address = {.host = "255.255.255.254", .port = 65534}, .seats = 2, .freeSeats = 0},
			{.address = {.host = "10.20.30.40", .port = 65535}, .seats = 3, .freeSeats = 1, .rules = ffa},
			{.address = {.host = "255.255.255.253", .port = 65534}, .seats = 4, .freeSeats = 0, .rules = ffa},
			{.address = {.host = "10.0.0.6", .port = 4001}, .seats = 2, .freeSeats = 0},
			{.address = {.host = "192.168.100.200", .port = 5000}, .seats = 4, .freeSeats = 2},
			{.address = {.host = "192.168.0.14", .port = 50005}, .seats = 2, .freeSeats = 1}};
}
}//namespace

ServerScreen::ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address,
						   std::optional<std::string> lastHost)
	: _events{events}
	, _offeredIPv4{address}
	, _offeredIPv6{.host = network::LocalIPv6Address(), .port = address.port}
	, _lastHost{std::move(lastHost)}
{
	//NOTE: the IPv6 row offers this machine's own address, unless the given one is an IPv6 and takes it
	_ipv6.Fill(_offeredIPv6, false);
	Fill(address, false);

	_subs.push_back(_events->AddListener(this, &ServerScreen::OnMenuShown));
}

void ServerScreen::Open(const GameMode mode)
{
	_mode = mode;
	_rejection = {};
	_isConfirmHeld = false;
	_servers.clear();
	_firstShown = 0;

	if (IsHosting())
	{
		//NOTE: read again on every opening - a map dropped in since turns up without a restart
		_maps = LevelRotation{}.Names() | std::views::filter(IsMapName) | std::ranges::to<std::vector>();
		if (!_maps.empty() && !std::ranges::contains(_maps, _match.map))
		{
			_match.map = _maps.front();
		}
	}
	else
	{
		_scan = std::make_unique<network::DiscoveryScan>(network::LocalAddress());
	}

	OfferAddresses();
	//NOTE: the caret starts at the address, only a paste puts it on the port
	_ipv4.CaretToStart();
	_ipv6.CaretToStart();

	PickFirst();
	_servers = Ordered(FakeServers()); //TEMP

	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnPreTickUpdate));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextTyped));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextPasted));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnTextKey));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnRowClicked));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnRowHovered));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnEnter));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnCancelled));
	_openSubs.push_back(_events->AddListener(this, &ServerScreen::OnDrawUserInterface));
	//NOTE: a pad cannot type, but it can pick a server, turn a setting and confirm
	for (const InputChannel channel: kSlots | std::views::transform(LocalInput))
	{
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadUp));
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadDown));
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadLeft));
		_openSubs.push_back(_events->AddListener(Key(channel), this, &ServerScreen::OnPadRight));
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

void ServerScreen::OnPreTickUpdate(const PreTickUpdateEvent&)
{
	if (!_scan || !_scan->Poll())
	{
		return;
	}

	const bool wasEmpty{_servers.size() == FakeServers().size()}; //TEMP: _servers.empty()
	std::vector<network::FoundServer> servers{_scan->Servers()};
	std::ranges::copy(FakeServers(), std::back_inserter(servers)); //TEMP
	Arrange(std::move(servers));
	//NOTE: the first servers found take the focus they would have had at opening, unless it has moved since
	if (wasEmpty && _focus == Item{.line = Line::IPv4} && _ipv4.IsCaretAtStart())
	{
		PickFirst();
	}
}

void ServerScreen::OnTextTyped(const TextTypedEvent& event)
{
	std::ranges::for_each(event.text, [this](const char symbol) { Type(symbol); });
	_rejection = {};
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

	_rejection = {};
	_caretMoved = std::chrono::steady_clock::now();
}

void ServerScreen::OnTextKey(const TextKeyEvent& event)
{
	_rejection = {};
	_caretMoved = std::chrono::steady_clock::now();
	if (event.key == TextKey::NextField || event.key == TextKey::PreviousField)
	{
		Step(event.key == TextKey::NextField);

		return;
	}

	//NOTE: on a setting or the header the keys that move a caret turn it instead
	const bool isLeft{event.key == TextKey::CaretLeft || event.key == TextKey::WordLeft};
	const bool isRight{event.key == TextKey::CaretRight || event.key == TextKey::WordRight};
	if (IsTurnable(_focus.line) && (isLeft || isRight))
	{
		Change(isRight);

		return;
	}

	if (AddressField* const field{FocusedField()})
	{
		field->Press(event.key);
	}
}

//NOTE: a click on anything but an address row presses it at once - on the header, the column under it
void ServerScreen::OnRowClicked(const PanelRowClickedEvent& event)
{
	const std::optional<Item> item{ItemAt(event.row)};
	if (!item || !IsPickable(*item))
	{
		return;
	}

	if (item->line == Line::ServersHeader)
	{
		_column = ColumnAt(event.symbol);
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
	if (const std::optional<Item> item{ItemAt(event.row)}; item && IsPickable(*item) && *item != _focus)
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

void ServerScreen::OnPadLeft(const MoveLeftEvent& event)
{
	if (event.isPressed && IsTurnable(_focus.line))
	{
		Change(false);
	}
}

void ServerScreen::OnPadRight(const MoveRightEvent& event)
{
	if (event.isPressed && IsTurnable(_focus.line))
	{
		Change(true);
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
	if (_focus.line == Line::ServersHeader)
	{
		SortBy(_column);

		return;
	}

	if (IsTurnable(_focus.line))
	{
		Change(true);

		return;
	}

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
	if (!address)
	{
		_rejection = kNotAnAddress;

		return;
	}

	//NOTE: a server listens only on an address this machine has
	if (IsHosting() && !network::IsThisMachine(address->host))
	{
		_rejection = kNotThisMachine;

		return;
	}

	Choose(*address);
}

void ServerScreen::Choose(const network::ServerAddress& address)
{
	const GameMode mode{_mode};
	Close();
	_events->EmitEvent(ServerAddressChosenEvent{.mode = mode, .address = address, .match = _match});
}

bool ServerScreen::IsTurnable(const Line line) noexcept
{
	return line == Line::ServersHeader || line == Line::Rules || line == Line::Seats || line == Line::Map
		   || line == Line::Enemies || line == Line::Bots || line == Line::Start;
}

//NOTE: round and round, the way the list is stepped through
void ServerScreen::Change(const bool isForward)
{
	if (_focus.line == Line::ServersHeader)
	{
		_column = static_cast<Column>(Turned(static_cast<std::size_t>(_column), kHeaders.size(), isForward));
		_caretMoved = std::chrono::steady_clock::now();
	}
	else if (_focus.line == Line::Rules)
	{
		_match.rules = _match.rules == MatchRules::Classic ? MatchRules::FreeForAll : MatchRules::Classic;
	}
	else if (_focus.line == Line::Seats)
	{
		const std::size_t choices{kSeatCount - kMinNetworkSeats + 1u};
		const std::size_t at{static_cast<std::size_t>(_match.seats - kMinNetworkSeats)};
		_match.seats = static_cast<std::uint8_t>(kMinNetworkSeats + Turned(at, choices, isForward));
		_match.bots = std::min(_match.bots, MaxBots(_match.seats));
	}
	else if (_focus.line == Line::Map && !_maps.empty())
	{
		const auto at{static_cast<std::size_t>(std::distance(_maps.begin(), std::ranges::find(_maps, _match.map)))};
		_match.map = _maps[Turned(at % _maps.size(), _maps.size(), isForward)];
	}
	else if (_focus.line == Line::Enemies)
	{
		const std::size_t at{static_cast<std::size_t>(_match.enemiesAtOnce - 1u)};
		_match.enemiesAtOnce = static_cast<std::uint8_t>(1u + Turned(at, kMaxEnemiesAtOnce, isForward));
	}
	else if (_focus.line == Line::Bots)
	{
		_match.bots = static_cast<std::uint8_t>(Turned(_match.bots, MaxBots(_match.seats) + 1u, isForward));
	}
	else if (_focus.line == Line::Start)
	{
		_match.isStartingAtOnce = !_match.isStartingAtOnce;
	}
}

//NOTE: a second click on the same column turns the order round
void ServerScreen::SortBy(const Column column)
{
	const bool isDescending{_sort.column == column && !_sort.isDescending};
	_sort = Sort{.column = column, .isDescending = isDescending};
	Arrange(_servers);
}

ServerScreen::Column ServerScreen::ColumnAt(const std::size_t symbol) noexcept
{
	if (symbol < kColumnSymbols)
	{
		return Column::Players;
	}

	return symbol < 2 * kColumnSymbols ? Column::Mode : Column::Address;
}

//NOTE: the focus follows its server as the list shifts, and leaves one gone
void ServerScreen::Arrange(std::vector<network::FoundServer> servers)
{
	const std::optional<network::ServerAddress> focused{
			_focus.line == Line::Server ? std::optional{_servers[_focus.server].address} : std::nullopt};
	_servers = Ordered(std::move(servers));
	_firstShown = std::min(_firstShown, LastFirstShown(_servers.size()));
	if (!focused)
	{
		return;
	}

	const auto server{std::ranges::find(_servers, *focused, &network::FoundServer::address)};
	if (server == _servers.end())
	{
		PickFirst();

		return;
	}

	Pick(Item{.line = Line::Server, .server = static_cast<std::size_t>(std::distance(_servers.begin(), server))});
}

//NOTE: JoinRank first - a sort by another column keeps it among the servers it finds equal
std::vector<network::FoundServer> ServerScreen::Ordered(std::vector<network::FoundServer> servers) const
{
	std::ranges::stable_sort(servers, {}, JoinRank);
	const auto sort = [&servers, isDescending = _sort.isDescending](const auto key)
	{
		if (isDescending)
		{
			std::ranges::stable_sort(servers, std::ranges::greater{}, key);
		}
		else
		{
			std::ranges::stable_sort(servers, std::ranges::less{}, key);
		}
	};
	switch (_sort.column)
	{
		case Column::Players:
			sort(JoinRank);
			break;
		case Column::Mode:
			sort(&network::FoundServer::rules);
			break;
		case Column::Address:
			sort(AddressOrder);
			break;
	}

	return servers;
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
	else
	{
		lines.push_back(Item{.line = Line::Rules});
		lines.push_back(Item{.line = Line::Seats});
		lines.push_back(Item{.line = Line::Map});
		lines.push_back(Item{.line = Line::Enemies});
		lines.push_back(Item{.line = Line::Bots});
		lines.push_back(Item{.line = Line::Start});
		lines.push_back(Item{.line = Line::Gap});
	}

	lines.push_back(Item{.line = Line::IPv4});
	lines.push_back(Item{.line = Line::ConfirmIPv4});
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
		case Line::NoServers:
		case Line::Gap:
		case Line::AddressCaption:
		case Line::Error:
			return false;
		case Line::Server:
			return item.server < _servers.size();
		case Line::Enemies:
			return _match.rules == MatchRules::Classic;
		case Line::ServersHeader:
		case Line::Rules:
		case Line::Seats:
		case Line::Map:
		case Line::Bots:
		case Line::Start:
		case Line::Refresh:
		case Line::IPv4:
		case Line::ConfirmIPv4:
		case Line::IPv6:
		case Line::ConfirmIPv6:
			break;
	}

	return true;
}

std::optional<ServerScreen::Item> ServerScreen::ItemAt(const std::size_t row) const
{
	const std::vector<Item> lines{Lines()};

	return row < lines.size() ? std::optional{lines[row]} : std::nullopt;
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
	std::vector<Item> pickable{Lines()};
	std::erase_if(pickable, [this](const Item& item) { return item.line == Line::Server || !IsPickable(item); });
	const auto servers{std::views::iota(std::size_t{}, _servers.size())
					   | std::views::transform([](const std::size_t server)
					   {
						   return Item{.line = Line::Server, .server = server};
					   })};
	//NOTE: every server under the header, the shown ones and the scrolled off alike
	const auto header{std::ranges::find(pickable, Line::ServersHeader, &Item::line)};
	if (header != pickable.end())
	{
		pickable.insert(std::next(header), servers.begin(), servers.end());
	}

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

//NOTE: the network may have changed since the rows were given their addresses. A client is offered the host it
//joined last, in the row of its family; a server listens only on this machine, so it is offered that
void ServerScreen::OfferAddresses()
{
	const auto offer = [](AddressField& field, network::ServerAddress& offered, std::string host)
	{
		if (network::ParseServerAddress(field.Text()) != offered)
		{
			return;
		}

		offered.host = std::move(host);
		field.Fill(offered, false);
	};
	const auto lastHostOr = [this](const bool isIPv6, std::string local)
	{
		const bool isOffered{!IsHosting() && _lastHost && _lastHost->contains(':') == isIPv6};

		return isOffered ? *_lastHost : std::move(local);
	};
	offer(_ipv4, _offeredIPv4, lastHostOr(false, network::LocalAddress()));
	offer(_ipv6, _offeredIPv6, lastHostOr(true, network::LocalIPv6Address()));
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
			return UiRow{.cells = {Word(Header())}};
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
		case Line::IPv4:
		case Line::IPv6:
			return UiRow{.cells = {Address(Padded(FieldOf(item.line).Shown(false), item.line == Line::IPv4).text)}};
		case Line::ConfirmIPv4:
		case Line::ConfirmIPv6:
			return UiRow{.cells = {Button(IsHosting(), item.line == Line::ConfirmIPv6)}};
		case Line::Rules:
			return SettingRow("RULES", _match.rules == MatchRules::FreeForAll ? "FFA" : "CLASSIC", true);
		case Line::Seats:
			return SettingRow("SEATS", std::to_string(_match.seats), true);
		case Line::Map:
			return SettingRow("MAP", _match.map, true);
		case Line::Enemies:
			return IsPickable(item) ? SettingRow("ENEMIES", std::to_string(_match.enemiesAtOnce), true)
									: SettingRow("ENEMIES", std::to_string(kFreeForAllBots), false);
		case Line::Bots:
			return SettingRow("BOTS", std::to_string(_match.bots), true);
		case Line::Start:
			return SettingRow("START", _match.isStartingAtOnce ? "AT ONCE" : "WHEN FULL", true);
		case Line::Error:
			break;
	}

	return _rejection.empty() ? UiRow{} : UiRow{.cells = {Word(std::string{_rejection}, kErrorColor)}};
}

//NOTE: the column the list is sorted by carries an arrow - up for ascending, as file lists have it
std::string ServerScreen::Header() const
{
	const auto title = [this](const Column column)
	{
		const std::string_view word{kHeaders[static_cast<std::size_t>(column)]};
		const bool isSorted{_sort.column == column};
		std::string text{word};
		text += isSorted ? (_sort.isDescending ? kDescending : kAscending) : std::string_view{};
		//NOTE: the arrow is three bytes and one symbol
		text.append(kColumnSymbols - word.size() - (isSorted ? 1 : 0), ' ');

		return text;
	};

	return title(Column::Players) + title(Column::Mode) + title(Column::Address);
}

//NOTE: where Left and Right act - a typed line's caret, or the header's column underlined
std::optional<PanelCaret> ServerScreen::Caret(const std::size_t row) const
{
	const std::uint8_t alpha{CaretAlpha(std::chrono::steady_clock::now() - _caretMoved)};
	if (_focus.line == Line::ServersHeader)
	{
		const auto column{static_cast<std::size_t>(_column)};

		return PanelCaret{.row = row,
						  .symbol = column * kColumnSymbols,
						  .symbols = kHeaders[column].size(),
						  .alpha = alpha};
	}

	if (_focus.line != Line::IPv4 && _focus.line != Line::IPv6)
	{
		return std::nullopt;
	}

	const ShownAddress shown{Padded(FieldOf(_focus.line).Shown(true), _focus.line == Line::IPv4)};

	return shown.caret.transform([row, alpha](const std::size_t symbol)
	{
		return PanelCaret{.row = row, .symbol = symbol, .alpha = alpha};
	});
}

void ServerScreen::Draw() const
{
	const std::vector<Item> lines{Lines()};
	UiTable picked{.rows = lines | std::views::transform([this](const Item& item) { return LineRow(item); })
						   | std::ranges::to<std::vector>()};

	//NOTE: a row a line, so the focused line's place in the lines is its row
	const auto focus{std::ranges::find(lines, _focus)};
	const auto focusRow{static_cast<std::size_t>(std::distance(lines.begin(), focus))};

	std::vector<UiTable> tables{};
	tables.push_back(Caption(IsHosting() ? "PLAY AS HOST" : "PLAY AS CLIENT"));

	const std::size_t pickedTable{tables.size()};
	tables.push_back(std::move(picked));
	tables.push_back(Keys(IsHosting()));

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
							  .caret = Caret(focusRow)}});
}
