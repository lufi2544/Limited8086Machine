﻿#include "sim86_memory.h"





u32 LoadMemoryFromFile(char* FileName, memory* Memory, u32 AtOffset)
{
    u32 Result = 0;
    
    if(AtOffset < ArrayCount(Memory->Bytes))
    {
        FILE *File = {};
        if(fopen_s(&File, FileName, "rb") == 0)
        {
            Result = fread(Memory->Bytes + AtOffset, 1, ArrayCount(Memory->Bytes) - AtOffset, File);
            fclose(File);
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to open the file %s\n", FileName);
	}
	
	return Result;
}

u32 GetMemoryAddress_8086(u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset)
{
    u32 Result = (((u32)SegmentBase << 4) + (u32)(SegmentOffset + AdditionalOffset)) & MEMORY_ACCESS_MASK;
    return Result;
}

u32 GetMemoryAddress_8086(segmented_access Access, s16 AdditionalOffset)
{
    u32 Result = GetMemoryAddress_8086(Access.SegmentBase, Access.SegmentOffset, AdditionalOffset);
    return Result;
}

u8 ReadMemory(memory *Memory, u32 AbsoluteAddress)
{
    // Simulating 1MB of memory, getting whatever byte here.
    assert(AbsoluteAddress < ArrayCount(Memory->Bytes));
    u8 Result = Memory->Bytes[AbsoluteAddress];
    return Result;
}

void
WriteMemory(u8 Value, u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset, memory *Memory)
{
	u32 MemoryAddress = GetMemoryAddress_8086(SegmentBase, SegmentOffset, AdditionalOffset);
	Memory->Bytes[MemoryAddress] = Value;
}


u8 
ReadMemory(memory *Memory, u16 SegmentBase, u16 SegmentOffset, s16 AdditionalOffset)
{
	u32 MemoryAddress = GetMemoryAddress_8086(SegmentBase, SegmentOffset, AdditionalOffset);
	return Memory->Bytes[MemoryAddress];
}