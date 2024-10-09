#pragma once

#include "GameMode.h"
#include "Point.h"
#include "input/InputProviderForMenu.h"

#include <SDL.h>
#include <SDL_ttf.h>

class GameStatistics;

class Menu final
{
	UPoint _pos;
	UPoint _windowSize;
	int* _windowBuffer;
	unsigned int _yOffsetStart;
	GameMode _selectedGameMode{Demo};
	std::unique_ptr<InputProviderForMenu> _input;
	std::shared_ptr<EventSystem> _events;
	std::string _name;
	SDL_Renderer* _renderer;

	TTF_Font* _menuFont{nullptr};
	SDL_Texture* _menuLogo{nullptr};

	std::shared_ptr<GameStatistics> _statistics;
	int _enemyRespawnResource{20};
	int _playerOneRespawnResource{3};
	int _playerTwoRespawnResource{3};

	void Subscribe();
	void Unsubscribe() const;

	void TextToRender(SDL_Renderer* renderer, const Point& pos, const SDL_Color& color, int value) const;
	void TextToRender(SDL_Renderer* renderer, Point pos, SDL_Color color, const std::string& text) const;
	void RenderStatistics(SDL_Renderer* renderer, Point pos) const;

	void BlendBackgroundToWindowBuffer();
	void HandleMenuText(SDL_Renderer* renderer, UPoint menuBackgroundPos) const;

public:
	Menu(SDL_Renderer* renderer, TTF_Font* menuFont, SDL_Texture* menuLogo, std::shared_ptr<GameStatistics> statistics,
	     UPoint windowSize, int* windowBuffer, std::unique_ptr<InputProviderForMenu>& input,
	     std::shared_ptr<EventSystem> events);

	~Menu();

	void Update() const;

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
