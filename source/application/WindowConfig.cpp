#include "application/WindowConfig.h"
#include "application/LaunchOptions.h"
#include "application/ProjectConfig.h"
#include "enums/GameMode.h"
#include "enums/WindowSide.h"

WindowConfig::WindowConfig(const ProjectConfig& projectConfig)
{
	size = UPoint{.x = projectConfig.Get<unsigned>("Window.width", 800u),
				  .y = projectConfig.Get<unsigned>("Window.height", 600u)};

	pos = UPoint{.x = projectConfig.Get<unsigned>("Window.posX", 100u),
				 .y = projectConfig.Get<unsigned>("Window.posY", 100u)};
}

void WindowConfig::Apply(const LaunchOptions& launchOptions)
{
	if (launchOptions.windowSize)
	{
		size = *launchOptions.windowSize;
		hasExplicitSize = true;
	}

	if (launchOptions.windowPos)
	{
		pos = *launchOptions.windowPos;
		hasExplicitPos = true;
	}

	//NOTE: with a dedicated server both processes are clients, so the mode alone puts them on one half
	if (launchOptions.windowSide)
	{
		side = *launchOptions.windowSide;
	}
	else if (IsHost(launchOptions.gameMode))
	{
		side = WindowSide::Left;
	}
	else if (IsClient(launchOptions.gameMode))
	{
		side = WindowSide::Right;
	}
}
