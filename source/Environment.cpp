#include "../headers/Environment.h"

void Environment::SetPixel(const int x, const int y, const int color) const
{
	if (x >= 0 && x < WindowWidth && y >= 0 && y < WindowHeight) {
		const int rowSize = WindowWidth;
		WindowBuffer[y * rowSize + x] = color;
	}
}