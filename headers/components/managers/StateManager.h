#pragma once

#include <SDL_render.h>
#include <memory>
#include <string>

class EventSystem;
enum class GameMode : char8_t;

class StateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};

	GameMode _gameMode{};
	bool _isPause{false};
	bool _isGameOver{false};
	bool _isGameWon{false};
	int _dynamicEnemiesRespawnCount{0};
	int _destroyedEnemiesCount{0};

	bool _playerOneFailState{false};
	bool _playerTwoFailState{false};
	bool _playersBaseFailState{false};

	void Subscribe();
	void Unsubscribe() const;

	void DrawPauseText() const;
	void DrawGameOverText() const;
	void DrawGameWonText() const;

	void Draw() const;

	void Reset();
	bool IsGameoverReached() const;
	bool IsGameWon();

public:
	StateManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
	             const std::shared_ptr<SDL_Texture>& atlasTexture);

	~StateManager();
};
