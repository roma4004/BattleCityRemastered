#pragma once

#include "BaseObj.h"
#include "enums/TextureOffset.h"
#include "interfaces/IDrawable.h"

#include <memory>

struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Texture;
class BaseObj;

class TextureManager final : public IDrawable
{
	TextureOffset _offset{};
	std::shared_ptr<SDL_Renderer> _renderer;
	std::shared_ptr<SDL_Texture> _texture;

public:
	TextureManager(std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer);

	~TextureManager() override;

	void Draw(const BaseObj* obj) const override;

	static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
};
