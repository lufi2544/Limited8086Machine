#pragma once
#include "sim8086.h"
#include "sim8086_types.h"

/*
struct instruction_format
{
    operation_type Op;
    instruction_bit
};*/


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

struct disasm_context
{
    register_index DefaultSegment;
    u32 AdditionalFlags;
};


static disasm_context DefaultContext(void);