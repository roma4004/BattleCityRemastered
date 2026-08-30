#pragma once

#include <SDL3/SDL_render.h>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

struct SDL_Config;
struct TTF_Font;

//NOTE: SDL stretching is right for pixel art and wrong for glyphs, so text alone is rasterized at
//its final pixel size - and a raster is only valid for the scale that made it
class TextTextureCache
{
public:
	struct CachedText
	{
		std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{nullptr, nullptr};
		int width{};
		int height{};
	};

	explicit TextTextureCache(const SDL_Config& sdlConfig);

	//NOTE: filled on first draw, not up front
	[[nodiscard]] const CachedText* Acquire(const std::string& text, const SDL_Color& color, int basePointSize,
											float scale);
	void Clear();

private:
	struct Key
	{
		std::string text{};
		unsigned int color{};
		int basePointSize{};

		bool operator==(const Key&) const = default;
	};

	struct KeyHash
	{
		[[nodiscard]] size_t operator()(const Key& key) const noexcept;
	};

	[[nodiscard]] TTF_Font* FontForScale(int basePointSize, float scale);
	[[nodiscard]] static unsigned int ColorToKey(const SDL_Color& color);

	const SDL_Config& _sdlConfig;

	std::unordered_map<Key, CachedText, KeyHash> _entries{};
	float _scale{0.f};

	//NOTE: keyed by the final pixel size - only the current scale is ever drawn at, so a scale change
	//drops these along with the rasters
	std::unordered_map<int, std::shared_ptr<TTF_Font>> _fonts{};

	static constexpr size_t kMaxEntries{512};
};
