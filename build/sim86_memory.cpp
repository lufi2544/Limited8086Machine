#include "sim86_memory.h"

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

u32 GetAbsoluteAddressOf(u16 SegmentBase, u16 SegmentOffset, u16 AdditionalOffset)
{
    u32 Result = (((u32)SegmentBase << 4) + (u32)(SegmentOffset + AdditionalOffset)) & MEMORY_ACCESS_MASK;
    return Result;
}

u32 GetAbsoluteAddressOf(segmented_access Access, u16 AdditionalOffset)
{
    u32 Result = GetAbsoluteAddressOf(Access.SegmentBase, Access.SegmentOffset, AdditionalOffset);
    return Result;
}

 u8 ReadMemory(memory *Memory, u32 AbsoluteAddress)
{
    // Simulating 1MB of memory, getting whatever byte here.
    assert(AbsoluteAddress < ArrayCount(Memory->Bytes));
    u8 Result = Memory->Bytes[AbsoluteAddress];
    return Result;
}

