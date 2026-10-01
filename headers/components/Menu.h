#pragma once

#include "components/EventSystem.h"
#include "components/UiTable.h"
#include "components/input/InputProviderForMenu.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct DrawUserInterfaceEvent;
struct SelectedGameModeChangedToEvent;
struct MenuShowedEvent;
class GameConfig;
class EventSystem;
class GameStatistics;
class InputProviderForMenu;

class Menu final
{
	int _slide{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayMenu(), where _subs is filled once at construction and stays
	EventSubscription _drawSub{};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	GameMode _selectedGameMode{};
	bool _isMenuDisplayed{};

	void Subscribe();

	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);

	[[nodiscard]] static UiTable TitleTable();
	[[nodiscard]] static UiTable ModesTable();
	//NOTE: an action, the keys that do it, and for each pad its button as a picture and by name
	[[nodiscard]] static UiTable ControlsTable();
	[[nodiscard]] int SelectedRow() const;
	void DisplayMenu(bool isDisplayed);

	void Draw();

public:
	Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
