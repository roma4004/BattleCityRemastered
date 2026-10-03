#include "application/CommandLineParser.h"
#include "application/ConsoleCommand.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "application/ServerConsole.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ServerConsoleEvents.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/LevelRotation.h"
#include "components/MapData.h"
#include "components/MapLoader.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

namespace
{
std::atomic_bool isStopRequested{};

extern "C" void OnStopSignal(int) { isStopRequested.store(true, std::memory_order_relaxed); }

//NOTE: in the game the pause belongs to the menu, and there is no menu here - so a console command and a
//client's request would reach nothing. Everything downstream waits on PauseStatusEvent, this is what says it
class PauseSwitch final
{
public:
	explicit PauseSwitch(const std::shared_ptr<EventSystem>& events)
		: _events{events}
		, _setPauseSub{events->AddListener(this, &PauseSwitch::OnSetPause)} {}

private:
	void OnSetPause(const SetPauseEvent& event)
	{
		if (_isPaused == event.isPaused)
		{
			return;
		}

		_isPaused = event.isPaused;
		_events->EmitEvent(PauseStatusEvent{.isPaused = _isPaused});
	}

	std::shared_ptr<EventSystem> _events;
	EventSubscription _setPauseSub;
	bool _isPaused{};
};


//NOTE: said only when a name did not work out - a listing nobody asked for is noise, and one that comes
//with the complaint is the answer to the question the complaint raises
std::string KnownMaps()
{
	const std::vector<std::string> names{LevelRotation{}.Names()};

	//NOTE: one named result and one object returned - two returns of different ones is what -Wnrvo is about
	std::string listed{};
	if (!names.empty())
	{

		listed = ". known maps: ";
		for (const std::string& name: names)
		{
			listed += name + (&name == &names.back() ? "" : ", ");
		}
	}

	return listed;
}

//NOTE: a map that does not load or cannot be played says why, and no match is started on it
[[nodiscard]] bool IsPlayable(const std::string& path)
{
	const auto loaded{MapLoader::LoadFromFile(path)};
	if (!loaded)
	{
		const MapError& error{loaded.error()};
		const std::string where{error.line != 0u ? " (line " + std::to_string(error.line) + ')'
												 : std::string{}};
		Log::Error("cannot load map " + path + where + ": " + error.reason + KnownMaps());

		return false;
	}

	if (const auto playable{MapLoader::Validate(*loaded, path)};
		!playable)
	{
		Log::Error("map " + path + " cannot be played: " + playable.error().reason);

		return false;
	}

	return true;
}

//NOTE: what the console asks for, done on the game thread between frames; a restart goes the way a player's does
struct ConsoleCommandHandler final
{
	EventSystem& events;
	//NOTE: not const any more - /map is the one command that writes the world it describes
	GameConfig& gameConfig;
	const FramePerSecondManager& fpsManager;
	std::chrono::steady_clock::time_point startedAt;

	void operator()(const ExitCommand&) const { isStopRequested.store(true, std::memory_order_relaxed); }
	void operator()(const RestartCommand&) const { events.EmitEvent(ServerInRestartRequestedEvent{}); }
	void operator()(const PlayersCommand&) const { events.EmitEvent(ServerPlayersRequestedEvent{}); }

	void operator()(const KickCommand& command) const
	{
		events.EmitEvent(ServerKickRequestedEvent{.slot = command.slot});
	}

	void operator()(const PauseCommand& command) const
	{
		events.EmitEvent(SetPauseEvent{.isPaused = command.isPaused});
	}

	void operator()(const LogLevelCommand& command) const { Log::SetLevel(command.level); }

	//NOTE: the map is checked before anything is torn down - a name that does not load leaves the match
	//running on the one it already has, and the console says why
	void operator()(const MapCommand& command) const
	{
		const std::string path{MapPathForName(command.name)};
		if (!IsPlayable(path))
		{
			return;
		}

		gameConfig.mapPath = path;
		Log::Info("next match runs on " + path);
		events.EmitEvent(ServerInRestartRequestedEvent{});
	}

	void operator()(const AcceptClientsCommand& command) const
	{
		events.EmitEvent(ServerAcceptingChangedEvent{.isAccepting = command.isAccepting});
	}

	void operator()(const HelpCommand&) const
	{
		std::ranges::for_each(kConsoleHelp, [](const char* line) { std::cout << line << '\n'; });
	}

