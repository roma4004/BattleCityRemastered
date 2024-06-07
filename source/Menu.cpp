#include "../headers/Menu.h"

#include "../headers/Point.h"

Menu::Menu(const UPoint windowSize, int* windowBuffer)
: _windowSize{windowSize}, _windowBuffer{windowBuffer}, yOffsetStart{static_cast<unsigned int>(_windowSize.y)} {
}


inline unsigned int ChangeAlpha(unsigned int color, unsigned char alpha)
{
	// Очистите старый альфа-канал
	color &= 0x00FFFFFF;

	// Установите новый альфа-канал
	color |= (static_cast<int>(alpha) << 24);

	return color;
}

inline unsigned int BlendPixel(const unsigned int src, const unsigned int dst)
{
	const unsigned int srcA = (src & 0xFF000000) >> 24;
	const unsigned int srcR = (src & 0x00FF0000) >> 16;
	const unsigned int srcG = (src & 0x0000FF00) >> 8;
	const unsigned int srcB = (src & 0x000000FF);

	const unsigned int dstA = (dst & 0xFF000000) >> 24;
	const unsigned int dstR = (dst & 0x00FF0000) >> 16;
	const unsigned int dstG = (dst & 0x0000FF00) >> 8;
	const unsigned int dstB = (dst & 0x000000FF);

	const unsigned int a = srcA + (dstA * (255 - srcA) + 127) / 255;
	const unsigned int r = (srcR * srcA + dstR * dstA * (255 - srcA) / 255 + 127) / 255;
	const unsigned int g = (srcG * srcA + dstG * dstA * (255 - srcA) / 255 + 127) / 255;
	const unsigned int b = (srcB * srcA + dstB * dstA * (255 - srcA) / 255 + 127) / 255;

	return a << 24 | r << 16 | g << 8 | b;
}

// blend menu panel and menu texture background
void Menu::BlendToWindowBuffer()
{
	const unsigned int sizeX = static_cast<unsigned int>(_windowSize.x);
	const unsigned menuHeight = static_cast<unsigned int>(_windowSize.y) - 50;
	const unsigned menuWidth = sizeX - 228;
	for (_pos.y = 50 + yOffsetStart; _pos.y < menuHeight + yOffsetStart; ++_pos.y)
	{
		for (_pos.x = 50; _pos.x < menuWidth; ++_pos.x)
		{
			if (_pos.y < _windowSize.y && _pos.x < _windowSize.x)
			{
				constexpr unsigned int backgroundColor = 0xFF808080;
				int& srcColor = _windowBuffer[_pos.y * sizeX + _pos.x];
				srcColor = static_cast<int>(
					BlendPixel(
						ChangeAlpha(static_cast<unsigned int>(_windowBuffer[_pos.y * sizeX + _pos.x]), 91),
						backgroundColor
					)
				);
			}
		}
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0; yOffsetStart > yOffsetEnd)
	{
		yOffsetStart -= 3;
	}
}