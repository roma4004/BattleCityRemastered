#pragma once

#include "components/EventSystem.h"
#include "components/input/InputProviderForMenu.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct DrawUserInterfaceEvent;
struct SelectedGameModeChangedToEvent;
struct MenuShownEvent;
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
	bool _isMenuShown{};

	void Subscribe();

	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnMenuShown(const MenuShownEvent& event);

	[[nodiscard]] int SelectedRow() const;
	void DisplayMenu(bool isShown);

	void Draw();

public:
	Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
