#pragma once

#include "../Point.h"
#include "../input/InputProviderForMenu.h"
#include <SDL.h>
#include <SDL_ttf.h>

class GameStatistics;
class InputProviderForMenu;
class EventSystem;

class Menu final
{
	Point _pos;
	int _height;
	int _width;
	int _padding;
	unsigned int _yOffsetStart{};
	GameMode _selectedGameMode{};

	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TTF_Font> _menuFont{nullptr};
	std::shared_ptr<SDL_Texture> _menuLogo{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};
	std::shared_ptr<int[]> _menuBackground{nullptr};
	std::shared_ptr<SDL_Texture> _menuBackgroundTexture{nullptr};
	std::shared_ptr<SDL_Texture> _backgroundTexture{nullptr};

	//TODO: extract to separate sidebar class
	int _enemyRespawnResource{20};
	int _playerOneRespawnResource{3};
	int _playerTwoRespawnResource{3};

	std::string _name;

	void Subscribe();
	void Unsubscribe() const;

	void PregenerateMenuBackground();

	void TextToRender(const Point& pos, const SDL_Color& color, int value) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text) const;
	void RenderStatistics(Point pos) const;
	void RenderTextWithAlignment(Point pos, SDL_Color color, const std::string& text, int player1, int player2,
	                             int enemy = -1) const;
	void RenderTextWithAlignment(Point pos, SDL_Color color, const std::string& text, const std::string& text2,
	                             const std::string& text3) const;

	void DrawBackground() const;
	void DrawMenuLogo() const;
	void DrawText() const;

	void OnRespawnResourceChanged(const std::string& objectName, int respawnResource);

public:
	Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
	     std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics, UPoint windowSize,
	     std::shared_ptr<EventSystem> events);

	~Menu();

	void MenuUpdate() const;
	void DrawMenu();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
