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
