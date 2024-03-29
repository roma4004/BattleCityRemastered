#include "../headers/Environment.h"

void Event::AddListener(const std::string& listenerName, const std::function<void()>& callback)
{
	listeners[listenerName] = callback;
}

void Event::Emit()
{
	for (const auto& callback : listeners | std::views::values)
	{
		if (callback != nullptr)
		{
			callback();
		}
	}
}

void Event::RemoveListener(const std::string& listenerName)
{
	listeners.erase(listenerName);
}

void EventSystem::AddEvent(const std::string& eventName)
{
	events[eventName] = Event{.name = eventName};
}

void EventSystem::AddListenerToEvent(const std::string& eventName, const std::string& listenerName,
									 const std::function<void()>& callback)
{
	events[eventName].AddListener(listenerName, callback);
}

void EventSystem::EmitEvent(const std::string& eventName)
{
	if (const auto it = events.find(eventName); it != events.end())
	{
		it->second.Emit();
	}
}

void EventSystem::RemoveListenerFromEvent(const std::string& eventName, const std::string& listenerName)
{
	if (const auto it = events.find(eventName); it != events.end())
	{
		events[eventName].RemoveListener(listenerName);
	}
}

void Environment::SetPixel(const int x, const int y, const int color) const
{
	if (x >= 0 && x < windowWidth && y >= 0 && y < windowHeight)
	{
		const int rowSize = windowWidth;
		windowBuffer[y * rowSize + x] = color;
	}
}
