#include "components/Menu.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/UiTable.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameMode.h"
#include "enums/UiIcon.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <utility>

namespace
{
constexpr unsigned int kTextColor{0xffffffffu};

//NOTE: the order the modes stand in - the row a click lands on is looked up here
constexpr std::array kModes{GameMode::OnePlayer, GameMode::TwoPlayers, GameMode::CoopWithBot,
							GameMode::PlayAsHost, GameMode::PlayAsClient};

UiCell Text(std::string text) { return UiCell{.text = std::move(text), .color = kTextColor}; }

UiCell Picture(const UiIcon icon) { return UiCell{.icon = icon, .color = kTextColor}; }
}//namespace

Menu::Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _pos{.x = 25, .y = 0}
	, _yOffsetStart{static_cast<int>(gameConfig.LogicalSize().y)}
	, _events{events}
	, _input{std::make_unique<InputProviderForMenu>(events, gameConfig)}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();
}

void Menu::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Menu::OnSelectedGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &Menu::OnMenuShowed));
}

void Menu::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }
void Menu::OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event) { _selectedGameMode = event.mode; }
void Menu::OnMenuShowed(const MenuShowedEvent& event) { DisplayMenu(event.isShown); }

void Menu::Draw()
{
	//NOTE: the opening slide - the menu scrolls up from below the screen to its resting place
	if (constexpr int yOffsetEnd = 0; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
		constexpr int padding{25};
		_pos.y = padding + _yOffsetStart;
	}

	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});
	_events->EmitEvent(RenderMenuEvent{.menuPos = _pos,
									   .selectedRow = SelectedRow(),
									   .modes = ModesTable(),
									   .controls = ControlsTable()});
}

UiTable Menu::ModesTable()
{
	return UiTable{.rows = {UiRow{.cells = {Text("ONE PLAYER")}},
							UiRow{.cells = {Text("TWO PLAYER")}},
							UiRow{.cells = {Text("COOP WITH BOT")}},
							UiRow{.cells = {Text("PLAY AS HOST")}},
							UiRow{.cells = {Text("PLAY AS CLIENT")}}}};
}

UiTable Menu::ControlsTable()
{
	return UiTable{.rows = {UiRow{.cells = {Text("Controls:"), Text("P1/P2"), Picture(UiIcon::XBoxHome),
											Text("XBox"), Picture(UiIcon::PS5Home), Text("PS")}},
							UiRow{.cells = {Text("Pause"), Text("P"), Picture(UiIcon::XBoxView), Text("View"),
											Picture(UiIcon::PS5Create), Text("Create")}},
							UiRow{.cells = {Text("Menu"), Text("M"), Picture(UiIcon::XBoxMenu), Text("Menu"),
											Picture(UiIcon::PS5Options), Text("Options")}},
							UiRow{.cells = {Text("Swap"), Text("TAB"), Picture(UiIcon::XBoxY), Text("Y"),
											Picture(UiIcon::PS5Triangle), Text("Triangle")}},
							UiRow{.cells = {Text("Move"), Text("Arrows/WASD"), Picture(UiIcon::XBoxDpad),
											Text("D-pad"), Picture(UiIcon::PS5Dpad), Text("D-pad")}},
							UiRow{.cells = {Text("Fire"), Text("Space/LCtrl"), Picture(UiIcon::XBoxA), Text("A"),
											Picture(UiIcon::PS5Cross), Text("Cross")}}}};
}

int Menu::SelectedRow() const
{
	const auto mode{std::ranges::find(kModes, _selectedGameMode)};

	return mode == kModes.end() ? 0 : static_cast<int>(std::distance(kModes.begin(), mode));
}

void Menu::DisplayMenu(const bool isDisplayed)
{
	_isMenuDisplayed = isDisplayed;

	if (_isMenuDisplayed)
	{
		_drawSub = _events->AddListener(this, &Menu::OnDrawUserInterface);
	}
	else
	{
		_drawSub = EventSubscription{};
	}
}
