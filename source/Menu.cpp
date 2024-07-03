#include "../headers/Menu.h"
#include "../headers/PixelUtils.h"
#include "../headers/Point.h"

Menu::Menu(const UPoint windowSize, int* windowBuffer, std::unique_ptr<InputProviderForMenu>& input)
	: _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  yOffsetStart{static_cast<unsigned int>(_windowSize.y)},
	  input{std::move(input)} {}

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
				constexpr unsigned int menuColor = 0xFF808080;
				int& targetColor = _windowBuffer[_pos.y * sizeX + _pos.x];
				unsigned int targetColorLessAlpha = PixelUtils::ChangeAlpha(static_cast<unsigned int>(targetColor), 91);
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColorLessAlpha, menuColor));
			}
		}
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0; yOffsetStart > yOffsetEnd)
	{
		yOffsetStart -= 3;
	}
}
