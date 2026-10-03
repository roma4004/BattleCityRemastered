#pragma once

#include "components/AddressField.h"
#include "components/EventSystem.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

enum class GameMode : char8_t;
struct DrawUserInterfaceEvent;
struct EnterEvent;
struct FireEvent;
struct MenuShownEvent;
struct MoveDownEvent;
struct MoveUpEvent;
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
		AddressCaption,
		IPv4Caption,
		IPv4,
		ConfirmIPv4,
		IPv6Caption,
		IPv6,
		ConfirmIPv6,
		Error
	};

	struct Item
	{
		Line line{};
		//NOTE: an index into _servers, on a server line
		std::size_t server{};

		[[nodiscard]] bool operator==(const Item& rhs) const = default;
	};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	//NOTE: held only while the screen is up - clearing them closes it
	std::vector<EventSubscription> _openSubs{};
	//NOTE: only while a client has the screen up
	std::unique_ptr<network::DiscoveryScan> _scan{nullptr};
	std::vector<network::FoundServer> _servers{};
	std::size_t _firstShown{};

	AddressField _ipv4{AddressFamily::IPv4};
	AddressField _ipv6{AddressFamily::IPv6};
	Item _focus{};
	GameMode _mode{};
	bool _isRejected{};
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
	void OnCancelled(const TextInputCancelledEvent&);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;

	[[nodiscard]] bool IsOpen() const noexcept;
	[[nodiscard]] bool IsHosting() const noexcept;
	void Close();
	void Confirm(bool isPressed);
	void Press();
	void Choose(const network::ServerAddress& address);

	[[nodiscard]] std::vector<Item> Lines() const;
	[[nodiscard]] bool IsPickable(const Item& item) const;
	[[nodiscard]] std::optional<Item> PickableAt(std::size_t row) const;
	void PickFirst();
	void Pick(const Item& item);
	void Step(bool isForward);

	[[nodiscard]] AddressField* FocusedField();
	//NOTE: the row a typed line or its button belongs to
	[[nodiscard]] const AddressField& FieldOf(Line line) const;
	void Fill(const network::ServerAddress& address, bool isBracketed);
	void Type(char symbol);
	//NOTE: one table row a line - a click on row N is a click on Lines()[N]
	[[nodiscard]] UiRow LineRow(const Item& item) const;
	void Draw() const;

public:
	ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address);

	void Open(GameMode mode);
};
