#pragma once

#include "SDL.h"
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <vector>

#include "../headers/MouseButton.h"
#include "../headers/Pawn.h"

#include <execution>
#include <functional>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <vector>

struct Event
{
	std::string name;
	std::map<std::string, std::function<void()>> listeners;

	void AddListener(const std::string& listenerName, const std::function<void()>& callback);

	void Emit();

	void RemoveListener(const std::string& listenerName);
};

struct EventSystem
{
	std::map<std::string, Event> events;

	void AddEvent(const std::string& eventName);

	void AddListenerToEvent(const std::string& eventName, const std::string& listenerName,
							const std::function<void()>& callback);

	void EmitEvent(const std::string& eventName);

	void RemoveListenerFromEvent(const std::string& eventName, const std::string& listenerName);
};

class Pawn;

struct Environment
{
	int windowWidth = 640;
	int windowHeight = 480;
	int* windowBuffer{};
	SDL_Event event{};
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* screen{};
	bool isGameOver = false;

	//fps
	float deltaTime = 0.f;

	void SetPixel(int x, int y, int color) const;

	MouseButtons mouseButtons;
	std::vector<BaseObj*> allPawns;
	std::list<BaseObj*> pawnsToDestroy;
	
	
	EventSystem events;

};