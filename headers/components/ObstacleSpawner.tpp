#pragma once

template<typename TObstaclesType>
void ObstacleSpawner::SpawnObstacles(const ObjRectangle& rect, const int id)
{
	_allObjects->emplace_back(std::make_shared<TObstaclesType>(rect, _window, _events, id, _gameMode));
}

template<>
inline void ObstacleSpawner::SpawnObstacles<FortressWall>(const ObjRectangle& rect, const int id)
{
	_allObjects->emplace_back(std::make_shared<FortressWall>(rect, _window, _events, _allObjects, id, _gameMode));
}
