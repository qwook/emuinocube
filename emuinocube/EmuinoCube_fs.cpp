//
//  EmuinoCube_fs.cpp
//  emuinocube
//
//  Created by Henry Tran on 10/9/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#include "EmuinoCube_fs.h"

EmuinoCubeFileSystem filesystem;

EmuinoCubeFileSystem::EmuinoCubeFileSystem()
{
    memset(filehandles, NULL, 256);
}

uint16_t EmuinoCubeFileSystem::open(const char *fn, const char *mode)
{
    for (char i = 0; i < 256; i++) {
        if (filehandles[i] == NULL) {
            FILE *fp = fopen(fn, mode);
            if (fp) {
                filehandles[i] = fp;
                return i+1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}

FILE *EmuinoCubeFileSystem::fhandle(uint16_t handle) {
    return filehandles[handle-1];
}

void EmuinoCubeFileSystem::close(uint16_t handle) {
    fclose(filehandles[handle-1]);
    filehandles[handle-1] = NULL;
}

uint16_t EmuinoCubeFileSystem::read(uint16_t handle, void *dst, uint32_t size)
{
    return fread(dst, 1, size, filehandles[handle-1]);
}

uint32_t EmuinoCubeFileSystem::size(uint16_t handle) {
    FILE *f = filehandles[handle-1];
    fpos_t pos;
    fgetpos(f, &pos);
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fsetpos(f, &pos);
    return size;
}
