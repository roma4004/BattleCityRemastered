#pragma once

#include <functional>
#include <string>
#include <variant>

enum Direction : char8_t;
enum GameMode : char8_t;
struct FPoint;

template<typename... Args>
struct Event final
{
	using listenerCallback = std::function<void(Args...)>;

	void AddListener(const std::string& listenerName, listenerCallback callback);

	// TODO: template<typename... EmitArgs>
	// void Emit(EmitArgs&&... args)
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
		Event<const FPoint, const int, const Direction>,// bullets send new pos, bulletId, direction
		Event<const FPoint, const Direction>,// send newPos and direction
		Event<const Direction>,// send tank shot
		Event<const int>,// respawn resource changed
		Event<const int, const int>,// send health changed
		Event<const std::string, const int>,// send health changed
		Event<const std::string, const std::string, const int>,// send health changed
		Event<const std::string, const std::string, const FPoint, const Direction>,// send new pos and direction
		Event<const std::string&, const std::string&>,// statistics, bonus
		Event<const std::string&, const std::string&, int>,// bonus with duration
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
