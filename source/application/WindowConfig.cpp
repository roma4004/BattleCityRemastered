#include "application/WindowConfig.h"
#include "application/LaunchOptions.h"
#include "application/ProjectConfig.h"
#include "enums/GameMode.h"

WindowConfig::WindowConfig(const ProjectConfig& projectConfig)
{
	size = UPoint{.x = projectConfig.Get<unsigned>("Window.width", 800u),
				  .y = projectConfig.Get<unsigned>("Window.height", 600u)};

	pos = UPoint{.x = projectConfig.Get<unsigned>("Window.posX", 100u),
				 .y = projectConfig.Get<unsigned>("Window.posY", 100u)};
}

void WindowConfig::Apply(const LaunchOptions& launchOptions)
{
	//NOTE: size first - the host/client offset below is half a window wide
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

	//NOTE: two windows on one screen - the pair is pushed apart instead of landing on top of each other
	if (IsHost(launchOptions.gameMode))
	{
		posOffset.x -= size.x / 2;
	}
	else if (IsClient(launchOptions.gameMode))
	{
		posOffset.x += size.x / 2;
	}
}
