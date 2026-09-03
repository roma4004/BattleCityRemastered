#pragma once

//NOTE: what happens to a line's own pos - the two are different layouts, not a flag on one
enum class TextBlockAlign : char8_t
{
	//NOTE: each line lands where it says; the panel only caps the point size
	LinePositions,
	//NOTE: lines are stacked in the middle of the panel and their pos is not read at all
	CenteredInPanel
};
