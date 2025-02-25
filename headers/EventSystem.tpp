#pragma once

template<typename ... Args>
void Event<Args...>::AddListener(const std::string& listenerName, listenerCallback callback) {
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

template<typename ... Args>
void Event<Args...>::Emit(Args&&... args) {
	for (auto& [_, callback]: _listeners)
	{
		//TODO: check proper using forward types should be diferent event and event args
		//callback(std::forward<EmitArgs>(args)...);
		callback(std::forward<Args>(args)...);
	}
}

template<typename ... Args>
void Event<Args...>::RemoveListener(const std::string& listenerName) { _listeners.erase(listenerName); }

template<typename ... Args>
void EventSystem::AddListener(const std::string& eventName, const std::string& listenerName, auto callback) {
	if (std::holds_alternative<Event<Args...>>(_events[eventName]))
	{
		std::get<Event<Args...>>(_events[eventName]).AddListener(listenerName, std::move(callback));
	}
	else
	{
		_events[eventName] = Event<Args...>{};
		std::get<Event<Args...>>(_events[eventName]).AddListener(listenerName, std::move(callback));
	}
}

template<typename ... Args>
void EventSystem::EmitEvent(const std::string& eventName, Args&... args) {
	if (std::holds_alternative<Event<Args...>>(_events[eventName]))
	{
		std::get<Event<Args...>>(_events[eventName]).Emit(std::forward<Args>(args)...);
	}
}

template<typename ... Args>
void EventSystem::RemoveListener(const std::string& eventName, const std::string& listenerName) {
	if (std::holds_alternative<Event<Args...>>(_events[eventName]))
	{
		std::get<Event<Args...>>(_events[eventName]).RemoveListener(listenerName);
	}
}
