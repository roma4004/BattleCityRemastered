#pragma once
#include "enums/WindowSide.h"
#include "geometry/Point.h"

struct LaunchOptions;
class ProjectConfig;

//NOTE: the window, the battlefield sizes itself from the map. Read at startup and never written
//afterwards, so everyone but main holds it by const&
class WindowConfig final
{
public:
	explicit WindowConfig(const ProjectConfig& projectConfig);

	void Apply(const LaunchOptions& launchOptions);

	UPoint size{};
	UPoint pos{};
	WindowSide side{WindowSide::Center};
	bool hasExplicitPos{false};//NOTE: explicit pos wins over monitor centering
	bool hasExplicitSize{false};
};
