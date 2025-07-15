#pragma once

#include "AnimationManager.h"
#include "Point.h"
#include "enums/TextureOffset.h"
#include <SDL_ttf.h>
#include <memory>
#include <unordered_map>

enum class Direction : char8_t;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Texture;
typedef uint8_t Uint8;
class BaseObj;
class EventSystem;

class TextureManager final
{
	std::string _name{"TextureManager"};
	UPoint _windowSize{};
	TextureOffset _offset{};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<SDL_Texture> _texture{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	std::shared_ptr<TTF_Font> _fpsFont{nullptr};

	std::unordered_map<size_t, SDL_Texture*> _fpsTextures;// pregenerated fps texture

	std::unordered_map<int, SDL_Texture*> _colorTextureCache;
	SDL_Rect _fpsRectangle{};
	AnimationManager _animationManager;

	void Subscribe();
	void Unsubscribe() const;

	void Draw(const BaseObj* obj);

	void SetRenderDrawColor(int color, Uint8 transparency) const;

	void ClearColorTextureCache();
	void ClearFpsTextureCache();
	void DrawHealthBar(const BaseObj* obj) const;
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	[[nodiscard]] SDL_Texture* CreateColorTexture(int color);
	void RectDraw(const BaseObj* obj);
	[[nodiscard]] SDL_Rect GetTextureRect(const BaseObj* obj, ObjRectangle rect, SDL_Rect& destRect) const;
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void GenerateFpsTextures();

public:
	TextureManager(UPoint windowSize, std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer,
	               std::shared_ptr<TTF_Font> fpsFont, std::shared_ptr<EventSystem> events);

	~TextureManager();

	void ClearFrame() const;
	void DisplayFrame(size_t fps);
};
