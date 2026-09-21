#pragma once

enum class TextBlockAlign : char8_t
{
	//NOTE: the lines keep the layout they were given and the block moves as one, so that what is left of
	//it and what is right of it inside the panel come out the same
	CenteredBlock,
	//NOTE: lines are stacked in the middle of the panel and their pos is not read at all
	CenteredInPanel
};
