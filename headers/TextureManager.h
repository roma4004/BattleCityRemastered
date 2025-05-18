#pragma once
#include "BaseObj.h"
#include "enums/TextureOffset.h"
#include "interfaces/ITextureDrawable.h"

#include <SDL_render.h>
#include <memory>



class BaseObj;

class TextureManager : public ITextureDrawable
{
	std::shared_ptr<SDL_Renderer> _renderer;
	std::shared_ptr<SDL_Texture> _texture;
	
	
	
public:
	void DrawTexture(BaseObj* obj) override; 
	
	TextureManager(std::shared_ptr<SDL_Renderer> renderer,std::shared_ptr<SDL_Texture> texture);


	~TextureManager() override = default;

private:
	TextureOffset _offset;
	
};
