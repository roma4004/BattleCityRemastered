#pragma once

#include "application/SdlHandle.h"
#include "geometry/Point.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

struct SDL_Color;
struct SDL_Config;
struct TTF_Text;
struct TTF_TextEngine;

//NOTE: declared here rather than taken from SDL_Config.h - the cache opens fonts and nothing
//else, so it has no business pulling the renderer and the mixer in with the alias
using FontHandle = SdlHandle<TTF_Font, TTF_CloseFont>;

//NOTE: glyphs are laid out at final pixel size, so a layout is only valid for the scale that made it
class TextCache final
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

	//NOTE: the color is set per call, so one entry serves every color of the same line
	[[nodiscard]] const CachedText* Acquire(std::string_view text, const SDL_Color& color, int basePointSize,
											float scale);

	//NOTE: no layout - fitting tries sizes nothing will draw
	[[nodiscard]] Point MeasureString(std::string_view text, int basePointSize, float scale);

	//NOTE: the fonts and the engine go too - only a lost device needs that, a scale change does not
	void Clear();

private:
	struct Key
	{
		std::string text{};
		int basePointSize{};

		bool operator==(const Key&) const = default;
	};

	//NOTE: what a lookup is built from - the borrowed spelling of a key, so finding a line costs no string
	struct KeyView
	{
		std::string_view text{};
		int basePointSize{};
	};

	struct KeyHash
	{
		using is_transparent = void;

		[[nodiscard]] size_t operator()(const Key& key) const noexcept;
		[[nodiscard]] size_t operator()(KeyView key) const noexcept;
	};

	struct KeyEqual
	{
		using is_transparent = void;

		[[nodiscard]] bool operator()(const Key& lhs, const Key& rhs) const noexcept;
		[[nodiscard]] bool operator()(const Key& lhs, KeyView rhs) const noexcept;
		[[nodiscard]] bool operator()(KeyView lhs, const Key& rhs) const noexcept;
	};

	struct EngineDeleter
	{
		void operator()(TTF_TextEngine* engine) const noexcept;
	};

	[[nodiscard]] bool IsReady() const;
	[[nodiscard]] TTF_Font* FontForScale(int basePointSize, float scale);
	[[nodiscard]] TTF_TextEngine* Engine();
	[[nodiscard]] CachedText LayOut(std::string_view text, TTF_Font* font, float scale);
	//NOTE: the layouts alone - the fonts are keyed by final pixel size and the engine belongs to the
	//renderer, so a scale change leaves both valid
	void SyncScale(float scale);

	const SDL_Config& _sdlConfig;

	//NOTE: destroyed last - every TTF_Text points at the engine and a font
	std::unique_ptr<TTF_TextEngine, EngineDeleter> _engine{};

	//NOTE: keyed by final pixel size - a scale change only leaves the old sizes unasked for
	std::unordered_map<int, FontHandle> _fonts{};

	std::unordered_map<Key, CachedText, KeyHash, KeyEqual> _entries{};
	std::unordered_map<Key, Point, KeyHash, KeyEqual> _measures{};
	float _scale{};

	static constexpr size_t kMaxEntries{512};
};
