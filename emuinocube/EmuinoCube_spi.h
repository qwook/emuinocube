//
//  EmuinoCube_spi.h
//  emuinocube
//
//  Created by Henry Tran on 10/7/13.
//  Copyright (c) 2013 Henry Tran. All rights reserved.
//

#ifndef __emuinocube__EmuinoCube_spi__
#define __emuinocube__EmuinoCube_spi__

#include <iostream>
#include <cstdio>

#include "DuinoCube_mem.h"
#include "DuinoCube_defs.h"

#define DEFAULT_PIN_COUNT 6

#define MSBFIRST 0
#define SPI_CLOCK_DIV2 0
#define SPI_MODE0 0

#define HIGH 1
#define LOW 2

#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

typedef unsigned char byte;
typedef uint16_t word;

#define lowByte(w) ((w) & 0xff)
#define highByte(w) ((w) >> 8)

void digitalWrite(int pin, byte mode);
void pinMode(int pin, byte mode);

class Pin
{
public:
    Pin();
    virtual byte transfer(byte b);
    
    byte getMode();
    void setMode(byte mode);
    void setPin(int pin);
    
private:
    int pin;
    byte mode;
    byte state;
    
    
    int transferi;
    
    // initial settings
    char settings[3];
    
    // core
    uint16_t addr; // address
    bool write; // write or read
    
    // system
    byte opcode;
    byte ramopcode;
    uint16_t ramaddr;
};

class SPIEmulator
{
public:
    void begin();
    void setBitOrder(byte) {} // assume most-significant order
    void setClockDivider(byte) {}
    void setDataMode(byte) {}
                    
    byte transfer(byte b);
    void digitalWrite(int pin, byte mode);
    
private:
    Pin pins[DEFAULT_PIN_COUNT];
};


class RAMEmulator
{
public:
    
    unsigned char * operator[](int n)
    {
        return &data[n];
    }
    
    void read(int addr, void *c, size_t size)
    {
        memcpy(c, &data[addr], size);
    };
    
    void write(int addr, void *c, size_t size)
    {
        memcpy(&data[addr], c, size);
    };
    
private:
    unsigned char data[SHARED_MEMORY_SIZE + 0x8000 * 5];
};


#endif /* defined(__emuinocube__EmuinoCube_spi__) */
