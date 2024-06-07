#pragma once

#include "Point.h"
#include "Rectangle.h"

class Menu
{
	UPoint _windowSize;
	int* _windowBuffer;
	unsigned int yOffsetStart;

public:
	UPoint _pos;

	Menu(UPoint windowSize, int* windowBuffer);

	~Menu() = default;

	void BlendToWindowBuffer();
};
