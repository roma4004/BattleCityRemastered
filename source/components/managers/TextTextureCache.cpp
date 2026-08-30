#include "components/managers/TextTextureCache.h"
#include "application/SDL_Config.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <functional>

TextTextureCache::TextTextureCache(const SDL_Config& sdlConfig)
	: _sdlConfig{sdlConfig} {}

void TextTextureCache::Clear() { _entries.clear(); }

unsigned int TextTextureCache::ColorToKey(const SDL_Color& color)
{
	return (static_cast<unsigned int>(color.a) << 24u) | (static_cast<unsigned int>(color.r) << 16u)
		   | (static_cast<unsigned int>(color.g) << 8u) | static_cast<unsigned int>(color.b);
}

size_t TextTextureCache::KeyHash::operator()(const Key& key) const noexcept
{
	size_t hash = std::hash<std::string>{}(key.text);
	const auto mix = [&hash](const size_t value) {
		hash ^= value + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);
	};
	mix(key.color);
	mix(static_cast<size_t>(key.basePointSize));

	return hash;
}

TTF_Font* TextTextureCache::FontForScale(const int basePointSize, const float scale)
{
	const int pixelSize = static_cast<int>(std::lround(static_cast<float>(basePointSize) * scale));

	auto& scaledFont = _fonts[pixelSize];
	if (!scaledFont)
	{
		scaledFont = _sdlConfig.OpenFont(pixelSize);
	}

	//NOTE: the startup font is the last resort only - it is fixed at its own pixel size and ignores the
	//scale, so a line drawn with it comes out the wrong size
	return scaledFont ? scaledFont.get() : _sdlConfig.font.get();
}

const TextTextureCache::CachedText* TextTextureCache::Acquire(const std::string& text, const SDL_Color& color,
															  const int basePointSize, const float scale)
{
	if (!_sdlConfig.font || !_sdlConfig.renderer)
	{
		return nullptr;
	}

	if (scale != _scale)
	{
		Clear();
		_fonts.clear();
		_scale = scale;
	}

	const Key key{.text = text, .color = ColorToKey(color), .basePointSize = basePointSize};
	if (const auto it = _entries.find(key); it != _entries.end())
	{
		return &it->second;
	}

	//NOTE: a counter line is a fresh key every tick - drop all instead of growing, the few on screen refill
	if (_entries.size() >= kMaxEntries)
	{
		Clear();
	}

	TTF_Font* const currentFont = FontForScale(basePointSize, scale);
	const std::unique_ptr<SDL_Surface, void (*)(SDL_Surface*)> surface(
			TTF_RenderText_Solid(currentFont, text.c_str(), 0u, color), SDL_DestroySurface);
	if (!surface)
	{
		return nullptr;
	}

	std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
			SDL_CreateTextureFromSurface(_sdlConfig.renderer.get(), surface.get()), SDL_DestroyTexture);
	if (!texture)
	{
		return nullptr;
	}

	CachedText entry{.texture = std::move(texture),
					 .width = static_cast<int>(static_cast<float>(surface->w) / scale),
					 .height = static_cast<int>(static_cast<float>(surface->h) / scale)};

	return &_entries.insert_or_assign(key, std::move(entry)).first->second;
}
