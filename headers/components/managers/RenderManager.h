#pragma once

#include "Point.h"
#include <SDL_render.h>
#include <memory>
#include <string>
#include <unordered_map>

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

	struct MenuParams
	{
		UPoint panelSize{};
		size_t padding{};
		std::shared_ptr<unsigned int[]> backgroundPixelArray{nullptr};
		std::shared_ptr<SDL_Texture> backgroundTexture{nullptr};

		void Init(const UPoint windowSize, const size_t sideBarWidth, const std::shared_ptr<SDL_Renderer>& renderer)
		{
			padding = 25;
			panelSize = UPoint{.x = windowSize.x - sideBarWidth - padding,
							  .y = windowSize.y - padding * 3u};

			PregenerateBackgroundPixels();
			PregenerateBackgroundTexture(renderer);
		}

		void PregenerateBackgroundPixels()
		{
			backgroundPixelArray = std::make_shared<unsigned int[]>(panelSize.y * panelSize.x);
			for (size_t y = 0; y < panelSize.y; ++y)
			{
				for (size_t x = 0; x < panelSize.x; ++x)
				{
					const int i = static_cast<int>(y * panelSize.x + x);
					constexpr unsigned int menuColor = 0x91808080;// Alpha channel 0x80 for semi-transparency gray
					backgroundPixelArray[i] = menuColor;
				}
			}
		}

		void PregenerateBackgroundTexture(const std::shared_ptr<SDL_Renderer>& renderer)
		{
			// SDL_SetRenderDrawBlendMode(_gameConfig.renderer.get(), SDL_BLENDMODE_BLEND);
			backgroundTexture = std::shared_ptr<SDL_Texture>(SDL_CreateTexture(
																	 renderer.get(),
																	 SDL_PIXELFORMAT_ARGB8888,
																	 SDL_TEXTUREACCESS_TARGET,
																	 static_cast<int>(panelSize.x),
																	 static_cast<int>(panelSize.y)),
															 SDL_DestroyTexture);
			SDL_SetTextureBlendMode(backgroundTexture.get(), SDL_BLENDMODE_BLEND);

			const SDL_Rect rect{.x = static_cast<int>(padding),
								.y = static_cast<int>(padding),
								.w = static_cast<int>(panelSize.x),
								.h = static_cast<int>(panelSize.y)};
			const int pitch = static_cast<int>(panelSize.x << 2ul);
			SDL_UpdateTexture(backgroundTexture.get(), &rect, backgroundPixelArray.get(), pitch);
		}
	};

	MenuParams _menuParams{};

	SDL_Rect _fpsRectangle{};
	std::unordered_map<size_t, SDL_Texture*> _fpsTextures;// pregenerated fps texture
	std::unordered_map<unsigned int, SDL_Texture*> _colorTextureCache;
	std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> _colorTexture{nullptr, nullptr};

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

	void PregenerateMenuBackgroundPixels();
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

	[[nodiscard]] SDL_Texture* CreateColorTexture(unsigned int color);
	[[nodiscard]] static std::pair<double, SDL_RendererFlip> GetRotateAndAngleAndFlip(Direction dir);
	void DrawColorTexture(ObjRectangle rect) const;
	void DrawTexture(const ObjRectangle& texture, const ObjRectangle& dest, Direction dir) const;

	void GenerateFpsTextures();
	void RenderFPS(size_t fps);

	void DrawHealthBar(ObjRectangle rect, int health) const;
	void InitMenu(const GameConfig& gameConfig);

public:
	RenderManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~RenderManager();
};
