//
//  EmuinoCube_spi.cpp
//  emuinocube
//
//  Created by Henry Tran on 10/7/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#include "EmuinoCube_spi.h"
#include "EmuinoCube_fs.h"
#include "EmuinoSDL.h"

#include "DuinoCube_system.h"
#include "DuinoCube_rpc.h"
#include "DuinoCube_rpc_file.h"

// I'm still trying to learn about the SPI bus so please excuse me
// if this doesn't look like it emulate the SPI bus correctly.
// todo: everything in this file needs to be split up into different files

SPIEmulator SPI;
extern EmuinoCubeFileSystem filesystem;

#define DEFAULT_CORE_SS_PIN      5
#define DEFAULT_SYS_SS_PIN       4

#define SPI_BUS_STATE_NONE         0
#define SPI_BUS_STATE_MEMORY       1
#define SPI_BUS_STATE_MAIN_BUS     2
#define SPI_BUS_STATE_ALT_BUS      3
#define SPI_BUS_STATE_RESET        4

#define WRITE_BIT_MASK      0x80


void digitalWrite(int pin, byte mode)
{
    SPI.digitalWrite(pin, mode);
}

void pinMode(int pin, byte mode)
{
}

RAMEmulator SharedRAM;

Pin::Pin()
: state(SPI_BUS_STATE_NONE), transferi(0)
{
}

byte Pin::transfer(byte b)
{
    // for sure make this gets updated.
    transferi++;
    
    // todo: split this into different classes
    // I designed the Pin class to be used for different hardware emulation
    // but right now I just stuck it all in one class, which is a bit of lazy-work
    // but it works for now
    if (pin == DEFAULT_CORE_SS_PIN)
    {
        // Bus state is usually transported first
        if (transferi == 1)
        {
            state = b;
            return NULL;
        }
        
        switch (state) {
            case SPI_BUS_STATE_NONE:
                break;
            case SPI_BUS_STATE_MEMORY:
            {
                if (transferi == 2) {
                    addr = b;
                    write = (b & WRITE_BIT_MASK);
                    
                    if (write) {
                        addr &= ~WRITE_BIT_MASK;
                    }
                    
                    break;
                }
                
                if (transferi == 3) {
                    addr = (addr << 8) | (b & 0xFF);
                    break;
                }
                
                byte read = *SharedRAM[addr | 0x8000];
                
                if (write) {
                    *SharedRAM[addr | 0x8000] = b;
                }
                
                addr++;
                
                return read;
            }
            case SPI_BUS_STATE_MAIN_BUS:
            case SPI_BUS_STATE_ALT_BUS:
            case SPI_BUS_STATE_RESET:
                break;
        }
    }
    else if (pin == DEFAULT_SYS_SS_PIN)
    {
        // Opcode is usually transported first
        if (transferi == 1)
        {
            opcode = b;
            return NULL;
        }
        
        if (opcode == OP_WRITE_COMMAND && transferi == 2)
        {
            // todo: split this into an RPC emulator
            switch (b) {
                case RPC_CMD_FILE_INIT:
                    break;
                case RPC_CMD_FILE_INFO:
                {
                    break;
                }
                case RPC_CMD_FILE_OPEN:
                {
                    RPC_FileOpenArgs args;
                    SharedRAM.read(INPUT_ARG_ADDR, &args.in, sizeof(args.in));
                    
                    char filename[STRING_BUF_SIZE];
                    SharedRAM.read(args.in.filename_addr, &filename, STRING_BUF_SIZE);
                    
                    int mode = args.in.mode;
                    if (mode & FA_READ) {
                        args.out.handle = filesystem.open(filename, "rb");
                    } else if (mode & FA_WRITE) {
                        args.out.handle = filesystem.open(filename, "wb");
                    }
                    
                    SharedRAM.write(OUTPUT_ARG_ADDR, &args.out, sizeof(args.out));
                    
                    break;
                }
                case RPC_CMD_FILE_CLOSE:
                {
                    RPC_FileCloseArgs args;
                    SharedRAM.read(INPUT_ARG_ADDR, &args.in, sizeof(args.in));
                    filesystem.close(args.in.handle);
                    
                    break;
                }
                case RPC_CMD_FILE_READ:
                {
                    RPC_FileReadArgs args;
                    SharedRAM.read(INPUT_ARG_ADDR, &args.in, sizeof(args.in));
                    
                    int addr = args.in.dst_addr;
                    char bank = *SharedRAM[REG_MEM_BANK | 0x8000];
                    if (bank > 0 && addr > 0x3FFF) {
                        addr = (bank * 0x4000) + (args.in.dst_addr - 0x4000);
                    }
                    printf("poop: %x 0x%x 0x%x\n", (bank * 0x4000) + (args.in.dst_addr - 0x4000), addr, args.in.dst_addr);
                    
                    uint16_t bytesRead = filesystem.read(args.in.handle, SharedRAM[addr], args.in.size);
                    args.out.size_read = bytesRead;
                    break;
                }
                case RPC_CMD_FILE_WRITE:
                case RPC_CMD_FILE_SIZE:
                {
                    RPC_FileSizeArgs args;
                    SharedRAM.read(INPUT_ARG_ADDR, &args.in, sizeof(args.in));
                    uint32_t size = filesystem.size(args.in.handle);
                    args.out.size = size;
                    
                    SharedRAM.write(OUTPUT_ARG_ADDR, &args.out, sizeof(args.out));
                    break;
                }
                case RPC_CMD_FILE_SEEK:
                    break;
            }
            return NULL;
        }
        
        if (opcode == OP_ACCESS_RAM)
        {
            if (transferi == 2)
            {
                ramopcode = b;
                return NULL;
            }
            
            if (transferi == 3)
            {
                ramaddr = b;
                return NULL;
            }
            
            if (transferi == 4)
            {
                ramaddr = (ramaddr << 8) | (b & 0xFF);
                return NULL;
            }
            
            switch (ramopcode)
            {
                case RAM_ST_READ:
                case RAM_ST_WRITE:
                    break;
                case RAM_READ:
                {
                    byte b = *SharedRAM[ramaddr];
                    ramaddr++;
                    return b;
                }
                case RAM_WRITE:
                {
                    *SharedRAM[ramaddr] = b;
                    ramaddr++;
                    break;
                }
            }
        }
    }
    
    return NULL;
}

byte Pin::getMode()
{
    return this->mode;
}

void Pin::setMode(byte mode)
{
    // we progressively used the transferred data
    // it's a bit hacky, but we should reset the data here.
    if (mode == HIGH) {
        state = SPI_BUS_STATE_NONE;
        transferi = 0;
        
        emuinocube.tick();
    }
    
    this->mode = mode;
}

void Pin::setPin(int pin)
{
    this->pin = pin;
}

void SPIEmulator::begin()
{
    for (int i = 0; i < DEFAULT_PIN_COUNT; i++) {
        this->pins[i].setPin(i);
        this->pins[i].setMode(HIGH);
    }
}

byte SPIEmulator::transfer(byte b)
{
    for (int i = 0; i < DEFAULT_PIN_COUNT; i++) {
        Pin &pin = this->pins[i];
        if (pin.getMode() == LOW) {
           return pin.transfer(b);
        }
    }
    
    return NULL;
}

void SPIEmulator::digitalWrite(int pin, byte mode)
{
    this->pins[pin].setMode(mode);
}
