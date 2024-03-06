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
	std::string Name;
	std::map<std::string, std::function<void()>> Listeners;

	void AddListener(const std::string& listenerName, const std::function<void()>& callback);

	void Emit();

	void RemoveListener(const std::string& listenerName);
};

struct EventSystem
{
	std::map<std::string, Event> Events;

	void AddEvent(const std::string& eventName);

	void AddListenerToEvent(const std::string& eventName, const std::string& listenerName,
							const std::function<void()>& callback);

	void EmitEvent(const std::string& eventName);

	void RemoveListenerFromEvent(const std::string& eventName, const std::string& listenerName);
};

class Pawn;

struct Environment
{
	int WindowWidth = 640;
	int WindowHeight = 480;
	int* WindowBuffer{};
	SDL_Event Event{};
	SDL_Window* Window{};
	SDL_Renderer* Renderer{};
	SDL_Texture* Screen{};
	bool IsGameOver = false;

	void SetPixel(int x, int y, int color) const;

	MouseButtons MouseButtons;
	std::vector<Pawn*> AllPawns;
	std::list<Pawn*> PawnsToDestroy;

	EventSystem Events;

};