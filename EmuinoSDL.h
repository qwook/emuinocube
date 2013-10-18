//
//  EmuinoSDL.h
//  emuinocube
//
//  Created by Henry Tran on 10/10/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#ifndef __emuinocube__EmuinoSDL__
#define __emuinocube__EmuinoSDL__

#include <iostream>
#include <SDL2/SDL.h>

#include "DuinoCube_defs.h"

class EmuinoSDL
{
public:
    EmuinoSDL();
    
    void init();
    void postsetup();
    void quit();
    
    int isActive();
    
    void tick();
    void render();
    
    void renderTiles();
private:
    bool clickedQuit;
    SDL_Window *window;
    SDL_Renderer* renderer;
    SDL_Texture* screenTexture;
    SDL_Surface* screenSurface;
    
    SDL_Surface* tileMapSurfaces[NUM_TILE_LAYERS];
    SDL_Texture* tileMapTextures[NUM_TILE_LAYERS];
    
    bool tickToggle;
    
    struct TileLayerInfo {
        uint16_t ctrl0, empty, color_key, offset;
    };
    
    SDL_Palette *palettes[NUM_PALETTES];
    
    void checkTile(int tileID);
    TileLayerInfo tileLayers[NUM_TILE_LAYERS];
    SDL_Surface *tileSurfaces[NUM_TILE_LAYERS];
    SDL_Texture *tileTextures[NUM_TILE_LAYERS];
    
    bool setup;
};

extern EmuinoSDL emuinocube;

#endif /* defined(__emuinocube__EmuinoSDL__) */
