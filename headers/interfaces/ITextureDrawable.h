#pragma once


class BaseObj;

class ITextureDrawable
{
public:
	virtual void DrawTexture(BaseObj* obj) = 0;
	virtual ~ITextureDrawable() = default;
};
