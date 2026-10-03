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
constexpr std::array kModes{GameMode::OnePlayer,
							GameMode::TwoPlayers,
							GameMode::CoopWithBot,
							GameMode::FreeForAll,
							GameMode::TwoPlayersFreeForAll,
							GameMode::PlayAsHost,
							GameMode::PlayAsClient};

UiCell Text(std::string text) { return TextCell(std::move(text), kTextColor); }

UiCell Picture(const UiIcon icon) { return UiCell{.icon = icon, .color = kTextColor}; }

UiTable TitleTable() { return UiTable{.rows = {UiRow{.cells = {Picture(UiIcon::MenuLogo)}}}}; }

UiTable ModesTable()
{
	return UiTable{.rows = {UiRow{.cells = {Text("ONE PLAYER")}},
							UiRow{.cells = {Text("TWO PLAYER")}},
							UiRow{.cells = {Text("COOP WITH BOT")}},
							UiRow{.cells = {Text("FREE FOR ALL")}},
							UiRow{.cells = {Text("2P FREE FOR ALL")}},
							UiRow{.cells = {Text("PLAY AS HOST")}},
							UiRow{.cells = {Text("PLAY AS CLIENT")}}}};
}

//NOTE: an action, the keys that do it, and for each pad its button as a picture and by name
UiTable ControlsTable()
{
	return UiTable{.rows = {UiRow{.cells = {Text("Controls:"),
											Text("P1/P2"),
											Picture(UiIcon::MenuXBoxHome),
											Text("XBox"),
											Picture(UiIcon::MenuPS5Home),
											Text("PS")}},
							UiRow{.cells = {Text("Pause"),
											Text("P"),
											Picture(UiIcon::MenuXBoxView),
											Text("View"),
											Picture(UiIcon::MenuPS5Create),
											Text("Create")}},
							UiRow{.cells = {Text("Menu"),
											Text("M"),
											Picture(UiIcon::MenuXBoxMenu),
											Text("Menu"),
											Picture(UiIcon::MenuPS5Options),
											Text("Options")}},
							UiRow{.cells = {Text("Swap"),
											Text("TAB"),
											Picture(UiIcon::MenuXBoxY),
											Text("Y"),
											Picture(UiIcon::MenuPS5Triangle),
											Text("Triangle")}},
							UiRow{.cells = {Text("Move"),
											Text("Arrows/WASD"),
											Picture(UiIcon::MenuXBoxDpad),
											Text("D-pad"),
											Picture(UiIcon::MenuPS5Dpad),
											Text("D-pad")}},
							UiRow{.cells = {Text("Fire"),
											Text("Space/LCtrl"),
											Picture(UiIcon::MenuXBoxA),
											Text("A"),
											Picture(UiIcon::MenuPS5Cross),
											Text("Cross")}}}};
}
}//namespace

Menu::Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _slide{static_cast<int>(gameConfig.LogicalSize().y)}
	, _events{events}
	, _input{std::make_unique<InputProviderForMenu>(events, gameConfig)}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();
}

void Menu::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Menu::OnSelectedGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &Menu::OnMenuShown));
}

void Menu::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }
void Menu::OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event) { _selectedGameMode = event.mode; }
void Menu::OnMenuShown(const MenuShownEvent& event) { DisplayMenu(event.isShown); }

void Menu::Draw()
{
	if (_slide > 0)
	{
		constexpr int slideStep{3};
		_slide = std::max(_slide - slideStep, 0);
	}

	_events->EmitEvent(RenderMenuBackgroundEvent{.slide = _slide});
	_events->EmitEvent(RenderMenuEvent{.slide = _slide,
									   .selectedRow = SelectedRow(),
									   .title = TitleTable(),
									   .modes = ModesTable(),
									   .controls = ControlsTable()});
}

int Menu::SelectedRow() const
{
	const auto mode{std::ranges::find(kModes, _selectedGameMode)};

	return mode == kModes.end() ? 0 : static_cast<int>(std::distance(kModes.begin(), mode));
}

void Menu::DisplayMenu(const bool isShown)
{
	_isMenuShown = isShown;

	if (_isMenuShown)
	{
		_drawSub = _events->AddListener(this, &Menu::OnDrawUserInterface);
	}
	else
	{
		_drawSub = EventSubscription{};
	}
}
