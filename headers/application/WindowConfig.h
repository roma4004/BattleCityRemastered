#pragma once
#include "enums/WindowSide.h"
#include "geometry/Point.h"

struct LaunchOptions;
class ProjectConfig;

//NOTE: window settings only, the battlefield comes from the map
class WindowConfig final
{
public:
	explicit WindowConfig(const ProjectConfig& projectConfig);

	void Apply(const LaunchOptions& launchOptions);

	UPoint size{};
	UPoint pos{};
	WindowSide side{WindowSide::Center};
	bool hasExplicitPos{};//NOTE: explicit pos wins over monitor centering
	bool hasExplicitSize{};
};
