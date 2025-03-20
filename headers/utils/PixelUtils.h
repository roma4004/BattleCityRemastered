#pragma once

class PixelUtils final
{
public:
	[[nodiscard]] static unsigned int ChangeAlpha(unsigned int color, const unsigned char alpha)
	{
		// Clear the old alpha channel
		color &= 0x00FFFFFF;

		// Set the new alpha channel
		color |= (static_cast<int>(alpha) << 24);

		return color;
	}

	[[nodiscard]] static unsigned int BlendPixel(const unsigned int src, const unsigned int dst)
	{
		// Extract source pixel color parts
		const unsigned int srcA = (src & 0xFF000000) >> 24;
		const unsigned int srcR = (src & 0x00FF0000) >> 16;
		const unsigned int srcG = (src & 0x0000FF00) >> 8;
		const unsigned int srcB = (src & 0x000000FF);

		// Extract destination pixel color parts
		const unsigned int dstA = (dst & 0xFF000000) >> 24;
		const unsigned int dstR = (dst & 0x00FF0000) >> 16;
		const unsigned int dstG = (dst & 0x0000FF00) >> 8;
		const unsigned int dstB = (dst & 0x000000FF);

		// Blend channels
		const unsigned int a = srcA + (dstA * (255 - srcA) + 127) / 255;
		const unsigned int r = (srcR * srcA + dstR * dstA * (255 - srcA) / 255 + 127) / 255;
		const unsigned int g = (srcG * srcA + dstG * dstA * (255 - srcA) / 255 + 127) / 255;
		const unsigned int b = (srcB * srcA + dstB * dstA * (255 - srcA) / 255 + 127) / 255;

		// Merge back into a color value
		return a << 24 | r << 16 | g << 8 | b;
	}
};
