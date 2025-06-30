#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <variant>
// #include <iostream>

namespace boost::uuids
{
	struct uuid;
}

enum class TankType : char8_t;
enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
enum class Direction : char8_t;
enum class GameMode : char8_t;
struct FPoint;
struct ObjRectangle;
class BaseObj;

template<typename... Args>
struct Event final
{
	using listenerCallback = std::function<void(Args...)>;

	// template<typename... Args>
	void AddListener(const std::string& listenerName, listenerCallback callback)
	{
		_listeners[listenerName] = std::move(callback);
	}


	// template<typename ... TypeArgs>
	// template<typename ... ParamArgs>
	// void Event<TypeArgs...>::Emit(ParamArgs&&... args) {
	// 	for (auto& [_, callback]: _listeners)
	// 	{
	// 		callback(static_cast<TypeArgs>(std::forward<ParamArgs>(args)...));
	// 	}
	// }

	// template<typename... Args>
	void Emit(Args&&... args)
	{
		for (auto& [_, callback]: _listeners)
		{
			//TODO: check proper using forward types should be different event and event args
			//callback(std::forward<EmitArgs>(args)...);
			callback(std::forward<Args>(args)...);
		}
	}

	// template<typename... Args>
	void RemoveListener(const std::string& listenerName) { _listeners.erase(listenerName); }

private:
	std::unordered_map<std::string, listenerCallback> _listeners;
};

class EventSystem final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	using allEventTypes = std::variant<
		Event<>,// regular events eg method call
		Event<const float>,// tickUpdate(deltaTime)
		Event<const int>,// received healthChange(val)
		Event<const bool>,// pause keyStatus
		Event<const GameMode>,// gameMode switch
		Event<const buuid&>,// tankDied, tankSpawn, send/received bonusDeSpawn, send/received bulletDispose
		Event<const std::string&>,// send bonusEffect, animationUpdate
		Event<const BaseObj*>,// draw obj
		Event<const Direction, const buuid&>,// received tankShot(dir,uuid)
		Event<const TankType, const buuid&>,// send/received respawnTank(type,uuid)
		Event<const std::string&, const buuid&>,// send fortressChange(state,uuid)
		Event<const std::string&, const int>,// local respawn resource changed(who,val)
		Event<const std::string&, const bool>,// bonus status effect changed(name/team,isActive)
		Event<const std::string&, const milliseconds>,// bonus effect activates (author/fraction,duration)
		Event<const std::string&, const std::string&>,// (author,fraction) stat, bonusEffect,obstacleDied send/recieved
		Event<const FPoint, const BonusType, const buuid&>,// send/received bonusSpawn(pos,bonusType,uuid)
		Event<const ObjRectangle, const ObstacleType, const buuid&>,// send/received obstacleSpawn(rect,obstType,uuid)
		Event<const FPoint, const Direction, const buuid&>,// received posChange(pos,dir,uuid)
		Event<const std::string&, const int, const buuid&>,// send healthChanged(who,val,uuid),
		Event<const std::string&, const Direction, const buuid&>,// send tankShot(who,dir,uuid)
		Event<const std::string&, const std::string&, const std::string&>,// send/received stat(who,author,fraction)
		Event<const std::string&, const std::string&, const milliseconds>,// bonusEffect(author,fraction,duration)
		Event<const std::string&, const FPoint, const Direction, const buuid&>// send posChange(who,pos,dir,uuid)
	>;

	std::unordered_map<std::string, allEventTypes> _events;

public:
	template<typename... Args>
	void AddListener(const std::string& eventName, const std::string& listenerName, auto callback)
	{
		if (!std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			_events[eventName] = Event<Args...>{};
		}

		std::get<Event<Args...>>(_events[eventName]).AddListener(listenerName, std::move(callback));
	}

	template<typename... Args>
	void EmitEvent(const std::string& eventName, Args&... args)
	{
		if (std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			std::get<Event<Args...>>(_events[eventName]).Emit(std::forward<Args>(args)...);
		}
	}

	template<typename... Args>
	void RemoveListener(const std::string& eventName, const std::string& listenerName)
	{
		if (std::holds_alternative<Event<Args...>>(_events[eventName]))
		{
			std::get<Event<Args...>>(_events[eventName]).RemoveListener(listenerName);
			// std::cout << "[" << "EventSystem::RemoveListener" << "] "
			// 			<< ", eventName=" << eventName
			// 			<< ", listenerName=" << listenerName
			// 			<< std::endl;
		}
	}
};
