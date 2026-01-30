#include "components/managers/StateManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/TextureOffset.h"

#include <SDL_rect.h>
#include <SDL_render.h>

StateManager::StateManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
                           const std::shared_ptr<SDL_Texture>& atlasTexture)
	: _name{"StateManager"},
	  _events{events},
	  _renderer{renderer},//TODO: extract render to separated class that serve all the renders call
	  _atlasTexture{atlasTexture}
{
	Subscribe();
}

StateManager::~StateManager()
{
	Unsubscribe();
}

void StateManager::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool value) { this->_isPause = value; });// TODO: investigate why on demo after start we have skip one pause input
	_events->AddListener("SetGameOverText", _name, [this]() { this->_isGameOver = true; });
	_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("PlayerOneFinished", _name, [this]() { _playerOneFailState = true; _isGameOver = IsGameoverReached(); });
	_events->AddListener("PlayerTwoFinished", _name, [this]() { _playerTwoFailState = true; _isGameOver = IsGameoverReached(); }); 
	_events->AddListener("PlayersBaseFinished", _name, [this]() {_playersBaseFailState = true; _isGameOver = IsGameoverReached(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});
}

void StateManager::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("SetGameOverText", _name);
	_events->RemoveListener("DrawUserInterface", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("PlayerOneFinished", _name);
	_events->RemoveListener("PlayerTwoFinished", _name);
	_events->RemoveListener("PlayersBaseFinished", _name);
}

void StateManager::DrawPauseText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 135, .y = 142, .w = 300, .h = 75};

	SDL_Rect srcrect{static_cast<int>(offset.pauseText.x),
	                 static_cast<int>(offset.pauseText.y),
	                 static_cast<int>(offset.pauseText.w),
	                 static_cast<int>(offset.pauseText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
}

void StateManager::DrawGameOverText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 200, .y = 242, .w = 200, .h = 75};

	SDL_Rect srcrect{static_cast<int>(offset.gameOverText.x),
					 static_cast<int>(offset.gameOverText.y),
					 static_cast<int>(offset.gameOverText.w),
					 static_cast<int>(offset.gameOverText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
}

void StateManager::Draw() const
{
	if (_isPause)
	{
		DrawPauseText();
	}

	if (_isGameOver)
	{
		DrawGameOverText();
	}
}

void StateManager::Reset()
{
	_isPause = false;
	_isGameOver = false;
}

bool StateManager::IsGameoverReached() const
{
	return (_gameMode == GameMode::OnePlayer && _playerOneFailState && _playersBaseFailState)
		|| (_gameMode == GameMode::TwoPlayers && _playerOneFailState && _playerTwoFailState && _playersBaseFailState)
		|| (_gameMode == GameMode::Demo && _playerOneFailState && _playerTwoFailState && _playersBaseFailState);
}
