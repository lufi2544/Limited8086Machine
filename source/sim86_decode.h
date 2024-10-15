#pragma once
#include "sim8086.h"
#include "sim8086_types.h"
#include "sim86_memory.h"

enum instruction_bits_usage : u8
{
    Bits_Literal,
    Bits_MOD,
    Bits_REG,
    Bits_RM,
    Bits_SR,
    Bits_Disp,
    Bits_Data,
	
    Bits_HasDisp,
    Bits_DispAlwaysW,
    Bits_HasData,
    Bits_WMakesDataW,
    Bits_RMRegAlwaysW,
    Bits_RelJMPDisp,
    Bits_D,
    Bits_S,
    Bits_W,
    Bits_V,
    Bits_Z,
	
    Bits_Count
};

struct instruction_bits
{
    instruction_bits_usage Usage;
    u8 BitCount;
    u8 Shift;
    u8 BitsDecimalValue;
};

struct instruction_format
{
    operation_type Op;
    instruction_bits Bits[16];
};

struct disasm_context
{
    u32 AdditionalFlags;
    register_index DefaultSegment;
};


disasm_context DefaultContext(void);

void
UpdateContext(disasm_context *Context, instruction Instruction);

void
UpdateFlagsRegister(instruction Instruction);

instruction DecodeInstruction(disasm_context *Context, memory *Memory, segmented_access *At);
instruction TryDecode(disasm_context *Context, instruction_format *Inst, memory *Memory, segmented_access At);

