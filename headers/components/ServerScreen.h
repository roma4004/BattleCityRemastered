#pragma once

#include "components/AddressField.h"
#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include "network/PublicAddressProbe.h"
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class GameMode : char8_t;
struct DrawUserInterfaceEvent;
struct EnterEvent;
struct FireEvent;
struct MenuShownEvent;
struct MoveDownEvent;
struct MoveLeftEvent;
struct MoveRightEvent;
struct MoveUpEvent;
struct PanelCaret;
struct PanelDropDown;
struct PanelRowClickedEvent;
struct PanelRowHoveredEvent;
struct PreTickUpdateEvent;
struct TextInputCancelledEvent;
struct TextKeyEvent;
struct TextPastedEvent;
struct TextTypedEvent;
struct UiRow;
class EventSystem;

//NOTE: where a network game is created or joined; what is typed outlives the screen
class ServerScreen final
{
	enum class Line : char8_t
	{
		ServersCaption,
		ServersHeader,
		NoServers,
		Server,
		Refresh,
		Gap,
		Rules,
		Seats,
		Map,
		Enemies,
		Bots,
		Start,
		AddressCaption,
		IPv4,
		ConfirmIPv4,
		IPv6,
		ConfirmIPv6,
		Error
	};

	enum class Column : char8_t
	{
		Players,
		Mode,
		Address
	};

	struct Item
	{
		Line line{};
		//NOTE: an index into _servers, on a server line
		std::size_t server{};

		[[nodiscard]] bool operator==(const Item& rhs) const = default;
	};

	//NOTE: the order the header asks for
	struct Sort
	{
		Column column{};
		bool isDescending{};
	};

	//NOTE: a host row's list - its family's addresses this machine has, and the one the bar is on
	struct DropDown
	{
		std::vector<network::OwnAddress> choices{};
		std::size_t picked{};
	};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	//NOTE: held only while the screen is up - clearing them closes it
	std::vector<EventSubscription> _openSubs{};
	//NOTE: only while a client has the screen up
	std::unique_ptr<network::DiscoveryScan> _scan{nullptr};
	std::vector<network::FoundServer> _servers{};
	std::size_t _firstShown{};
	//NOTE: by the players at first; kept past closing, as what is typed is
	Sort _sort{};
	//NOTE: the header column Left and Right move along, sorted by on Enter
	Column _column{};

	//NOTE: what CREATE ON starts the server for - kept past closing, as what is typed is
	MatchSettings _match{};
	//NOTE: the maps folder, read on opening
	std::vector<std::string> _maps{};
	AddressField _ipv4{AddressFamily::IPv4};
	AddressField _ipv6{AddressFamily::IPv6};
	AddressField _ownIPv4{AddressFamily::IPv4, AddressInput::Picked};
	AddressField _ownIPv6{AddressFamily::IPv6, AddressInput::Picked};
	//NOTE: only while the focused host row has its list down
	std::optional<DropDown> _dropDown{};
	//NOTE: what the rows were given - a row still holding it gets the current offer on opening
	network::ServerAddress _offeredIPv4{};
	network::ServerAddress _offeredIPv6{};
	//NOTE: the host this game last joined - a client is offered it in place of this machine
	std::optional<std::string> _lastHost{};
	//NOTE: what the internet sees this machine as - asked while hosting, until somebody answers
	std::unique_ptr<network::PublicAddressProbe> _publicProbe{nullptr};
	//NOTE: the router's address on the internet - the host's IPv4 list offers it once known
	std::optional<std::string> _publicHost{};
	Item _focus{};
	GameMode _mode{};
	//NOTE: why the last confirm went nowhere - empty when it did not
	std::string_view _rejection{};
	//NOTE: the caret blinks from here - typing or moving it shows it whole
	std::chrono::steady_clock::time_point _caretMoved{};
	//NOTE: a release confirms only a press made while the screen was up, not the one that opened it
	bool _isConfirmHeld{};

	void OnMenuShown(const MenuShownEvent& event);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnTextTyped(const TextTypedEvent& event);
	void OnTextPasted(const TextPastedEvent& event);
	void OnTextKey(const TextKeyEvent& event);
	void OnRowClicked(const PanelRowClickedEvent& event);
	void OnRowHovered(const PanelRowHoveredEvent& event);
	void OnEnter(const EnterEvent& event);
	void OnFire(const FireEvent& event);
	void OnPadUp(const MoveUpEvent& event);
	void OnPadDown(const MoveDownEvent& event);
	void OnPadLeft(const MoveLeftEvent& event);
	void OnPadRight(const MoveRightEvent& event);
	void OnCancelled(const TextInputCancelledEvent&);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;

	[[nodiscard]] bool IsOpen() const noexcept;
	[[nodiscard]] bool IsHosting() const noexcept;
	void Close();
	void Confirm(bool isPressed);
	void Press();
	void Choose(const network::ServerAddress& address, bool isPortForwarded);
	void PollPublicAddress();
	//NOTE: the router's address, which no interface has
	[[nodiscard]] bool IsPublic(std::string_view host) const;

	//NOTE: Left and Right turn a setting's value and the header's column, not a caret
	[[nodiscard]] static bool IsTurnable(Line line) noexcept;
	void Change(bool isForward);

	void SortBy(Column column);
	[[nodiscard]] static Column ColumnAt(std::size_t symbol) noexcept;
	void Arrange(std::vector<network::FoundServer> servers);
	[[nodiscard]] std::vector<network::FoundServer> Ordered(std::vector<network::FoundServer> servers) const;

	[[nodiscard]] std::vector<Item> Lines() const;
	[[nodiscard]] bool IsPickable(const Item& item) const;
	[[nodiscard]] std::optional<Item> ItemAt(std::size_t row) const;
	void PickFirst();
	void Pick(const Item& item);
	void Step(bool isForward);

	[[nodiscard]] AddressField* FocusedField();
	//NOTE: the row a typed line or its button belongs to
	[[nodiscard]] const AddressField& FieldOf(Line line) const;
	void Fill(const network::ServerAddress& address, bool isBracketed);
	void OfferAddresses();
	void OfferOwnAddresses();
	void OpenDropDown();
	void PickOwn(std::size_t choice);
	[[nodiscard]] bool IsPortAt(std::size_t symbol) const;
	void Type(char symbol);
	//NOTE: one table row a line - a click on row N is a click on Lines()[N]
	[[nodiscard]] UiRow LineRow(const Item& item) const;
	[[nodiscard]] std::string Header() const;
	[[nodiscard]] std::optional<PanelCaret> Caret(std::size_t row) const;
	[[nodiscard]] std::optional<PanelDropDown> DropDownAt(std::size_t row) const;
	void Draw() const;

public:
	ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address,
				 std::optional<std::string> lastHost);

	void Open(GameMode mode);
	void RememberHost(std::string host) { _lastHost = std::move(host); }
};
