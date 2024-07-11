#pragma once

class PixelUtils
{
public:
	static unsigned int ChangeAlpha(unsigned int color, const unsigned char alpha)
	{
		// Очистите старый альфа-канал
		color &= 0x00FFFFFF;

		// Установите новый альфа-канал
		color |= (static_cast<int>(alpha) << 24);

		return color;
	}

	static unsigned int BlendPixel(const unsigned int src, const unsigned int dst)
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
};
