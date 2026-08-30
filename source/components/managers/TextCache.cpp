#include "components/managers/TextCache.h"
#include "application/SDL_Config.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <functional>
#include <utility>

void TextCache::TextDeleter::operator()(TTF_Text* text) const noexcept { TTF_DestroyText(text); }

void TextCache::EngineDeleter::operator()(TTF_TextEngine* engine) const noexcept
{
	TTF_DestroyRendererTextEngine(engine);
}

TextCache::TextCache(const SDL_Config& sdlConfig)
	: _sdlConfig{sdlConfig} {}

TextCache::~TextCache() = default;

//NOTE: the engine goes too - its atlas belongs to the renderer
void TextCache::Clear()
{
	_entries.clear();
	_fonts.clear();
	_engine.reset();
}

size_t TextCache::KeyHash::operator()(const Key& key) const noexcept
{
	size_t hash = std::hash<std::string>{}(key.text);
	hash ^= static_cast<size_t>(key.basePointSize) + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);

	return hash;
}

TTF_TextEngine* TextCache::Engine()
{
	if (!_engine)
	{
		_engine.reset(TTF_CreateRendererTextEngine(_sdlConfig.renderer.get()));
	}

	return _engine.get();
}

TTF_Font* TextCache::FontForScale(const int basePointSize, const float scale)
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

Point TextCache::MeasureString(const std::string& text, const int basePointSize, const float scale)
{
	if (!_sdlConfig.font)
	{
		return {};
	}

	int pixelWidth{};
	int pixelHeight{};
	if (!TTF_GetStringSize(FontForScale(basePointSize, scale), text.c_str(), text.size(), &pixelWidth, &pixelHeight))
	{
		return {};
	}

	return Point{.x = static_cast<int>(static_cast<float>(pixelWidth) / scale),
				 .y = static_cast<int>(static_cast<float>(pixelHeight) / scale)};
}

const TextCache::CachedText* TextCache::Acquire(const std::string& text, const SDL_Color& color,
												const int basePointSize, const float scale)
{
	if (!_sdlConfig.font || !_sdlConfig.renderer)
	{
		return nullptr;
	}

	if (scale != _scale)
	{
		Clear();
		_scale = scale;
	}

	const Key key{.text = text, .basePointSize = basePointSize};
	if (const auto it = _entries.find(key); it != _entries.end())
	{
		TTF_SetTextColor(it->second.text.get(), color.r, color.g, color.b, color.a);

		return &it->second;
	}

	//NOTE: a counter line is a fresh key every tick - drop all instead of growing, the few on screen refill
	if (_entries.size() >= kMaxEntries)
	{
		Clear();
		_scale = scale;
	}

	TTF_TextEngine* const engine = Engine();
	if (engine == nullptr)
	{
		return nullptr;
	}

	std::unique_ptr<TTF_Text, TextDeleter> laidOut(
			TTF_CreateText(engine, FontForScale(basePointSize, scale), text.c_str(), text.size()));
	if (!laidOut)
	{
		return nullptr;
	}

	TTF_SetTextColor(laidOut.get(), color.r, color.g, color.b, color.a);

	int pixelWidth{};
	int pixelHeight{};
	if (!TTF_GetTextSize(laidOut.get(), &pixelWidth, &pixelHeight))
	{
		return nullptr;
	}

	CachedText entry{.text = std::move(laidOut),
					 .width = static_cast<int>(static_cast<float>(pixelWidth) / scale),
					 .height = static_cast<int>(static_cast<float>(pixelHeight) / scale)};

	return &_entries.insert_or_assign(key, std::move(entry)).first->second;
}
