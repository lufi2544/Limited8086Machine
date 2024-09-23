#include "sim86_decode.h"

#include "sim86_memory.h"

disasm_context DefaultContext()
{
    disasm_context Result = {};

    Result.DefaultSegment = Register_ds;

    return Result;
}


static instruction DecodeInstruction(disasm_context *Context, memory *Memory, segment *At)
{
    
}