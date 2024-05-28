#pragma once

#include "../headers/Pawn.h"

struct FPoint;

class PlayerTwo final : public Pawn
{
public:
	PlayerTwo(const FPoint& pos, float width, float height, int color, float speed, int health, int* windowBuffer,
			  size_t windowWidth, size_t windowHeight, std::vector<std::shared_ptr<BaseObj>>* allPawns,
			  std::shared_ptr<EventSystem> events);

	~PlayerTwo() override;

	void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) override;
};
