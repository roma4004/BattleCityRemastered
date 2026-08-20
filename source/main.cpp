#include "application/CommandLineParser.h"
#include "application/Game.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "application/SDL_Config.h"
#include "utils/Log.h"

//TODO: how to improve event system, duplicated code, std::string_view, NRVO, remove std::function, cleanup
int main(const int argc, char* argv[])
{
	Log::SetConsole(true);
	Log::SetFile(true);
	Log::SetLevel(Log::Level::Normal);

	const auto launchOptions = CommandLineParser::Parse(argc, argv);
	if (!launchOptions)
	{
		Log::Error("bad argument '" + launchOptions.error().arg + "': " + launchOptions.error().reason);

		return 1;
	}

	ProjectConfig projectConfig{"config.ini"};
	//NOTE: not fatal - defaults play fine. Said out loud because the file is kept as it is, so
	//otherwise the settings would just look ignored.
	if (const auto& configError = projectConfig.LoadError())
	{
		Log::Error("config " + configError->path.string() + " line " + std::to_string(configError->line) + ": "
				   + configError->reason + ", running on defaults and leaving the file untouched");
	}

	GameConfig gameConfig{projectConfig};
	gameConfig.Apply(*launchOptions);

	SDL_Config sdlEnv{gameConfig, projectConfig};
	if (const auto init = sdlEnv.Init(); !init)
	{
		Log::Error(init.error().stage + ": " + init.error().detail);

		return 1;
	}

	Game game{gameConfig, projectConfig, sdlEnv, launchOptions->gameMode};
	game.Run();

	//NOTE: before sdlEnv drops the window and while projectConfig is still alive - its destructor
	//is what writes the ini, and it outlives both
	sdlEnv.SaveWindowState(projectConfig);

	return game.Result();
}
