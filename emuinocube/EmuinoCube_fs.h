//
//  EmuinoCube_fs.h
//  emuinocube
//
//  Created by Henry Tran on 10/9/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#ifndef __emuinocube__EmuinoCube_fs__
#define __emuinocube__EmuinoCube_fs__

#include <cstring>
#include <stdint.h>
#include <stdio.h>

#define FA_READ 0x01
#define FA_WRITE 0x02

class EmuinoCubeFileSystem
{
public:
    
    EmuinoCubeFileSystem();
    
    uint16_t open(const char *fn, const char *mode);
    void close(uint16_t handle);
    uint16_t read(uint16_t handle, void *dst, uint32_t size);
    void seek(uint16_t handle, uint32_t offset);
    uint32_t size(uint16_t handle);
    
    FILE * fhandle(uint16_t);
        
private:
    FILE * filehandles[256]; // hold up to 256 files
};

#endif /* defined(__emuinocube__EmuinoCube_fs__) */
