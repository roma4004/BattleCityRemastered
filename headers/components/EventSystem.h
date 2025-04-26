#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <variant>

enum ObstacleType : char8_t;
enum BonusType : char8_t;
enum Direction : char8_t;
enum GameMode : char8_t;
struct FPoint;

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
	using allEventTypes = std::variant<
		Event<>,// regular events eg method call
		Event<const float>,// update(deltaTime)
		Event<const FPoint>,// bullets tank new pos,
		Event<const FPoint, const int>,// bullets send new pos, bulletId
		Event<const FPoint, const BonusType, const int>,// received bonusSpawn
		Event<const FPoint, const ObstacleType, const int>,// received bonusSpawn
		Event<const FPoint, const int, const Direction>,// bullets send new pos, bulletId, direction
		Event<const FPoint, const Direction>,// received newPos and direction
		Event<const std::string&, const FPoint, const Direction>,// send new pos and direction
		Event<const std::string&, const FPoint, const BonusType, const int>,// send bonusSpawn
		Event<const Direction>,// received tank shot
		Event<const std::string&, const Direction>,// send tank shot
		Event<const int>,// serverSend bonusDeSpawn, clientReceived onHealthChange
		Event<const int, const int>,// clientReceived health changed
		Event<const std::string&>,// send/receive onGrenade
		Event<const std::string&, const int>,// serverSend health changed, respawn resource changed
		Event<const std::string&, const std::string&>,// statistics, bonus effect
		Event<const std::string&, const std::string&, const std::string&>,// serverSend statistics, local statistics
		Event<const std::string&, const std::string&, const std::chrono::milliseconds>,// bonus with duration
		Event<const GameMode>,// gameMode switch
		Event<const bool>// pause status
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
