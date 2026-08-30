#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/events/RenderUIEvents.h"
#include "components/input/InputProviderForMenu.h"
#include <memory>
#include <string>
#include <vector>

enum class GameMode : char8_t;
class GameConfig;
class EventSystem;
class GameStatistics;
class InputProviderForMenu;
struct DrawUserInterfaceEvent;
struct SelectedGameModeChangedToEvent;
struct MenuShowedEvent;

class Menu final
{
	Point _pos{};
	int _yOffsetStart{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayMenu() (also the initial Subscribe() call, if the menu starts
	// shown), independent of _subs's fixed subscribe-once-at-construction lifetime.
	EventSubscription _drawSub{};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	GameMode _selectedGameMode{};
	bool _isMenuDisplayed{false};

	void Subscribe();

	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);

	static constexpr int kLineStep{30};

	static void DrawTextLine(std::vector<TextBlockLine>& lines, Point& posText, std::string text);
	void DrawMenuText(std::vector<TextBlockLine>& lines) const;
	void DrawMenuLine(std::vector<TextBlockLine>& lines, Point& posText, bool isSelected, std::string text) const;
	void DrawControlHints(std::vector<TextBlockLine>& lines) const;
	void DisplayMenu(bool isDisplayed);

public:
	Menu(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	~Menu() = default;

	void Draw();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
