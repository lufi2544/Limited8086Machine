#include "sim86_decode.h"

#include "sim86_memory.h"
#include <iostream>



instruction_format InstructionsFormats[] =
{
#include "sim86_instruction_table.inl"
};

disasm_context DefaultContext()
{
    disasm_context Result = {};
	
    Result.DefaultSegment = Register_ds;
	
    return Result;
}

static u32 ParseDataValue(memory *Memory, segmented_access *Access, b32 Exists, b32 Wide, b32 SignExtended)
{
    u32 Result = {};
    if(Exists)
    {
        if(Wide)
        {
            u8 D0 = ReadMemory(Memory, GetMemoryAddress_8086(*Access, 0));
            u8 D1 =  ReadMemory(Memory, GetMemoryAddress_8086(*Access, 1));
            Result = (D1 << 8) | D0;
            Access->SegmentOffset += 2;
        }
        else
        {
            Result = ReadMemory(Memory,  GetMemoryAddress_8086(*Access));
            if(SignExtended)
            {
                Result = (s32)*(s8 *)&Result;
            }
            Access->SegmentOffset += 1;
        }
    }
	
    return Result;
}

static instruction_operand GetRegOperand(u32 IntelRegIndex, b32 Wide)
{
    register_access RegTable[][2]
    {
        {{Register_a, 0, 1}, {Register_a, 0, 2}},
        {{Register_c, 0, 1}, {Register_c, 0, 2}},
        {{Register_d, 0, 1}, {Register_d, 0, 2}},
        {{Register_b, 0, 1}, {Register_b, 0, 2}},
        {{Register_a, 1, 1}, {Register_sp, 0, 2}},
        {{Register_c, 1, 1}, {Register_bp, 0, 2}},
        {{Register_d, 1, 1}, {Register_si, 0, 2}},
        {{Register_b, 1, 1}, {Register_di, 0, 2}},
    };
	
    instruction_operand Result = {};
    Result.Type = Operand_Register;
    Result.Register = RegTable[IntelRegIndex & 0x7/*(0b00000111)*/][(Wide != 0)];
	
    return Result;
}

