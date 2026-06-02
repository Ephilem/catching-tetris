#include <stdlib.h>

void Memory_Init();

void Memory_Read(int addr, void* data, size_t size);

void Memory_Write(int addr, void* data, size_t size);