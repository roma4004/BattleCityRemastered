#pragma once

#include <cstdint>
#include <string_view>

//NOTE: what is happening, whatever mode fills the seats; the scoreboard is a view of Won and Over
enum class GameState : char8_t
{
	Menu,
	Demo,
	Lobby,
	Playing,
	Paused,
	Won,
	Over
};

[[nodiscard]] constexpr std::string_view ToString(const GameState state) noexcept
{
	switch (state)
	{
		case GameState::Menu:
			return "menu";
		case GameState::Demo:
			return "demo";
		case GameState::Lobby:
			return "lobby";
		case GameState::Playing:
			return "playing";
		case GameState::Paused:
			return "paused";
		case GameState::Won:
			return "won";
		case GameState::Over:
			return "over";
	}

	return "unknown";
}
