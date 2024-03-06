#include "../headers/Environment.h"

void Event::AddListener(const std::string& listenerName, const std::function<void()>& callback)
{
	Listeners[listenerName] = callback;
}

void Event::Emit()
{
	for (auto& callback: Listeners | std::views::values) {
		if (callback != nullptr) {
			callback();
		}
	}
}

void Event::RemoveListener(const std::string& listenerName)
{
	Listeners.erase(listenerName);
}

void EventSystem::AddEvent(const std::string& eventName)
{
	Events[eventName] = Event{.Name = eventName};
}

void EventSystem::AddListenerToEvent(const std::string& eventName, const std::string& listenerName,
									 const std::function<void()>& callback)
{
	Events[eventName].AddListener(listenerName, callback);
}

void EventSystem::EmitEvent(const std::string& eventName)
{
	if (const auto it = Events.find(eventName); it != Events.end()) {
		it->second.Emit();
	}
}

void EventSystem::RemoveListenerFromEvent(const std::string& eventName, const std::string& listenerName)
{
	if (const auto it = Events.find(eventName); it != Events.end()) {
		Events[eventName].RemoveListener(listenerName);
	}

}

void Environment::SetPixel(const int x, const int y, const int color) const
{
	if (x >= 0 && x < WindowWidth && y >= 0 && y < WindowHeight) {
		const int rowSize = WindowWidth;
		WindowBuffer[y * rowSize + x] = color;
	}
}