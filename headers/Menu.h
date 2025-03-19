#pragma once

#include "Point.h"
#include "input/InputProviderForMenu.h"

#include <SDL.h>
#include <SDL_ttf.h>

class GameStatistics;
class InputProviderForMenu;
class EventSystem;
struct Window;

class Menu final
{
	UPoint _pos;
	unsigned int _yOffsetStart{0u};
	GameMode _selectedGameMode;

	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
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
	void RenderTextWithAlignment(Point pos, SDL_Color color, const std::string& text, int player1, int player2,
	                             int enemy = -1) const;
	void RenderTextWithAlignment(Point pos, SDL_Color color, const std::string& text, const std::string& text2,
	                             const std::string& text3) const;

	void BlendBackgroundToWindowBuffer();
	void DrawMenuText(UPoint menuBackgroundPos) const;
	void OnRespawnResourceChanged(const std::string& objectName, int respawnResource);

public:
	Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
	     std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics,
	     std::shared_ptr<Window> window, std::unique_ptr<InputProviderForMenu> input,
	     std::shared_ptr<EventSystem> events);

	~Menu();

	void Update() const;

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