instruction
TryDecode(disasm_context *Context, instruction_format *Inst, memory *Memory, segmented_access At)
{
    instruction Dest = {};
    u32 HasBits = 0;
    u32 Bits[Bits_Count] = {};
    b32 Valid = true;
	
    // Index in the simulated memory
    u32 StartingAddress = GetMemoryAddress_8086(At);
	
	
    u8 BitsPendingCount = 0;
    u8 ByteRawInstruction = 0;
	
    // Decoding all the Instruction and putting al in the Bits, so later we can translate everything
    auto count = ArrayCount(Inst->Bits);
    for(u32 BitsIndex = 0; Valid && (BitsIndex <  count); ++BitsIndex)
    {
        instruction_bits TestBits = Inst->Bits[BitsIndex];
        if((TestBits.Usage == Bits_Literal) && (TestBits.BitCount == 0))
        {
            break;
        }
		
        u32 BitsValue = TestBits.BitsDecimalValue;
        if(TestBits.BitCount != 0)
        {
            // Is this the first time we attempt to read the instruction bits?
            if(BitsPendingCount == 0)
            {
                BitsPendingCount = 8;
				
                // We only enter here the fist time we are reading the byte. Next times, we just read the bits and store
                // the rest of the bits values like the W and the D.
                
                // This will give us the Raw value of the instruction, counting with the Op_Code and the rest of the bits
                // in the Byte read.
				
                // return  would be 10001011(100010dw) and then at the bottom we can see how we mask off only the OpCode.  
                ByteRawInstruction = ReadMemory(Memory, GetMemoryAddress_8086(At));
				
                // Advance the byte reading count.
                ++At.SegmentOffset;
            }
			
            // NOTE(casey): If this assert fires, it means we have an error in our table,
            // since there are no 8086 instructions that have bit values straddling a
            // byte boundary.
            assert(TestBits.BitCount <= BitsPendingCount);
			
            
            // Here we basically extract the part we want in the instruction, so if we have
            // 100010dw, then we know is a move operation, but we have to remove the last 2 bits from the instruction.
            // Same process if for the value bits like W and D
            BitsPendingCount -=  TestBits.BitCount;
            BitsValue = ByteRawInstruction;
            BitsValue >>= BitsPendingCount;
			
            // masking off the rest of the bits in the instruction byte that we don't care about --> (100010011(10001001dw) >>=2 -->  001000100 &= ~(11111111 << 6(BitCount))) --> ~(11111111 << 6(BitCount) --> ~(11000000) --> (00111111)
            // 001000100 &= (00111111) --> 001000100 -> I imagine is just to be sure about the bits
            BitsValue &= ~(0xff << TestBits.BitCount);
        }
		
        if(TestBits.Usage ==  Bits_Literal)
        {
            // If we are decoding OpCode
            Valid = Valid && (BitsValue ==  TestBits.BitsDecimalValue);
        }
        else
        {
            // Plain instruction flags
            Bits[TestBits.Usage] |= (BitsValue <<  TestBits.Shift);
            HasBits |= (1 <<  TestBits.Usage);
        }
    }
	
    
	
    if(Valid)
    {
        // Once everything is decoded, then we just use the manual to see the conditions for parsing the instructions
        u32 Mod = Bits[Bits_MOD];
        u32 RM = Bits[Bits_RM];
        u32 W = Bits[Bits_W];
        b32 S = Bits[Bits_S];
        b32 D = Bits[Bits_D];
		
        b32 HasDirectAddress = ((Mod == 0b00) && (RM == 0b110));
        b32 HasDisplacement = ((Bits[Bits_HasDisp]) || (Mod == 0b10) || (Mod == 0b01) || HasDirectAddress);
        b32 DisplacementIsW = ((Bits[Bits_DispAlwaysW]) || (Mod == 0b10) || HasDirectAddress);
        b32 DataIsW = ((Bits[Bits_WMakesDataW]) && !S && W);
		
        Bits[Bits_Disp] |= ParseDataValue(Memory, &At, HasDisplacement, DisplacementIsW, (!DisplacementIsW));
        Bits[Bits_Data] |= ParseDataValue(Memory, &At, Bits[Bits_HasData], DataIsW, S);
		
        Dest.Op = Inst->Op;
        Dest.Flags = Context->AdditionalFlags;
        Dest.Address = StartingAddress;
        auto currentAddress = GetMemoryAddress_8086(At);
        Dest.Size = GetMemoryAddress_8086(At) - StartingAddress;
        if(W)
        {
            Dest.Flags |= Inst_Wide;
        }
		
        u32 Disp = Bits[Bits_Disp];
        s16 Displacement = (s16)Disp;
		
        instruction_operand *RegOperand = &Dest.Operands[D ? 0 : 1];
        instruction_operand *ModOperand = &Dest.Operands[D ? 1 : 0];
        
        if(HasBits & (1 << Bits_SR))
        {
            RegOperand->Type = Operand_Register;
            RegOperand->Register.Index = (register_index)(Register_es + (Bits[Bits_SR] & 0x3));
            RegOperand->Register.WidenessID = 2;
        }
		
        if(HasBits & (1 << Bits_REG))
        {
            *RegOperand = GetRegOperand(Bits[Bits_REG], W);
        }
		
        if(HasBits & (1 << Bits_MOD))
        {
            if(Mod == 0b11)
            {
                *ModOperand = GetRegOperand(RM, W || (Bits[Bits_RMRegAlwaysW]));
            }
            else
            {
                ModOperand->Type = Operand_Memory;
                ModOperand->Address.Segment = Context->DefaultSegment;
                ModOperand->Address.Displacement = Displacement;
				
                if((Mod == 0b00) && (RM == 0b110))
                {
                    ModOperand->Address.Base = EffectiveAddress_direct;
                }
                else
                {
                    ModOperand->Address.Base = (effective_address_base)(1+RM);
                }
            }
        }
		
        //
        // NOTE(casey): Because there are some strange opcodes that do things like have an immediate as
        // a _destination_ ("out", for example), I define immediates and other "additional operands" to
        // go in "whatever slot was not used by the reg and mod fields".
        //
		
        instruction_operand *LastOperand = &Dest.Operands[0];
        if(LastOperand->Type)
        {
            LastOperand = &Dest.Operands[1];
        }
		
        if(Bits[Bits_RelJMPDisp])
        {
            LastOperand->Type = Operand_RelativeImmediate;
            LastOperand->ImmediateS32 = Displacement + Dest.Size;
        }
		
        if(Bits[Bits_HasData])
        {
            LastOperand->Type = Operand_Immediate;
            LastOperand->ImmediateU32 = Bits[Bits_Data];
        }
		
        if(HasBits & (1 << Bits_V))
        {
            if(Bits[Bits_V])
            {
                LastOperand->Type = Operand_Register;
                LastOperand->Register.Index = Register_c;
                LastOperand->Register.Offset = 0;
                LastOperand->Register.WidenessID = 1;
            }
            else
            {
                LastOperand->Type = Operand_Immediate;
                LastOperand->ImmediateS32 = 1;
            }
        }
    }
	
    return Dest;
}



