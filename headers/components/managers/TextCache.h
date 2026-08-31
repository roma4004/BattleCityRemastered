#pragma once

#include "application/SdlHandle.h"
#include "geometry/Point.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

struct SDL_Color;
struct SDL_Config;
struct TTF_Text;
struct TTF_TextEngine;

//NOTE: declared here rather than taken from SDL_Config.h - the cache opens fonts and nothing
//else, so it has no business pulling the renderer and the mixer in with the alias
using FontHandle = SdlHandle<TTF_Font, TTF_CloseFont>;

//NOTE: glyphs are laid out at final pixel size, so a layout is only valid for the scale that made it
class TextCache
{
public:
	struct TextDeleter
	{
		void operator()(TTF_Text* text) const noexcept;
	};

	struct CachedText
	{
		std::unique_ptr<TTF_Text, TextDeleter> text{};
		//NOTE: logical units, not pixels
		int width{};
		int height{};
	};

	explicit TextCache(const SDL_Config& sdlConfig);
	~TextCache();

	TextCache(const TextCache&) = delete;
	TextCache& operator=(const TextCache&) = delete;
	TextCache(TextCache&&) = delete;
	TextCache& operator=(TextCache&&) = delete;

	//NOTE: the colour is set per call, so one entry serves every colour of the same line
	[[nodiscard]] const CachedText* Acquire(const std::string& text, const SDL_Color& color, int basePointSize,
											float scale);

	//NOTE: no layout - fitting tries sizes nothing will draw
	[[nodiscard]] Point MeasureString(const std::string& text, int basePointSize, float scale);
	void Clear();

private:
	struct Key
	{
		std::string text{};
		int basePointSize{};

		bool operator==(const Key&) const = default;
	};

	struct KeyHash
	{
		[[nodiscard]] size_t operator()(const Key& key) const noexcept;
	};

	struct EngineDeleter
	{
		void operator()(TTF_TextEngine* engine) const noexcept;
	};

	[[nodiscard]] TTF_Font* FontForScale(int basePointSize, float scale);
	[[nodiscard]] TTF_TextEngine* Engine();

	const SDL_Config& _sdlConfig;

	//NOTE: destroyed last - every TTF_Text points at the engine and a font
	std::unique_ptr<TTF_TextEngine, EngineDeleter> _engine{};

	//NOTE: keyed by final pixel size - a scale change drops these along with the layouts
	std::unordered_map<int, FontHandle> _fonts{};

	std::unordered_map<Key, CachedText, KeyHash> _entries{};
	float _scale{0.f};

	static constexpr size_t kMaxEntries{512};
};
