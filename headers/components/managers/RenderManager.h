#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "entities/pawns/BulletResetProperty.h"
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <unordered_map>

enum class GameMode : char8_t;
struct UPoint;
class EventSystem;

class RenderManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};
	std::shared_ptr<TTF_Font> _font{nullptr};
	std::shared_ptr<SDL_Texture> _menuLogo{nullptr};
	std::shared_ptr<SDL_Texture> _menuBackgroundTexture{nullptr};
	std::shared_ptr<SDL_Texture> _backgroundTexture{nullptr};
	std::shared_ptr<SDL_Texture> _atlas{nullptr};
	std::shared_ptr<SDL_Texture> _joyIcon{nullptr};
	std::shared_ptr<SDL_Texture> _xBoxHint{nullptr};
	std::shared_ptr<SDL_Texture> _pS5Hint{nullptr};
	std::shared_ptr<unsigned int[]> _menuBackground{nullptr};

	int _height;
	int _windowHeight;
	int _width;
	int _padding;

	SDL_Rect _fpsRectangle{};
	std::unordered_map<size_t, SDL_Texture*> _fpsTextures;// pregenerated fps texture
	std::unordered_map<unsigned int, SDL_Texture*> _colorTextureCache;

	void Subscribe();
	void Unsubscribe() const;

	void DrawPauseText() const;
	void DrawGameOverText() const;
	void DrawGameWonText() const;

	void PregenerateMenuBackgroundPixels();
	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ComponentsToColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void DrawBackground(Point pos) const;
	void DrawMenuLogo(Point pos) const;
	void DrawJoyIcon(Point pos) const;
	void RenderCopyWithClipping(SDL_Texture* texture, SDL_Rect srcRect, SDL_Rect dstRect) const;
	void DrawXBoxHint(Point pos) const;
	void DrawPS5Hint(Point pos) const;
	void TextToRender(const Point& pos, const SDL_Color& color, int value) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text) const;
	void PregenerateMenuBackgroundTexture();

	void ClearFrame() const;
	void ClearColorTextureCache();
	void ClearFpsTextureCache();

	[[nodiscard]] SDL_Texture* CreateColorTexture(unsigned int color);
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void DrawTexture(const ObjRectangle& texture, const ObjRectangle& dest, Direction dir) const;

	void GenerateFpsTextures();
	void RenderFPS(size_t fps);

	void DrawHealthBar(ObjRectangle rect, int health, unsigned int color) const;

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, const std::shared_ptr<SDL_Renderer>& renderer,
				  const std::shared_ptr<TTF_Font>& menuFont, const std::shared_ptr<SDL_Texture>& menuLogo,
				  const std::shared_ptr<SDL_Texture>& atlas, const std::shared_ptr<SDL_Texture>& joyIcon,
				  const std::shared_ptr<SDL_Texture>& xBoxHint, const std::shared_ptr<SDL_Texture>& pS5Hint,
				  UPoint windowSize);

	~RenderManager();
};
