#pragma once

#include "sim8086_types.h"

struct memory
{
    u8 Bytes[1024 * 1024];
};


static u32 LoadMemoryFromFile(char* FileName, memory* Memory, u32 AtOffset);