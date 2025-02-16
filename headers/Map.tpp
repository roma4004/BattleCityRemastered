#pragma once

#include "Map.h"

template<typename T>
void Map::ObstacleCreation(std::vector<std::shared_ptr<BaseObj>>* allObjects, const ObjRectangle& rect,
                           std::shared_ptr<int[]> windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
                           const int obstacleId) const
{
	allObjects->emplace_back(
			std::make_shared<T>(rect, std::move(windowBuffer), windowSize, std::move(events), obstacleId));
}

template<>
inline void Map::ObstacleCreation<FortressWall>(std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                                    const ObjRectangle& rect, std::shared_ptr<int[]> windowBuffer,
                                                    UPoint windowSize, std::shared_ptr<EventSystem> events,
                                                    const int obstacleId) const
{
	allObjects->emplace_back(
			std::make_shared<FortressWall>(
					rect, std::move(windowBuffer), windowSize, std::move(events), allObjects, obstacleId));
}
