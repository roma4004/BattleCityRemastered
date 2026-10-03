#pragma once

#include "components/EventSystem.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
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
class EventSystem;

//NOTE: where a network game is created or joined; what is typed outlives the screen
class ServerScreen final
{
	//NOTE: four octets and a port, or an IPv6 address and a port - the separators between parts are drawn
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

	struct ShownRow
	{
		std::string text{};
		std::optional<std::size_t> caret{};
	};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	//NOTE: held only while the screen is up - clearing them closes it
	std::vector<EventSubscription> _openSubs{};
	//NOTE: only while a client has the screen up
	std::unique_ptr<network::DiscoveryScan> _scan{nullptr};
	std::vector<network::FoundServer> _servers{};
	std::size_t _firstShown{};

	std::array<Row, 2> _rows{};
	Item _focus{};
	GameMode _mode{};
	bool _isRejected{};
	//NOTE: the IPv6 row's last colon closed a full group - a colon typed next is the same one
	bool _isColonAdded{};
	//NOTE: only drawn - the text never holds the brackets
	bool _isBracketed{};
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

	[[nodiscard]] Row* FocusedRow();
	void Fill(const network::ServerAddress& address);
	void Type(char symbol);
	static void TypeIPv4(Row& row, char symbol);
	void TypeIPv6(Row& row, char symbol);
	void TypeScope(Row& row, char symbol);
	static void TypePort(Row& row, char symbol);
	void CloseFullGroup(Row& row);
	void Erase(Row& row);
	static void EraseRight(Row& row);
	static void CaretLeft(Row& row);
	static void CaretRight(Row& row);
	void WordLeft(Row& row) const;
	void WordRight(Row& row) const;

	//NOTE: the row as the parser reads it, and as drawn - with placeholders
	[[nodiscard]] std::string Text(Line line) const;
	[[nodiscard]] ShownRow Shown(Line line) const;
	void Draw() const;

public:
	ServerScreen(const std::shared_ptr<EventSystem>& events, const network::ServerAddress& address);

	void Open(GameMode mode);
};
