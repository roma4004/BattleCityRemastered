#pragma once

template<typename TObstaclesType>
void ObstacleSpawner::SpawnObstacles(const ObjRectangle& rect, const boost::uuids::uuid uuid)
{
	_allObjects->emplace_back(std::make_shared<TObstaclesType>(rect, _window, _events, uuid, _gameMode));
}

template<>
inline void ObstacleSpawner::SpawnObstacles<FortressWall>(const ObjRectangle& rect, const boost::uuids::uuid uuid)
{
	_allObjects->emplace_back(std::make_shared<FortressWall>(rect, _window, _events, _allObjects, uuid, _gameMode));
}
