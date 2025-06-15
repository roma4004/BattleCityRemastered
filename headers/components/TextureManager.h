#pragma once

#include "../BaseObj.h"
#include "../Point.h"
#include "../enums/TextureOffset.h"
#include "../interfaces/IDrawable.h"
#include <memory>
#include <unordered_map>

struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Texture;
typedef uint8_t Uint8;
class BaseObj;
class EventSystem;

class TextureManager final : public IDrawable
{
	std::string _name{"TextureManager"};
	UPoint _windowSize{};
	TextureOffset _offset{};
	std::shared_ptr<SDL_Renderer> _renderer;
	std::shared_ptr<SDL_Texture> _texture;
	std::shared_ptr<EventSystem> _events{nullptr};

	mutable std::unordered_map<int, SDL_Texture*> _colorTextureCache;

public:
	TextureManager(UPoint windowSize, std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer,
	               std::shared_ptr<EventSystem> events);

	~TextureManager() override;

	void Subscribe() const;
	void Unsubscribe() const;

	void SetRenderDrawColor(int color, Uint8 transparency) const;

	void ClearColorTextureCache() const;

	void Draw(const BaseObj* obj) const override;

	void DrawHealthBar(const BaseObj* obj) const;
	static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	SDL_Texture* CreateColorTexture(int color) const;
	void RectDraw(const BaseObj* obj) const;
};
