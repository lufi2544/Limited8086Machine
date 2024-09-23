
#pragma once
#include "sim8086_types.h"


enum operation_type
{
    Op_None,

#define INS(Mnemonic, ...) Op##Mnemonic,
#define INSTALT(...)
#include "sim86_instruction_table.inl"    
};


enum register_index
{
    Register_none,

    Register_a,
    Register_b,
    Register_c,
    Register_d,
    Register_sp,
    Register_bp,
    Register_si,
    Register_di,
    Register_es,
    Register_cs,
    Register_ss,
    Register_ds,
    Register_ip,
    Register_flags,

    Register_count
};

enum operand_type
{
    Operand_None,
    Operand_Register,
    Operand_Memory,
    Operand_Immediate,
    Operand_RelativeImmediate
};

struct instruction
{
    u32 Address;
    u32 Size;

    operation_type Op;
    u32 Flags;

    instruction_operand Operands[2];
};