	void operator()(const StatusCommand&) const
	{
		const auto running{std::chrono::steady_clock::now() - startedAt};
		const auto uptime{std::chrono::duration_cast<std::chrono::seconds>(running)};
		Log::Info("phase " + std::string{ToString(gameConfig.gameState)} + ", uptime " + std::to_string(uptime.count())
				  + "s, fps " + std::to_string(fpsManager.ActualFps()));
		events.EmitEvent(ServerStatusRequestedEvent{});
	}
};

//NOTE: written whole and closed before the game is told to look - a half-written number reads as a port
bool WriteBoundPort(const std::string& path, const std::uint16_t port)
{
	std::ofstream file{path, std::ios::trunc};
	file << port;

	return file.good();
}

void ApplyConsoleLine(const std::string& line, const ConsoleCommandHandler& handler)
{
	const auto command{ParseConsoleCommand(line)};
	if (!command)
	{
		Log::Error(command.error());

		return;
	}

	std::visit(handler, *command);
}

}//namespace

//NOTE: the same Simulation the game runs, minus the screen - it links GameCore alone, so SDL cannot
//reach it. A spawning tank still lands: PostTickUpdate advances the animation without draw phases
int main(const int argc, char* argv[])
{
	Log::SetConsole(true);
	Log::SetFile(true);
	Log::SetLevel(Log::Level::Normal);

	std::signal(SIGINT, OnStopSignal);
	std::signal(SIGTERM, OnStopSignal);

	const auto launchOptions{CommandLineParser::ParseServer(argc, argv)};
	if (!launchOptions)
	{
		Log::Error("bad argument '" + launchOptions.error().arg + "': " + launchOptions.error().reason);

		return 1;
	}

	if (launchOptions->isHelpRequested)
	{
		//NOTE: past the log on purpose - a timestamped help line reads wrong and lands in the log file
		std::ranges::for_each(kServerUsage, [](const char* line) { std::cout << line << '\n'; });

		return 0;
	}

	const ProjectConfig projectConfig{ProjectConfig::DefaultFilePath()};
	if (const auto& configError{projectConfig.LoadError()})
	{
		Log::Error("config " + configError->path.string() + " line " + std::to_string(configError->line) + ": "
				   + configError->reason + ", running on defaults and leaving the file untouched");
	}

	GameConfig gameConfig{};
	//NOTE: loopback is out of reach from another machine
	gameConfig.serverAddress.host = launchOptions->serverHost ? *launchOptions->serverHost : network::LocalAddress();
	gameConfig.serverAddress.port = launchOptions->serverPort.value_or(gameConfig.serverAddress.port);
	gameConfig.networkSeats = launchOptions->seats.value_or(gameConfig.networkSeats);
	gameConfig.networkRules = launchOptions->rules.value_or(gameConfig.networkRules);
	gameConfig.simultaneousEnemies = launchOptions->enemiesAtOnce.value_or(gameConfig.simultaneousEnemies);
	if (launchOptions->mapName)
	{
		gameConfig.mapPath = MapPathForName(*launchOptions->mapName);
		if (!IsPlayable(gameConfig.mapPath))
		{
			return 1;
		}
	}

	const auto events{std::make_shared<EventSystem>()};
	const PauseSwitch pauseSwitch{events};
	//NOTE: no presenter here, so its pacing is all that stands between this loop and a busy spin
	const FramePerSecondManager fpsManager{events, projectConfig, false};
	Simulation simulation{events, gameConfig};

	simulation.ApplyGameMode(GameMode::PlayAsHost);

	//NOTE: the port asked for may have been 0, so this is the first place the real one is known
	const std::uint16_t boundPort{simulation.BoundPort()};
	//NOTE: a failed bind is caught and logged by the event system - no port is all that is left of it here
	if (boundPort == network::kAnyFreePort)
	{
		Log::Error("server: cannot listen on " + gameConfig.serverAddress.host + ':'
				   + std::to_string(gameConfig.serverAddress.port) + " - not this machine's address, or a port in use");

		return 1;
	}
	if (launchOptions->portFilePath && !WriteBoundPort(*launchOptions->portFilePath, boundPort))
	{
		Log::Error("server: could not write the port to " + *launchOptions->portFilePath
				   + ", whoever spawned it will not find it");

		return 1;
	}

	Log::Info("server listening on " + gameConfig.serverAddress.host + ':' + std::to_string(boundPort)
			  + ", ctrl+c or /exit to stop, /help for the rest");

	ServerConsole console;
	const ConsoleCommandHandler handler{.events = *events,
										.gameConfig = gameConfig,
										.fpsManager = fpsManager,
										.startedAt = std::chrono::steady_clock::now()};

	while (!isStopRequested.load(std::memory_order_relaxed))
	{
		std::ranges::for_each(console.TakeLines(), [&handler](const std::string& line)
		{
			ApplyConsoleLine(line, handler);
		});

		simulation.Tick();

		simulation.EndNetworkFrame();

		events->EmitEvent(CalculateActualFpsEvent{});
	}

	Log::Info("server stopped");

	return 0;
}
