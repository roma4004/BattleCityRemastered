#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <variant>
#include <boost/uuid/uuid.hpp>//TODO: add uuid lib to cmake

enum TankType : char8_t;
enum ObstacleType : char8_t;
enum BonusType : char8_t;
enum Direction : char8_t;
enum GameMode : char8_t;
struct FPoint;
struct ObjRectangle;

template<typename... Args>
struct Event final
{
	using listenerCallback = std::function<void(Args...)>;

	void AddListener(const std::string& listenerName, listenerCallback callback);

	void Emit(Args&&... args);

	void RemoveListener(const std::string& listenerName);

private:
	std::unordered_map<std::string, listenerCallback> _listeners;
};

class EventSystem final
{
	using milliseconds = std::chrono::milliseconds;
	using uuid = boost::uuids::uuid;//TODO: apply this using to solution
	using allEventTypes = std::variant<
		Event<>,// regular events eg method call
		Event<const float>,// tickUpdate(deltaTime)
		Event<const int>,// received healthChange(val)
		Event<const bool>,// pause keyStatus
		Event<const GameMode>,// gameMode switch
		Event<const uuid&>,//tankDied, tankSpawn, send/received bonusDeSpawn, send/received bulletDispose
		Event<const std::string&>,//send bonusEffect
		Event<const Direction, const uuid>,// received tankShot(dir,uuid)
		Event<const TankType, const uuid>,// send/received respawnTank(type,uuid)
		Event<const std::string&, const uuid>,// send fortressChange(state,uuid)
		Event<const std::string&, const int>,// local respawn resource changed(who,val)
		Event<const std::string&, const std::string&>,//(author,fraction) stat, bonusEffect, obstacleDied send/recieved
		Event<const FPoint, const BonusType, const uuid>,// send/received bonusSpawn(pos,bonusType,uuid)
		Event<const ObjRectangle, const ObstacleType, const uuid>,// send/received obstacleSpawn(rect,obstacleType,uuid)
		Event<const FPoint, const Direction, const uuid>,// received posChange(pos,dir,uuid)
		Event<const std::string&, const int, const uuid>,// send healthChanged(who,val,uuid),
		Event<const std::string&, const Direction, const uuid>,// send tankShot(who,dir,uuid)
		Event<const std::string&, const std::string&, const std::string&>,//send/recieved stat(who,author,fraction)
		Event<const std::string&, const std::string&, const milliseconds>,//bonusEffect(author,fraction,duration)
		Event<const std::string&, const FPoint, const Direction, const uuid>//send posChange(who,pos,dir,uuid)
	>;

	std::unordered_map<std::string, allEventTypes> _events;

public:
	template<typename... Args>
	void AddListener(const std::string& eventName, const std::string& listenerName, auto callback);

	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&... args);

	template<typename... Args>
	void RemoveListener(const std::string& eventName, const std::string& listenerName);
};

// Include the template implementation
#include "EventSystem.tpp"
