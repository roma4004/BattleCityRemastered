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
	unsigned int _yOffsetStart;
	GameMode _selectedGameMode{Demo};

	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TTF_Font> _menuFont{nullptr};
	std::shared_ptr<SDL_Texture> _menuLogo{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	//TODO: extract to separate sidebar class
	int _enemyRespawnResource{20};
	int _playerOneRespawnResource{3};
	int _playerTwoRespawnResource{3};

	std::string _name;

	void Subscribe();
	void Unsubscribe() const;

	void TextToRender(const Point& pos, const SDL_Color& color, int value) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text) const;
	void RenderStatistics(Point pos) const;

	void BlendBackgroundToWindowBuffer();
	void HandleMenuText(UPoint menuBackgroundPos) const;

public:
	Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
	     std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics, UPoint windowSize,
	     std::shared_ptr<int[]> windowBuffer, std::unique_ptr<InputProviderForMenu> input,
	     std::shared_ptr<EventSystem> events);

	~Menu();

	void Update() const;

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
