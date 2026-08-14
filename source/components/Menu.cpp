#include "components/Menu.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameMode.h"

Menu::Menu(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _pos{.x = 25, .y = 0}
	, _windowHeight{static_cast<int>(windowSize.y)}
	, _yOffsetStart{static_cast<int>(windowSize.y)}
	, _events{events}
	, _input{std::make_unique<InputProviderForMenu>(events)}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();
}

void Menu::Subscribe()
{
	if (_isMenuDisplayed)
	{
		_drawSub = _events->AddListener(this, &Menu::OnDrawUserInterface);
	}

	_subs.push_back(_events->AddListener(this, &Menu::OnSelectedGameModeChangedTo));

	_subs.push_back(_events->AddListener(this, &Menu::OnMenuShowed));
}

void Menu::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }
void Menu::OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event) { _selectedGameMode = event.mode; }
void Menu::OnMenuShowed(const MenuShowedEvent& event) { DisplayMenu(event.isShown); }

void Menu::Draw()
{
	// first time animation, slow scrolling from bottom corner to vertical center
	if (constexpr int yOffsetEnd = 0; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
		constexpr int padding{25};
		_pos.y = padding + _yOffsetStart;
		_events->EmitEvent(MenuPosChangedEvent{.pos = _pos});
	}

	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});
	_events->EmitEvent(RenderMenuLogoEvent{.pos = _pos});
	DrawMenuText();
	DrawControlHints();
}

void Menu::DrawMenuLine(Point& posText, const bool isSelected, std::string text) const
{
	if (isSelected)
	{
		_events->EmitEvent(RenderMenuSelectorIconEvent{.pos = Point{.x = posText.x - 35, .y = posText.y - 10}});
	}

	DrawTextLine(posText, std::move(text));
}

void Menu::DrawTextLine(Point& posText, std::string text) const
{
	constexpr unsigned int color = {0xffffffffu};
	_events->EmitEvent(RenderTextEvent{.pos = posText, .color = color, .text = text});
	posText.y += 30;
}

void Menu::DrawMenuText() const
{
	Point relativePosText{.x = _pos.x + 180, .y = _pos.y + 145};
	if (relativePosText.y >= _windowHeight)
	{
		return;
	}

	DrawMenuLine(relativePosText, _selectedGameMode == GameMode::OnePlayer, "ONE PLAYER");
	DrawMenuLine(relativePosText, _selectedGameMode == GameMode::TwoPlayers, "TWO PLAYER");
	DrawMenuLine(relativePosText, _selectedGameMode == GameMode::CoopWithBot, "COOP WITH BOT");
	DrawMenuLine(relativePosText, _selectedGameMode == GameMode::PlayAsHost, "PLAY AS HOST");
	DrawMenuLine(relativePosText, _selectedGameMode == GameMode::PlayAsClient, "PLAY AS CLIENT");
}

void Menu::DrawControlHints() const
{
	const Point relativePos{.x = _pos.x + 100, .y = _pos.y + 280};
	if (relativePos.y >= _windowHeight)
	{
		return;
	}

	constexpr int yBaseLineForControls = 150;
	_events->EmitEvent(RenderMenuXBoxHintEvent{.pos = Point{.x = relativePos.x + 245, .y = relativePos.y}});
	_events->EmitEvent(RenderMenuPS5HintEvent{
			.pos = Point{.x = relativePos.x + 280, .y = relativePos.y + yBaseLineForControls}});

	Point posText{.x = _pos.x + 40, .y = _pos.y + yBaseLineForControls + 200};
	DrawTextLine(posText, "Controls: P1/P2    XBox    PS");
	DrawTextLine(posText, "Pause       P      View    Create");
	DrawTextLine(posText, "Menu        M      Menu    Options");
	DrawTextLine(posText, "Swap       TAB     Y       Triangle");
	DrawTextLine(posText, "Move Arrows/WASD   D-pad   D-pad");
	DrawTextLine(posText, "Fire Space/LCtrl   A       Cross");
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
