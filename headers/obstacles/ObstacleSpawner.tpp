#pragma once

template<typename TObstaclesType>
void ObstacleSpawner::SpawnObstacles(const ObjRectangle& rect, const int id) {
	_allObjects->emplace_back(std::make_shared<TObstaclesType>(rect, _windowBuffer, _windowSize, _events, id));
}

template<>
inline void ObstacleSpawner::SpawnObstacles<FortressWall>(const ObjRectangle& rect, const int id)
{
	_allObjects->emplace_back(std::make_shared<FortressWall>(rect, _windowBuffer, _windowSize, _events, _allObjects, id));
}

