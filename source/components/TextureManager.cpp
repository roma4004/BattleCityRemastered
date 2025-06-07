#include "../../headers/components/TextureManager.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/pawns/Pawn.h"
#include <SDL.h>

TextureManager::TextureManager(std::shared_ptr<SDL_Texture> texture, std::shared_ptr<SDL_Renderer> renderer):
	_renderer(std::move(renderer)),
	_texture(std::move(texture)) {}

SDL_Rect TextureManager::RectToSdlRect(const ObjRectangle& rect)
{
	return SDL_Rect {
		static_cast<int>(rect.x),
		static_cast<int>(rect.y),
		static_cast<int>(rect.w),
		static_cast<int>(rect.h)};
}

TextureManager::~TextureManager() {}

void TextureManager::Draw(const BaseObj* obj) const
{
	const ObjRectangle rect = obj->GetRect();
	const SDL_Rect destRect = RectToSdlRect(rect);

	//What is it
	const auto name = obj->GetName();
	SDL_Rect textureRect{};
	const auto pawn = dynamic_cast<const Pawn*>(obj);
	if (name == "Enemy")
	{
		textureRect = RectToSdlRect(_offset.enemy);
		textureRect.x += pawn->_animationId * 13;
	}
	else if (name == "Player1" || name == "CoopBot1")
	{
		textureRect = RectToSdlRect(_offset.playerOne);
		textureRect.x += pawn->_animationId * 13;
	}
	else if (name == "Player2" || name == "CoopBot2")
	{
		textureRect = RectToSdlRect(_offset.playerTwo);
		textureRect.x += pawn->_animationId * 13;
	}
	else if (name == "Bullet")
	{
		textureRect = RectToSdlRect(_offset.bullet);
	}
	else if (name == "Eagle")
	{
		textureRect = RectToSdlRect(_offset.eagle);
	}
	else if (name == "BrickWall")
	{
		textureRect = RectToSdlRect(_offset.brick);
	}
	else if (name == "SteelWall")
	{
	}
	else if (name == "WaterTile")
	{
	}
	else
	{
		//TODO all types of objects
	}

	//local angle and flip for texture
	double angle = 0.0;
	SDL_RendererFlip flip = SDL_FLIP_NONE;

	if (pawn != nullptr)
	{
		switch (pawn->GetDirection())
		{
			case UP:
				angle = 0;
				flip = SDL_FLIP_NONE;
				break;

			case LEFT:
				angle = -90;
				flip = SDL_FLIP_NONE;
				break;

			case DOWN:
				angle = 0;
				flip = SDL_FLIP_VERTICAL;
				break;

			case RIGHT:
				angle = 90;
				flip = SDL_FLIP_NONE;
				break;
		}
	}

	SDL_RenderCopyEx(_renderer.get(), _texture.get(), &textureRect, &destRect, angle, nullptr, flip);
}
