#include <stdlib.h>

#define MEMORY_HI_SCORE_ADDR 0x020

void Memory_Init();

void Memory_Read(int addr, void* data, size_t size);

void Memory_Write(int addr, void* data, size_t size);