instruction DecodeInstruction(disasm_context *Context, memory *Memory, segmented_access *At)
{
    instruction Result = { };
	
    // Go through all the instructions to see if the bytes in the Memory buffer contains any.
    for(u32 Index = 0; Index < ArrayCount(InstructionsFormats); ++Index)
    {
        instruction_format Inst = InstructionsFormats[Index];
        Result = TryDecode(Context, &Inst, Memory, *At);
		
        if(Result.Op)
        {
            At->SegmentOffset += Result.Size;
            break;
        }
    }
	
    return Result;
}

// Tells us if the number is negative. Used for setting the Flags Register
bool
IsNegative(const u32 Num)
{
	return (1 << 15) & (Num & 0xFFFF);
}

const char*
GetFlagRegisterName(u8 Flag)
{
	register_flags_fields Casted_Flag = register_flags_fields(Flag);
	if(Casted_Flag == register_flags_fields::Flag_sign)
	{
		return "S";
	}
	else if(Casted_Flag == register_flags_fields::Flag_zero)
	{
		return "Z";
	}
	
	return nullptr;
}


void
UpdateContext(disasm_context *Context, instruction Instruction)
{
    Context->AdditionalFlags = 0;
    Context->DefaultSegment = Register_ds;
	
    // REGISTER STATES
    // For now we have simple move instructions. When we imply 8 bit registers like dh or dl, we have to check the D flag.
	
	// TODO REFACTOR THIS MESS.
	u32 Prev_Register_Flags = g_Registers_Infos[register_index::Register_flags - 1];
	
    register_index DestinationRegister = register_index::Register_none;
	s32 DestinationRegister_PostChange_Value = 0;
	bool bShouldAffectFlags = false;
    for(u32 i = 0; i < ArrayCount(Instruction.Operands); ++i)
    {
        instruction_operand Operand = Instruction.Operands[i];
        if(Instruction.Op == operation_type::Op_mov)
        {
            // MOV
            
            if(Operand.Type == operand_type::Operand_Register)
            {
                // TODO Should we check the D bit here?
                // This means we are performing a  mov register, register
                if(DestinationRegister != register_index::Register_none)
                {
                    g_Registers_Infos[DestinationRegister - 1] = g_Registers_Infos[Operand.Register.Index - 1];
                }
                else
                {
                    DestinationRegister = Operand.Register.Index;
                }
            }
            else if(Operand.Type == operand_type::Operand_Immediate)
            {
                if(DestinationRegister != register_index::Register_none)
                {
                    g_Registers_Infos[DestinationRegister - 1] = Operand.ImmediateU32;
                }
            }
        }
        else if(Instruction.Op == operation_type::Op_add)
        {
            // ADD
            
            if(Operand.Type == operand_type::Operand_Register)
            {
                if(DestinationRegister == register_index::Register_none)
                {
                    DestinationRegister = Operand.Register.Index;
					DestinationRegister_PostChange_Value = g_Registers_Infos[DestinationRegister - 1];
					bShouldAffectFlags = true;
                }
                else
                {
                    g_Registers_Infos[DestinationRegister - 1] += g_Registers_Infos[Operand.Register.Index - 1];
                }
            }
            else if(Operand.Type == operand_type::Operand_Immediate)
            {
                if(DestinationRegister != register_index::Register_none)
                {
                    g_Registers_Infos[DestinationRegister - 1] += Operand.ImmediateS32;
                }
            }
            
        }
        else if(Instruction.Op == operation_type::Op_sub)
        {
            // SUB
            
            if(Operand.Type == operand_type::Operand_Register)
            {
                if(DestinationRegister == register_index::Register_none)
                {
                    DestinationRegister = Operand.Register.Index;
					DestinationRegister_PostChange_Value = g_Registers_Infos[DestinationRegister - 1];
					bShouldAffectFlags = true;
                }
                else
                {
                    g_Registers_Infos[DestinationRegister - 1] -= g_Registers_Infos[Operand.Register.Index - 1];
                }
            }
            else if(Operand.Type == operand_type::Operand_Immediate)
            {
                if(DestinationRegister != register_index::Register_none)
                {
                    g_Registers_Infos[DestinationRegister - 1] -= Operand.ImmediateS32;
					
					u32 Comparison_Result = Operand.ImmediateS32 - DestinationRegister_PostChange_Value;
					
					if(Comparison_Result == 0)
					{
						// Zero
						g_Registers_Infos[register_index::Register_flags - 1] |= ( 1 << register_flags_fields::Flag_zero);
					}
					else if(IsNegative(Comparison_Result))
					{
						// Is smaller
						g_Registers_Infos[register_index::Register_flags - 1] |= ( 1 << register_flags_fields::Flag_sign);
					}
					else
					{
						// Greater
						g_Registers_Infos[register_index::Register_flags - 1] &= (~( 1 << register_flags_fields::Flag_sign));
					}
                }
            }
        }
		else if(Instruction.Op == operation_type::Op_cmp)
		{
			if(Operand.Type == operand_type::Operand_Register)
			{
				if(DestinationRegister == register_index::Register_none)
				{
					bShouldAffectFlags = true;
					DestinationRegister = Operand.Register.Index;
					DestinationRegister_PostChange_Value = g_Registers_Infos[DestinationRegister - 1];
				}
				else
				{
					s32 Register_To_Compare_Value = g_Registers_Infos[Operand.Register.Index - 1];
					u32 Comparison_Result = Register_To_Compare_Value - DestinationRegister_PostChange_Value;
					
					if(Comparison_Result == 0)
					{
						// Zero
						g_Registers_Infos[register_index::Register_flags - 1] |= ( 1 << register_flags_fields::Flag_zero);
					}
					else if(IsNegative(Comparison_Result))
					{
						// Is smaller
						g_Registers_Infos[register_index::Register_flags - 1] |= ( 1 << register_flags_fields::Flag_sign);
					}
					else
					{
						// Greater
						g_Registers_Infos[register_index::Register_flags - 1] &= (~( 1 << register_flags_fields::Flag_sign));
					}
					
				}
			}
		}
		
    }
	
	// Analize for setting the Flags Register
	if(DestinationRegister != register_index::Register_none)
	{
		u32& Register_Flags = g_Registers_Infos[register_index::Register_flags - 1];
		
		// FLAGS SETTING
		if(bShouldAffectFlags)
		{
			// Negative Flag
			u32 Mask_Sign = 1 << (u8)register_flags_fields::Flag_sign;
			if(IsNegative(DestinationRegister_PostChange_Value))
			{
				Register_Flags |= Mask_Sign; 
			}
			else
			{
				Register_Flags &= (~(Mask_Sign));
			}
		}
		
		
		// PRINT FLAGS CHANGE
		if(Register_Flags != Prev_Register_Flags)
		{
			std::printf("Flags ");
			
			// PREV FLAGS
			for(u8 Flag_Index = 0; Flag_Index < register_flags_fields::Flag_Num; ++Flag_Index)
			{
				if(Prev_Register_Flags & (1 << Flag_Index))
				{
					if(const char* FlagName = GetFlagRegisterName(Flag_Index))
					{
						std::printf("%s", FlagName);
					}
				}
			}
			
			// CURRENT FLAGS
			std::printf(" --> ");
			for(u8 Flag_Index = 0; Flag_Index < register_flags_fields::Flag_Num; ++Flag_Index)
			{
				if(Register_Flags & (1 << Flag_Index))
				{
					if(const char* FlagName = GetFlagRegisterName(Flag_Index))
					{
						std::printf("%s", FlagName);
					}
				}
				
			}
			
			std::printf("\n");
		}
		
		
		
		
	}
	
}
