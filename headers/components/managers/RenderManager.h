#pragma once

#include "Point.h"
#include <SDL_render.h>
#include <memory>
#include <string>
#include <unordered_map>

struct SDL_Config;
enum class Direction : char8_t;
enum class GameMode : char8_t;
struct ObjRectangle;
struct TTF_Font;
class EventSystem;
class GameConfig;

class RenderManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};
	GameConfig& _gameConfig;
	SDL_Config& _sdlConfig;
	int _healthBarScale{1};

	struct MenuParams
	{
		UPoint panelSize{};
		size_t padding{};

		void Init(const UPoint windowSize, const size_t sideBarWidth)
		{
			padding = 50;
			panelSize = UPoint{.x = windowSize.x - sideBarWidth - padding * 2,
							   .y = windowSize.y - padding * 2};
		}
	};

	MenuParams _menuParams{};

	SDL_Rect _fpsRectangle{};
	// pregenerated fps texture
	std::unordered_map<size_t, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _fpsTextures;	
	std::unordered_map<unsigned int, std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>> _colorTextureCache;

	void Subscribe();
	void Unsubscribe() const;

	void DrawPauseText() const;
	void DrawGameOverText() const;
	void DrawGameWonText() const;
	void DrawRightSideBar() const;
	void DrawEnemyIconBackground() const;
	void DrawEnemyIcons(int numberOfIcons) const;
	void DrawPlayerOneIcons(unsigned short respawnCount) const;
	void DrawPlayerTwoIcons(unsigned short respawnCount) const;
	void DrawStageNumber(unsigned short currentStageNumber) const;

	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ComponentsToColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void DrawMenuBackground(Point pos) const;
	void DrawMenuLogo(Point pos) const;
	void DrawSelectorIcon(Point pos) const;
	void RenderCopyWithClipping(SDL_Texture* texture, SDL_Rect srcRect, SDL_Rect dstRect) const;
	void RenderCopy(SDL_Texture* texture, SDL_Rect dstRect) const;
	void DrawXBoxHint(Point pos) const;
	void DrawPS5Hint(Point pos) const;
	void TextToRender(const Point& pos, const SDL_Color& color, int value, bool isMediumFontSize) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text, bool isMediumFontSize = false) const;

	void ClearFrame() const;
	void ClearColorTextureCache();
	void ClearFpsTextureCache();

	void CreateColorTexture(unsigned int color);
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void DrawColorTexture(ObjRectangle rect);
	void DrawTexture(const ObjRectangle& texture, const ObjRectangle& dest, Direction dir) const;

	void GenerateFpsTextures();
	void RenderFPS(unsigned int fps);

	void DrawHealthBar(ObjRectangle rect, int health) const;
	void InitMenu(const GameConfig& gameConfig);

	[[nodiscard]] static SDL_Rect CalcFpsPos(const UPoint& newSize);

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig, SDL_Config& sdlConfig);

	~RenderManager();
};
