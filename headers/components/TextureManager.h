#pragma once

#include "../BaseObj.h"
#include "../enums/TextureOffset.h"
#include "../interfaces/IDrawable.h"

#include <memory>

struct Window;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Texture;
class BaseObj;
class EventSystem;

class TextureManager final : public IDrawable
{
	std::string _name{"TextureManager"};
	TextureOffset _offset{};
	std::shared_ptr<SDL_Renderer> _renderer;
	std::shared_ptr<SDL_Texture> _texture;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<Window> _window{nullptr};

public:
	TextureManager(std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer,
	               std::shared_ptr<EventSystem> events, std::shared_ptr<Window> window);

	~TextureManager() override;

	void Draw(const BaseObj* obj) const override;

	static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	void RectDraw(const BaseObj* obj, ObjRectangle rect) const;
};
