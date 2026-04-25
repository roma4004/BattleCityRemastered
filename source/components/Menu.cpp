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

	_events->AddListener("ShowMenu", _name, [this](const bool isDisplayed)
	{
		DisplayMenu(isDisplayed);
	});

	_events->AddListener("ScoreBoardShowed", _name, [this](const bool isDisplayed)
	{
		if (isDisplayed)
		{
			this->DisplayMenu(false);
		}
	});
}

void Menu::Unsubscribe() const
{
	if (_isMenuDisplayed)
	{
		_events->RemoveListener("DrawUserInterface", _name);
	}

	_events->RemoveListener("SelectedGameModeChangedTo", _name);
	_events->RemoveListener("ShowMenu", _name);
	_events->RemoveListener("ScoreBoardShowed", _name);
}

//TODO: optimize draw call with cache non changed text part
void Menu::Draw()
{
	// first time animation, slow scrolling from bottom corner to vertical center
	if (constexpr unsigned int yOffsetEnd = 0u; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
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
		_events->EmitEvent("RenderMenuJoyIcon", Point{.x = posText.x - 35, .y = posText.y - 10});
	}

	DrawTextLine(posText, std::move(text));
}

void Menu::DrawTextLine(Point& posText, std::string text) const
{
	constexpr unsigned int color = {0xffffffff};
	_events->EmitEvent("RenderText", posText, color, text);
	posText.y += 25;
}

void Menu::DrawMenuText() const
{
	Point relativePosText{.x = _pos.x + 180, .y = _pos.y + 140};
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
	Point relativePos{.x = _pos.x + 100, .y = _pos.y + 280};
	if (relativePos.y >= _windowHeight)
	{
		return;
	}

	_events->EmitEvent("RenderP1ControlHint", relativePos);
	if (_selectedGameMode == GameMode::TwoPlayers)
	{
		_events->EmitEvent("RenderP2ControlHint", Point{.x = relativePos.x + 200, .y = relativePos.y});
	}

	Point posText{.x = _pos.x + 130, .y = _pos.y + 370};
	DrawTextLine(posText, "Show Menu [M]");
	DrawTextLine(posText, "Set/Unset Pause [P]");
	DrawTextLine(posText, "Swap Player Controls [TAB]");
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
