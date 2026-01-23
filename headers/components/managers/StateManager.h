#pragma once

#include <SDL_render.h>
#include <memory>
#include <string>

class EventSystem;

class StateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};

public:
	explicit StateManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
	                      const std::shared_ptr<SDL_Texture>& atlasTexture);

	~StateManager();

	void Subscribe() const;
	void Unsubscribe() const;

	void DrawPauseText() const;
};
