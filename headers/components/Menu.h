#pragma once

#include "components/EventSystem.h"
#include "components/input/InputProviderForMenu.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
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
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	GameMode _selectedGameMode{};
	bool _isMenuShown{};

	void Subscribe();

	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnMenuShown(const MenuShownEvent& event);

	[[nodiscard]] int SelectedRow() const;
	void DisplayMenu(bool isShown);

public:
	Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	void Draw();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
