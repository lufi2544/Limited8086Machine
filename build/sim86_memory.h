#pragma once

#include "sim8086_types.h"

struct memory
{
    u8 Bytes[1024 * 1024];
};

struct segmented_access
{
    u16 SegmentBase;
    u16 SegmentOffset;
};

static u32 LoadMemoryFromFile(char* FileName, memory* Memory, u32 AtOffset);