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
using Uint8 = uint8_t;
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
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::shared_ptr<TTF_Font> _fpsFont{nullptr};
	std::unordered_map<size_t, SDL_Texture*> _fpsTextures;// pregenerated fps texture
	std::unordered_map<int, SDL_Texture*> _colorTextureCache;
	SDL_Rect _fpsRectangle{};

	void Subscribe();
	void Unsubscribe() const;

	void Draw(ObjRectangle rect, Direction dir, const std::string& name, int color);
	void DrawTexture(const SDL_Rect* textureRect, const SDL_Rect* destRect, Direction dir) const;
	void DrawAnimation(ObjRectangle rect, Direction dir, int step, int scale, const std::string& name, int color);
	void DrawTankAnimation(ObjRectangle rect, Direction dir, int step, int scale, const std::string& name, int color);

	void SetRenderDrawColor(int color, Uint8 transparency) const;

	void ClearColorTextureCache();
	void ClearFpsTextureCache();
	void DrawHealthBar(ObjRectangle rect, int health, int color) const;
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	[[nodiscard]] SDL_Texture* CreateColorTexture(int color);
	void RectDraw(ObjRectangle rect, int color);
	[[nodiscard]] SDL_Rect GetAnimTextureRect(const std::string& name, ObjRectangle rect, SDL_Rect& destRect) const;
	[[nodiscard]] SDL_Rect GetTextureRect(const std::string& name) const;
	[[nodiscard]] SDL_Rect GetTankTextureRect(const std::string& name) const;
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void GenerateFpsTextures();

public:
	TextureManager(UPoint windowSize, const std::shared_ptr<SDL_Texture>& texture,
	               const std::shared_ptr<SDL_Renderer>& renderer, const std::shared_ptr<TTF_Font>& fpsFont,
	               const std::shared_ptr<EventSystem>& events);

	~TextureManager();

	void ClearFrame() const;
	void DisplayFrame(size_t fps);
};
