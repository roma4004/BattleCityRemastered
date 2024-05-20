#pragma once

#include "../headers/Pawn.h"

struct FPoint;

class PlayerOne final : public Pawn
{
public:
	PlayerOne(const FPoint& pos, float width, float height, int color, float speed, int health, int* windowBuffer,
			  size_t windowWidth, size_t windowHeight, std::vector<std::shared_ptr<BaseObj>>* allPawns,
			  std::shared_ptr<EventSystem> events);

	~PlayerOne() override;

	void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) override;
};
