#pragma once

using buuid = boost::uuids::uuid;

template<typename TObstaclesType>
void ObstacleSpawner::SpawnObstacles(const ObjRectangle& rect, const buuid uuid)
{
	if (auto obstacle = std::make_shared<TObstaclesType>(rect, _window, _events, uuid, _gameMode, _textureManager);
		obstacle.get() != nullptr)
	{
		_allObjects->emplace_back(obstacle);
	}
}

template<>
inline void ObstacleSpawner::SpawnObstacles<FortressWall>(const ObjRectangle& rect, const buuid uuid)
{
	if (auto fortressWall = std::make_shared<FortressWall>(
				rect, _window, _events, _allObjects, uuid, _gameMode, _textureManager);
		fortressWall.get() != nullptr)
	{
		_allObjects->emplace_back(fortressWall);
	}
}
