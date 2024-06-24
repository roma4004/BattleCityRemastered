#pragma once

#include "InputProviderForMenu.h"
#include "Point.h"

class Menu
{
	UPoint _windowSize;
	int* _windowBuffer;
	unsigned int yOffsetStart;

public:
	Menu(UPoint windowSize, int* windowBuffer, std::unique_ptr<InputProviderForMenu>& input);

	~Menu() = default;

	void BlendToWindowBuffer();

	std::unique_ptr<InputProviderForMenu> input;
	UPoint _pos;
};
