#include "components/managers/StateManager.h"
#include "components/EventSystem.h"
#include "enums/TextureOffset.h"

#include <SDL_rect.h>
#include <SDL_render.h>

StateManager::StateManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
                           const std::shared_ptr<SDL_Texture>& atlasTexture)
	: _name{"StateManager"},
	  _events{events},
	  _renderer{renderer},
	  _atlasTexture{atlasTexture}
{
	Subscribe();
}

StateManager::~StateManager()
{
	Unsubscribe();
}

void StateManager::Subscribe() const {}

void StateManager::Unsubscribe() const {}

void StateManager::DrawPauseText() const
{
	constexpr TextureOffset offset{};
	constexpr SDL_Rect rect{.x = 135, .y = 242, .w = 300, .h = 75};

	SDL_Rect srcrect{static_cast<int>(offset.pauseText.x),
	                 static_cast<int>(offset.pauseText.y),
	                 static_cast<int>(offset.pauseText.w),
	                 static_cast<int>(offset.pauseText.h)};
	SDL_RenderCopy(_renderer.get(), _atlasTexture.get(), &srcrect, &rect);
}
