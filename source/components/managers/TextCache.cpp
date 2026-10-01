#include "components/managers/TextCache.h"
#include "application/SDL_Config.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <functional>
#include <utility>

namespace
{
int PixelSize(const int basePointSize, const float scale)
{
	return static_cast<int>(std::lround(static_cast<float>(basePointSize) * scale));
}

//NOTE: logical units - the layout itself is in output pixels
Point SizeOfLaidOut(TTF_Text* const text, const float scale)
{
	int pixelWidth{};
	int pixelHeight{};
	if (!TTF_GetTextSize(text, &pixelWidth, &pixelHeight))
	{
		return {};
	}

	return Point{.x = static_cast<int>(static_cast<float>(pixelWidth) / scale),
				 .y = static_cast<int>(static_cast<float>(pixelHeight) / scale)};
}
}//namespace

void TextCache::TextDeleter::operator()(TTF_Text* text) const noexcept { TTF_DestroyText(text); }

void TextCache::EngineDeleter::operator()(TTF_TextEngine* engine) const noexcept
{
	TTF_DestroyRendererTextEngine(engine);
}

TextCache::TextCache(const SDL_Config& sdlConfig)
	: _sdlConfig{sdlConfig} {}

TextCache::~TextCache() = default;

void TextCache::Clear()
{
	_entries.clear();
	_measures.clear();
	_fonts.clear();
	_engine.reset();
}

//NOTE: every layout is laid out in output pixels, so a new scale invalidates the lot of them - and
//nothing else: reopening the fonts here is what made a window drag pay for the whole cache each frame
void TextCache::SyncScale(const float scale)
{
	if (scale == _scale)
	{
		return;
	}

	_entries.clear();
	_measures.clear();
	_scale = scale;
}

size_t TextCache::KeyHash::operator()(const Key& key) const noexcept
{
	return (*this)(KeyView{.text = key.text, .basePointSize = key.basePointSize});
}

size_t TextCache::KeyHash::operator()(const KeyView key) const noexcept
{
	size_t hash{std::hash<std::string_view>{}(key.text)};
	hash ^= static_cast<size_t>(key.basePointSize) + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);

	return hash;
}

bool TextCache::KeyEqual::operator()(const Key& lhs, const Key& rhs) const noexcept { return lhs == rhs; }

bool TextCache::KeyEqual::operator()(const Key& lhs, const KeyView rhs) const noexcept
{
	return lhs.basePointSize == rhs.basePointSize && lhs.text == rhs.text;
}

bool TextCache::KeyEqual::operator()(const KeyView lhs, const Key& rhs) const noexcept { return (*this)(rhs, lhs); }

bool TextCache::IsReady() const { return _sdlConfig.font && _sdlConfig.renderer; }

TTF_TextEngine* TextCache::Engine()
{
	if (!_engine)
	{
		_engine.reset(TTF_CreateRendererTextEngine(_sdlConfig.renderer.get()));
	}

	return _engine.get();
}

//NOTE: the file is read once, at startup - a size is a copy of that font resized, so fitting a block
//tries a dozen of them without touching the disk
TTF_Font* TextCache::FontForScale(const int basePointSize, const float scale)
{
	const int pixelSize{PixelSize(basePointSize, scale)};
	if (const auto it{_fonts.find(pixelSize)}; it != _fonts.end())
	{
		return it->second.get();
	}

	FontHandle sized{TTF_CopyFont(_sdlConfig.font.get())};
	if (!sized || !TTF_SetFontSize(sized.get(), static_cast<float>(pixelSize)))
	{
		//NOTE: nothing is stored on failure - an empty handle would retry the copy on every call,
		//and the startup font draws the line at its own size rather than not at all
		return _sdlConfig.font.get();
	}

	return _fonts.insert_or_assign(pixelSize, std::move(sized)).first->second.get();
}

TextCache::CachedText TextCache::LayOut(const std::string_view text, TTF_Font* const font, const float scale)
{
	TTF_TextEngine* const engine{Engine()};
	if (engine == nullptr)
	{
		return {};
	}

	std::unique_ptr<TTF_Text, TextDeleter> laidOut{TTF_CreateText(engine, font, text.data(), text.size())};
	if (!laidOut)
	{
		return {};
	}

	const Point size{SizeOfLaidOut(laidOut.get(), scale)};

	return CachedText{.text = std::move(laidOut), .width = size.x, .height = size.y};
}

Point TextCache::MeasureString(const std::string_view text, const int basePointSize, const float scale)
{
	if (!IsReady())
	{
		return {};
	}

	SyncScale(scale);

	if (const auto it{_measures.find(KeyView{.text = text, .basePointSize = basePointSize})}; it != _measures.end())
	{
		return it->second;
	}

	int pixelWidth{};
	int pixelHeight{};
	if (!TTF_GetStringSize(FontForScale(basePointSize, scale), text.data(), text.size(), &pixelWidth, &pixelHeight))
	{
		return {};
	}

	const Point size{.x = static_cast<int>(static_cast<float>(pixelWidth) / scale),
					 .y = static_cast<int>(static_cast<float>(pixelHeight) / scale)};

	return _measures.insert_or_assign(Key{.text = std::string{text}, .basePointSize = basePointSize}, size)
			.first->second;
}

const TextCache::CachedText* TextCache::Acquire(const std::string_view text, const SDL_Color& color,
												const int basePointSize, const float scale)
{
	if (!IsReady())
	{
		return nullptr;
	}

	SyncScale(scale);

	if (const auto it{_entries.find(KeyView{.text = text, .basePointSize = basePointSize})}; it != _entries.end())
	{
		TTF_SetTextColor(it->second.text.get(), color.r, color.g, color.b, color.a);

		return &it->second;
	}

	//NOTE: dropping the layouts bounds the cache and keeps their fonts - the few lines on screen refill it
	if (_entries.size() >= kMaxEntries)
	{
		_entries.clear();
		_measures.clear();
	}

	CachedText entry{LayOut(text, FontForScale(basePointSize, scale), scale)};
	if (!entry.text)
	{
		return nullptr;
	}

	TTF_SetTextColor(entry.text.get(), color.r, color.g, color.b, color.a);

	return &_entries.insert_or_assign(Key{.text = std::string{text}, .basePointSize = basePointSize},
									  std::move(entry))
					 .first->second;
}
