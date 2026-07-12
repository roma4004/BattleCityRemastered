#include "components/Menu.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "enums/GameMode.h"

Menu::Menu(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _yOffsetStart{static_cast<unsigned int>(windowSize.y)}
	, _events{events}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _input{std::make_unique<InputProviderForMenu>(events)}
	, _name{std::string("Menu")}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	_padding = 25;
	_windowHeight = static_cast<int>(windowSize.y);
}

Menu::~Menu()
{
	Unsubscribe();
}

void Menu::Subscribe()
{
	if (_isMenuDisplayed)
	{
		_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
	}

	_events->AddListener("SelectedGameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_selectedGameMode = newGameMode;
	});

	_events->AddListener("MenuShowed", _name, [this](const bool isDisplayed)
	{
		DisplayMenu(isDisplayed);
	});
}

void Menu::Unsubscribe() const { _events->RemoveAllListeners(_name); }

//TODO: optimize draw call with cache non changed text part
void Menu::Draw()
{
	// first time animation, slow scrolling from bottom corner to vertical center
	if (constexpr unsigned int yOffsetEnd = 0u; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3u;
	}

	_pos.x = _padding;
	_pos.y = static_cast<int>(_padding + _yOffsetStart);

	_events->EmitEvent("RenderMenuBackground", _pos);
	_events->EmitEvent("RenderMenuLogo", _pos);
	DrawMenuText();
	DrawControlHints();
}

void Menu::DrawMenuLine(Point& posText, bool isSelected, std::string text) const
{
	if (isSelected)
	{
		_events->EmitEvent("RenderMenuSelectorIcon", Point{.x = posText.x - 35, .y = posText.y - 10});
	}

	DrawTextLine(posText, std::move(text));
}

void Menu::DrawTextLine(Point& posText, std::string text) const
{
	constexpr unsigned int color = {0xffffffffu};
	_events->EmitEvent("RenderText", posText, color, text);
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
	_events->EmitEvent("RenderMenuXBoxHint", Point{.x = relativePos.x + 245, .y = relativePos.y});
	_events->EmitEvent("RenderMenuPS5Hint", Point{.x = relativePos.x + 280, .y = relativePos.y + yBaseLineForControls});

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
		_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
	}
	else
	{
		_events->RemoveListener("DrawUserInterface", _name);
	}
}
