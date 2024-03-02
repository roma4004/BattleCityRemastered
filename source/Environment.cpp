#include "../headers/Environment.h"

void Environment::SetPixel(const int x, const int y, const int color) const
{
	if (x >= 0 && x < windowWidth && y >= 0 && y < windowHeight) {
		const int rowSize = windowWidth;
		windowBuffer[y * rowSize + x] = color;
	}
}