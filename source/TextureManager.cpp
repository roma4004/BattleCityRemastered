#include "../headers/TextureManager.h"
#include "SDL.h"
#include "../headers/pawns/Pawn.h"

TextureManager::TextureManager(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<SDL_Texture> texture):
	_renderer(std::move(renderer)),
	_texture(std::move(texture)),
	_offset(PLAYER_ONE) {}


void TextureManager::DrawTexture(BaseObj *obj)
{
	const ObjRectangle shape = obj->GetShape();
	TextureOffset id{ENEMY};
	const SDL_Rect destRect{.x = static_cast<int>(shape.x),
	                    .y = static_cast<int>(shape.y),
	                    .w = static_cast<int>(shape.w),
	                    .h = static_cast<int>(shape.h)};

	//What is it
	const auto name = obj->GetName();
	if (name == "Enemy1" || name == "Enemy2" || name == "Enemy3" || name == "Enemy4")
	{
		id = ENEMY; // 0
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		id = PLAYER_TWO; // 1
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		id = PLAYER_ONE; // 2
	}
	else if (name == "Bullet")
	{
		id = BULLET; // 3
	}
	else if (name == "EAGLE")
	{
		id = EAGLE; // 4
	}
	else
	{
		//TODO all types of objects
	}

	//Source texture (viewport)
	const SDL_Rect textureRect{.x = 0,
						.y = 15 * id,
						.w = 15,
						.h = 15};
	
	//local angle and flip for texture
	double angle = 0.0;
	SDL_RendererFlip flip = SDL_FLIP_NONE;

	if (const auto pawn = dynamic_cast<Pawn*>(obj); pawn != nullptr)
	{
		switch (pawn->GetDirection())
		{
			case 0: //UP
				angle = 0;
				flip = SDL_FLIP_NONE;
				break;
	
			case 1: //LEFT
				angle = -90;
				flip = SDL_FLIP_NONE;
				break;
	
			case 2: //DOWN
				angle = 0;
				flip = SDL_FLIP_VERTICAL;
				break;
	
			case 3: //RIGHT
				angle = 90;
				flip = SDL_FLIP_NONE;
				break;
		}
	}

	SDL_RenderCopyEx(_renderer.get(), _texture.get(), &textureRect, &destRect, angle, nullptr ,flip);
}


