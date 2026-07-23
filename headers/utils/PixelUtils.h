#pragma once

class PixelUtils final
{
public:
	[[nodiscard]] static unsigned int ChangeAlpha(unsigned int color, const unsigned char alpha)
	{
		// Clear the old alpha channel
		color &= 0x00FFFFFFu;

		// Set the new alpha channel
		color |= (static_cast<unsigned int>(alpha) << 24u);

		return color;
	}

	[[nodiscard]] static unsigned int BlendPixel(const unsigned int src, const unsigned int dst)
	{
		// Extract source pixel color parts
		const unsigned int srcA = (src & 0xFF000000u) >> 24u;
		const unsigned int srcR = (src & 0x00FF0000u) >> 16u;
		const unsigned int srcG = (src & 0x0000FF00u) >> 8u;
		const unsigned int srcB = (src & 0x000000FFu);

		// Extract destination pixel color parts
		const unsigned int dstA = (dst & 0xFF000000u) >> 24u;
		const unsigned int dstR = (dst & 0x00FF0000u) >> 16u;
		const unsigned int dstG = (dst & 0x0000FF00u) >> 8u;
		const unsigned int dstB = (dst & 0x000000FFu);

		// Blend channels
		const unsigned int a = srcA + (dstA * (255u - srcA) + 127u) / 255u;
		const unsigned int r = (srcR * srcA + dstR * dstA * (255u - srcA) / 255u + 127u) / 255u;
		const unsigned int g = (srcG * srcA + dstG * dstA * (255u - srcA) / 255u + 127u) / 255u;
		const unsigned int b = (srcB * srcA + dstB * dstA * (255u - srcA) / 255u + 127u) / 255u;

		// Merge back into a color value
		return a << 24u | r << 16u | g << 8u | b;
	}
};
