#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include <SDL_render.h>
#include <memory>
#include <string>
#include <unordered_map>

enum class Direction : char8_t;
enum class GameMode : char8_t;
struct ObjRectangle;
struct TTF_Font;
struct SDL_Config;
class EventSystem;
class GameConfig;
struct PreTickUpdateEvent;
struct RenderTextEvent;
struct RenderMenuBackgroundEvent;
struct RenderMenuLogoEvent;
struct RenderMenuSelectorIconEvent;
struct RenderMenuXBoxHintEvent;
struct RenderMenuPS5HintEvent;
struct RenderPauseTextEvent;
struct RenderGameOverTextEvent;
struct RenderGameWonTextEvent;
struct RenderColorTextureEvent;
struct RenderTextureEvent;
struct RenderFPSEvent;
struct RenderHealthBarEvent;
struct RenderRightSideBarEvent;
struct RenderEnemyIconBackgroundEvent;
struct RenderEnemyIconsEvent;
struct RenderPlayerOneIconEvent;
struct RenderPlayerTwoIconEvent;
struct RenderStageNumberEvent;
struct WindowSizeChangedToEvent;

class RenderManager
{

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	GameConfig& _gameConfig;
	SDL_Config& _sdlConfig;

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
	void OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event);

	void DrawPauseText(const RenderPauseTextEvent&) const;
	void DrawGameOverText(const RenderGameOverTextEvent&) const;
	void DrawGameWonText(const RenderGameWonTextEvent&) const;
	void DrawRightSideBar(const RenderRightSideBarEvent&) const;
	void DrawEnemyIconBackground(const RenderEnemyIconBackgroundEvent&) const;
	void DrawEnemyIcons(const RenderEnemyIconsEvent& event) const;
	void DrawPlayerOneIcons(const RenderPlayerOneIconEvent& event) const;
	void DrawPlayerTwoIcons(const RenderPlayerTwoIconEvent& event) const;
	void DrawStageNumber(const RenderStageNumberEvent& event) const;

	[[nodiscard]] static unsigned int ColorToInt(const SDL_Color& color);
	[[nodiscard]] static SDL_Color IntToColor(unsigned int color);
	[[nodiscard]] static unsigned int ComponentsToColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	[[nodiscard]] static SDL_Rect RectToSdlRect(const ObjRectangle& rect);
	void SetRenderDrawColor(unsigned int color, Uint8 transparency = 255) const;

	void DrawMenuBackground(const RenderMenuBackgroundEvent& event) const;
	void DrawMenuLogo(const RenderMenuLogoEvent& event) const;
	void DrawSelectorIcon(const RenderMenuSelectorIconEvent& event) const;
	void RenderCopyWithClipping(SDL_Texture* texture, SDL_Rect srcRect, SDL_Rect dstRect) const;
	void RenderCopy(SDL_Texture* texture, SDL_Rect dstRect) const;
	void DrawXBoxHint(const RenderMenuXBoxHintEvent& event) const;
	void DrawPS5Hint(const RenderMenuPS5HintEvent& event) const;
	void OnRenderText(const RenderTextEvent& event) const;
	void TextToRender(const Point& pos, const SDL_Color& color, int value, bool isMediumFontSize) const;
	void TextToRender(Point pos, SDL_Color color, const std::string& text, bool isMediumFontSize = false) const;

	void ClearFrame(const PreTickUpdateEvent&) const;
	void ClearColorTextureCache();
	void ClearFpsTextureCache();

	void CreateColorTexture(unsigned int color);
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void DrawColorTexture(const RenderColorTextureEvent& event);
	void DrawTexture(const RenderTextureEvent& event) const;

	void GenerateFpsTextures();
	void RenderFPS(const RenderFPSEvent& event);

	void DrawHealthBar(const RenderHealthBarEvent& event) const;
	void InitMenu(const GameConfig& gameConfig);

	[[nodiscard]] static SDL_Rect CalcFpsPos(const UPoint& newSize);

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig, SDL_Config& sdlConfig);

	~RenderManager();
};
