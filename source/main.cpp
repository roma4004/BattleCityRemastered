#include "application/CommandLineParser.h"
#include "application/Game.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "application/SDL_Config.h"
#include "application/WindowConfig.h"
#include "utils/Log.h"
#include <algorithm>
#include <iostream>

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main(const int argc, char* argv[])
{
	Log::SetConsole(true);
	Log::SetFile(true);
	Log::SetLevel(Log::Level::Normal);

	const auto launchOptions{CommandLineParser::Parse(argc, argv)};
	if (!launchOptions)
	{
		Log::Error("bad argument '" + launchOptions.error().arg + "': " + launchOptions.error().reason);

		return 1;
	}

	if (launchOptions->isHelpRequested)
	{
		//NOTE: past the log on purpose - a timestamped help line reads wrong and lands in the log file
		std::ranges::for_each(kUsage, [](const char* line) { std::cout << line << '\n'; });

		return 0;
	}

	ProjectConfig projectConfig{ProjectConfig::DefaultFilePath()};
	//NOTE: not fatal, but logged - the file is left untouched, and its settings would silently look ignored
	if (const auto& configError{projectConfig.LoadError()})
	{
		Log::Error("config " + configError->path.string() + " line " + std::to_string(configError->line) + ": "
				   + configError->reason + ", running on defaults and leaving the file untouched");
	}

	GameConfig gameConfig{};
	gameConfig.Apply(*launchOptions);

	WindowConfig windowConfig{projectConfig};
	windowConfig.Apply(*launchOptions);

	SDL_Config sdlEnv{gameConfig, projectConfig, windowConfig};
	if (const auto init{sdlEnv.Init()}; !init)
	{
		Log::Error(init.error().stage + ": " + init.error().detail);

		return 1;
	}

	Game game{gameConfig, projectConfig, windowConfig, sdlEnv, *launchOptions};
	game.Run();

	//NOTE: while the window still exists; projectConfig's destructor writes the ini afterwards
	sdlEnv.SaveWindowState(projectConfig);

	return Game::Result();
}
