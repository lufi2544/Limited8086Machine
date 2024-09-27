#pragma once

#include "sim8086_types.h"

struct memory
{
    u8 Bytes[1024 * 1024];
};

#define MEMORY_ACCESS_MASK 0xfffff

struct segmented_access
{
    u16 SegmentBase;
    u16 SegmentOffset;
};

 u32 LoadMemoryFromFile(char* FileName, memory* Memory, u32 AtOffset);

 u32 GetAbsoluteAddressOf(u16 SegmentBase, u16 SegmentOffset, u16 AdditionalOffset = 0);
 u32 GetAbsoluteAddressOf(segmented_access Access, u16 AdditionalOffset = 0);

 u8 ReadMemory(memory *Memory, u32 AbsoluteAddress);