//
//  EmuinoSDL.cpp
//  emuinocube
//
//  Created by Henry Tran on 10/10/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#include "EmuinoSDL.h"
#include "EmuinoCube_spi.h"

extern RAMEmulator SharedRAM;
EmuinoSDL emuinocube;

EmuinoSDL::EmuinoSDL()
{
    // nullify everything
    memset(tileSurfaces, NULL, sizeof(&tileSurfaces) * NUM_TILE_LAYERS);
    memset(tileTextures, NULL, sizeof(&tileTextures) * NUM_TILE_LAYERS);
    memset(&tileLayers, NULL, sizeof(TileLayerInfo) * NUM_TILE_LAYERS);
    
    tickToggle = false;
    setup = false;
}

void EmuinoSDL::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(
          "Duinocube Emulator",              // window title
          SDL_WINDOWPOS_UNDEFINED,           // initial x position
          SDL_WINDOWPOS_UNDEFINED,           // initial y position
          320,                               // width, in pixels
          240,                               // height, in pixels
          SDL_WINDOW_OPENGL                  // flags - see below
          );
    renderer = SDL_CreateRenderer(window, -1, 0);

    clickedQuit = false;
}

void EmuinoSDL::postsetup()
{
    TileLayerInfo layer_info;
    
    // set up the palettes
    for (int i = 0; i < NUM_PALETTES; i++) {
        palettes[i] = SDL_AllocPalette(NUM_PALETTE_ENTRIES);
        
        SDL_Color colors[NUM_PALETTE_ENTRIES];
        SharedRAM.read(PALETTE(i) | 0x8000, colors, NUM_PALETTE_ENTRIES * 4);
        
        for (int z = 0; z < NUM_PALETTE_ENTRIES; z++)
            colors[z].a = 255;
        
        SDL_SetPaletteColors(palettes[i], colors, 0, NUM_PALETTE_ENTRIES);
    }
    
    screenSurface = SDL_CreateRGBSurface(0, 320, 240, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 320, 240);
    
    // create the tile textures
    for (int i = 0; i < NUM_TILE_LAYERS; i++) {
        tileMapSurfaces[i] = SDL_CreateRGBSurface(0, 32 * 16, 32 * 16, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        tileMapTextures[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 32 * 16, 32 * 16);
        SDL_SetTextureBlendMode(tileMapTextures[i], SDL_BLENDMODE_BLEND);
    }
    
    setup = true;
}

void EmuinoSDL::quit()
{
    for (int i = 0; i < NUM_PALETTES; i++) {
        if (palettes[i] != NULL)
            SDL_FreePalette(palettes[i]);
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int EmuinoSDL::isActive()
{
    return SDL_GetNumVideoDisplays() > 0;// and !clickedQuit;
}

void EmuinoSDL::tick()
{
    tickToggle = !tickToggle;
    
    uint16_t output_status = (1 << REG_VBLANK);
    
    if (tickToggle) {
        output_status = 0;
    }
    
    SharedRAM.write(REG_OUTPUT_STATUS | 0x8000, &output_status, 2);
    
    // sdl events
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type) {
        case SDL_QUIT:
        {
            clickedQuit = true;
            break;
        }
        case SDL_KEYDOWN:
        {
            //if ( event.key.keysym.sym == SDLK_a )
            // do something
            break;
        }
    }
    
    emuinocube.render();
}

void EmuinoSDL::render()
{
    if (!setup) return;
    
    renderTiles();
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    SDL_UpdateTexture(screenTexture, NULL, screenSurface->pixels, screenSurface->pitch);
    
    // get camera offset
    int16_t camX = 0, camY = 0;
    
    SharedRAM.read(REG_SCROLL_X | 0x8000, &camX, 2);
    SharedRAM.read(REG_SCROLL_Y | 0x8000, &camY, 2);
    
    // render everything on the screen texture such as sprites and tilemaps
    SDL_SetRenderTarget(renderer, screenTexture);
    for (int i = 0; i < NUM_TILE_LAYERS; i++) {
        int16_t offsetX, offsetY;
        int w = tileMapSurfaces[i]->w;
        int h = tileMapSurfaces[i]->h;
        SharedRAM.read(TILE_LAYER_REG(i, TILE_OFFSET_X) | 0x8000, &offsetX, 2);
        SharedRAM.read(TILE_LAYER_REG(i, TILE_OFFSET_Y) | 0x8000, &offsetY, 2);
        
        // add camera offset
        offsetX -= camX;
        offsetY -= camY;
        
        // hacky repetition, will need to be replaced with proper OpenGL texture repetition
        offsetX = offsetX % w;
        if (offsetX < 0)
            offsetX += w;
        
        offsetY = offsetY % h;
        if (offsetY < 0)
            offsetY += w;
        
        SDL_Rect offsetRect = {offsetX, offsetY, w, h};
        SDL_RenderCopy(renderer, tileMapTextures[i], NULL, &offsetRect);
        offsetRect.x -= w;
        SDL_RenderCopy(renderer, tileMapTextures[i], NULL, &offsetRect);
        offsetRect.y -= w;
        SDL_RenderCopy(renderer, tileMapTextures[i], NULL, &offsetRect);
        offsetRect.x = offsetX;
        SDL_RenderCopy(renderer, tileMapTextures[i], NULL, &offsetRect);
    }
    SDL_SetRenderTarget(renderer, NULL);
    
    // render the screen texture
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


// could probably be simpler
// sdl textures have the ability to be flipped using hardware acceleration
void flip(SDL_Surface *surf, bool flipX = false, bool flipY = false, bool flipDiag = false)
{
    int depth = surf->w / surf->pitch; // depth in bytes
    int width = surf->w;
    unsigned char temp[depth];
    
    if (flipX) {
        for (int x = 0; x < surf->w/2; x++)
        {
            for (int y = 0; y < surf->h; y++)
            {
                // tempx, tempy = x, y
                memcpy(temp, (char*)surf->pixels + x*depth + y*width*depth, depth);
                // x, y = x2, y2
                memcpy((char*)surf->pixels + x*depth + y*width*depth, (char*)surf->pixels + (surf->w-x-1)*depth + y*width*depth, depth);
                // x2, y2 = tempx, tempy
                memcpy((char*)surf->pixels + (surf->w-x-1)*depth + y*width*depth, temp, depth);
            }
        }
    }
    
    if (flipY) {
        for (int x = 0; x < surf->w; x++)
        {
            for (int y = 0; y < surf->h/2; y++)
            {
                // tempx, tempy = x, y
                memcpy(temp, (char*)surf->pixels + x*depth + y*width*depth, depth);
                // x, y = x2, y2
                memcpy((char*)surf->pixels + x*depth + y*width*depth, (char*)surf->pixels + x*depth + (surf->h-y-1)*width*depth, depth);
                // x2, y2 = tempx, tempy
                memcpy((char*)surf->pixels + x*depth + (surf->h-y-1)*width*depth, temp, depth);
            }
        }
    }
    
    if (flipDiag) {
        char temp[surf->pitch * surf->h];
        memcpy(temp, surf->pixels, surf->pitch * surf->h);
        
        for (int x = 0; x < surf->w; x++)
        {
            for (int y = 0; y < surf->h; y++)
            {
                // x, y = tempy, tempx
                memcpy((char*)surf->pixels + x*depth + y*width*depth, &temp[y*depth + x*width*depth], depth);
            }
        }
    }
}

void EmuinoSDL::renderTiles()
{
    uint8_t *addr;
    for (int i = 0; i < NUM_TILEMAPS; i++) {
        TileLayerInfo layer_info;
        
        SharedRAM.read(TILE_LAYER_REG(i, TILE_CTRL_0) | 0x8000, &layer_info.ctrl0, 2);
        SharedRAM.read(TILE_LAYER_REG(i, TILE_EMPTY_VALUE) | 0x8000, &layer_info.empty, 2);
        SharedRAM.read(TILE_LAYER_REG(i, TILE_COLOR_KEY) | 0x8000, &layer_info.color_key, 2);
        SharedRAM.read(TILE_LAYER_REG(i, TILE_DATA_OFFSET) | 0x8000, &layer_info.offset, 2);
        
        // clear map surface
        memset(tileMapSurfaces[i]->pixels, 0, tileMapSurfaces[i]->pitch * tileMapSurfaces[i]->h);
        
        if (layer_info.ctrl0 & (1 << TILE_LAYER_ENABLED)) {
            addr = SharedRAM[(TILEMAP_BANK * (0x4000) + 0x8000) + (0x800 * i) - 1];
            for (int y = 0; y < 32; y++) {
                for (int x = 0; x < 32; x++) {
                    uint16_t entry;
                    SharedRAM.read((TILEMAP_BANK * (0x4000) + 0x8000) + (0x800 * i) + (2*(y*32+x)), &entry, 2);
                    uint16_t tile = entry & ~(1 << 15 | 1 << 14 | 1 << 13);
                    
                    //printf("\t0x%x 0x%x\n", entry, tile);
                    
                    if ( tile != layer_info.empty ) {
                    
                        int paletteIndex = (layer_info.ctrl0 >> TILE_PALETTE_START) & 0xFFFF;
                        
                        char tileCopy[16*16];
                        memcpy(tileCopy, SharedRAM[(VRAM_BANK_BEGIN * (0x4000) + layer_info.offset + 0x8000) + (tile * 16*16)], 16*16);
                        
                        SDL_Surface *tileSurface = SDL_CreateRGBSurfaceFrom(tileCopy, 16, 16, 8, 16, 0, 0, 0, 0);
                        
                        flip(tileSurface, entry & (1 << 13), entry & (1 << 14), entry & (1 << 15));
                        
                        SDL_SetSurfacePalette(tileSurface, palettes[paletteIndex]);
                        SDL_SetColorKey(tileSurface, SDL_TRUE, layer_info.color_key);
                        SDL_Rect dst = {x * 16, y * 16, 0, 0};
                        SDL_BlitSurface(tileSurface, NULL, tileMapSurfaces[i], &dst);
                        SDL_FreeSurface(tileSurface);
                        
                    }

                    /*if (entry & (1 << 14))
                        std::cout << "flip y" << std::endl;
                    if (entry & (1 << 15))
                        std::cout << "flip diag" << std::endl;*/
                }
            }
            SDL_UpdateTexture(tileMapTextures[i], NULL, tileMapSurfaces[i]->pixels, tileMapSurfaces[i]->pitch);
        }
    }
}

void EmuinoSDL::checkTile(int tileID)
{
}
