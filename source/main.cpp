#include "../headers/Environment.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"
#include <cstdio>
#include <iostream>

static void MouseEvents(Environment& env, const SDL_Event& event)
{
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        env.mouseButtons.MouseLeftButton = true;
        std::cout << "MouseLeftButton: " << "Down" << '\n';

        return;
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT){
        env.mouseButtons.MouseLeftButton = false;
        std::cout << "MouseLeftButton: " << "Up" << '\n';

        return;
    }

    if (event.type == SDL_MOUSEMOTION && env.mouseButtons.MouseLeftButton) {
        const Sint32 x = event.motion.x;
        const Sint32 y = event.motion.y;
        std::cout << "x: " << x <<  " \t y: " << y << '\n';
        const int rowSize = env.windowWidth;

        if (x < 1 || y < 1 || x >= env.windowWidth - 1 && y >= env.windowHeight - 1) {
            return;
        }
    }
}

void ClearBuffer(const Environment& env)
{
    for (int y = 0; y < env.windowHeight; y++)
    {
        for (int x = 0; x < env.windowWidth; x++)
        {
            env.SetPixel(x, y, 0x0);
        }
    }
}

int Init(Environment& env)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return 1;
    }

    env.window = SDL_CreateWindow("Battle City remastered", 100, 100, env.windowWidth, env.windowHeight, SDL_WINDOW_SHOWN);
    if (env.window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        return 1;
    }

    env.renderer = SDL_CreateRenderer(env.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (env.renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        return 1;
    }

    env.screen = SDL_CreateTexture(env.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, env.windowWidth, env.windowHeight);
    if (env.screen == nullptr) {
        std::cerr << "Screen SDL_CreateTexture Error: " << SDL_GetError() << '\n';
        return 1;
    }

    const int bufferSize = env.windowWidth * env.windowHeight;
    env.windowBuffer = new int[bufferSize];

    return 0;
}

int main(int argc, char* argv[])
{
    Environment env;
    constexpr int speed = 4;
    PlayerOne playerOne { 20, 20, 100, 100, 0x00ff00, speed, 1 };
    PlayerTwo playerTwo { 200, 200, 100, 100, 0xff0000, speed, 2 };

    env.allPawns.reserve(2);
    env.allPawns.emplace_back(&playerOne);
    env.allPawns.emplace_back(&playerTwo);
       
    Init(env);
    while (!env.isGameOver)
    {
        ClearBuffer(env);

        // event handling
        while (SDL_PollEvent(&env.event))
        {
            if (env.event.type == SDL_QUIT) {
                env.isGameOver = true;
            }

            MouseEvents(env, env.event);

            for (auto* pawn : env.allPawns){
                pawn->KeyboardEvensHandlers(env, env.event.type, env.event.key.keysym.sym);
            }
        }

        // physics handling
        for (size_t i = 0; i < env.allPawns.size(); ++i){
            Pawn* pawn = env.allPawns[i];
            pawn->TickUpdate(env);
        }

        // Destroy all dead objects
        for (size_t i = 0; i < env.allPawns.size(); ++i){
            Pawn* pawn = env.allPawns[i];
            if (!pawn->GetIsAlive())
            {
                pawn->Destroy(env);
            }
        }
        
        // draw handling
        for (size_t i = 0; i < env.allPawns.size(); ++i){
            Pawn* pawn = env.allPawns[i];
            pawn->Draw(env);
        }

        // update screen with buffer
        SDL_UpdateTexture(env.screen, nullptr, env.windowBuffer, env.windowWidth << 2);
        SDL_RenderCopy(env.renderer, env.screen, nullptr, nullptr);

        SDL_RenderPresent(env.renderer);
    }

    SDL_DestroyRenderer(env.renderer);
    SDL_DestroyWindow(env.window);

    delete env.windowBuffer;

    SDL_Quit();

    return 0;
}
