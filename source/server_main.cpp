#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/managers/FramePerSecondManager.h"
#include "enums/GameMode.h"
#include "utils/Log.h"
#include <atomic>
#include <csignal>
#include <memory>

namespace
{
std::atomic_bool isStopRequested{false};

extern "C" void OnStopSignal(int) { isStopRequested.store(true, std::memory_order_relaxed); }

}//namespace

//NOTE: the same Simulation the game runs, minus the screen - it links GameCore alone, so SDL cannot
//reach it. A spawning tank still lands: PostTickUpdate advances the animation without draw phases
int main()
{
	Log::SetConsole(true);
	Log::SetFile(true);
	Log::SetLevel(Log::Level::Normal);

	std::signal(SIGINT, OnStopSignal);
	std::signal(SIGTERM, OnStopSignal);

	ProjectConfig projectConfig{ProjectConfig::DefaultFilePath()};
	if (const auto& configError = projectConfig.LoadError())
	{
		Log::Error("config " + configError->path.string() + " line " + std::to_string(configError->line) + ": "
				   + configError->reason + ", running on defaults and leaving the file untouched");
	}

	GameConfig gameConfig{};

	const auto events = std::make_shared<EventSystem>();
	//NOTE: no presenter here, so its pacing is all that stands between this loop and a busy spin
	const FramePerSecondManager fpsManager{events, projectConfig, false};
	Simulation simulation{events, gameConfig};

	simulation.ApplyGameMode(GameMode::PlayAsHost);

	Log::Info("server listening, ctrl+c to stop");

	while (!isStopRequested.load(std::memory_order_relaxed))
	{
		simulation.Tick();

		simulation.EndNetworkFrame();

		events->EmitEvent(CalculateActualFpsEvent{});
	}

	Log::Info("server stopped");

	return 0;
}
