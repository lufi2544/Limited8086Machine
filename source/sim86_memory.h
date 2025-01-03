﻿#pragma once

#include "sim8086_types.h"
#include "stack.h"


struct memory
{
    u8 Bytes[1024 * 1024];
	stack Stack;
};

#define MEMORY_ACCESS_MASK 0xfffff
#define STACK_SIZE_86 0xff // 256 bytes
#define STACK_SEGMENT_OFFSET 0xfff0 // segment that lets 256 bytes of stack

struct segmented_access
{
    u16 SegmentBase;
    u16 SegmentOffset;
};

u32 LoadMemoryFromFile(char* FileName, memory* Memory, u32 AtOffset);

u32 GetMemoryAddress_8086(u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset = 0);

u32 GetMemoryAddress_8086(segmented_access Access, s16 AdditionalOffset = 0);

u8 ReadMemory(memory *Memory, u32 AbsoluteAddress);

u8 
ReadMemory(memory *Memory, u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset);

void
WriteMemory(u8 Value, u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset, memory* Memory);



