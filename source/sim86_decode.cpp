﻿#include "sim86_decode.h"

#include "sim86_memory.h"
#include "sim8086.h"
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
	else if(Casted_Flag == register_flags_fields::Flag_parity)
	{
		return "P";
	}
	
	return nullptr;
}

void
UpdateContext(disasm_context *Context, instruction Instruction)
{
    Context->AdditionalFlags = 0;
    Context->DefaultSegment = Register_ds;
}


void PerformInstructionJump(segmented_access *At, u32 InstructionSize, s32 JumpOffset)
{
	At->SegmentOffset += JumpOffset;
	At->SegmentOffset -= InstructionSize;
}


u32 
GetMemoryAddressBaseFromOperand(instruction_operand *Operand)
{
	// mov ax, 4
	u32 MemoryAddressBase = Operand->Address.Displacement;
	if(Operand->Address.Base == effective_address_base::EffectiveAddress_bx)
	{
		// [bx + 4]
		MemoryAddressBase += g_Register_Infos[register_index::Register_b - 1];
	}
	else if(Operand->Address.Base == effective_address_base::EffectiveAddress_bp_si)
	{
		// [bp + si]
		u32 Bp_Si = g_Register_Infos[register_index::Register_bp - 1] + g_Register_Infos[register_index::Register_si - 1];
		MemoryAddressBase = Bp_Si;
	}
	else
	{
		printf("ERROR: Reading from Memory as Source for Operation, but the effective_address_base is not implemented %i \n", Operand->Address.Base);
	}
	
	return MemoryAddressBase;
}


void
UpdateRegisterValues(disasm_context *Context, instruction Instruction, segmented_access *At, memory *Memory)
{
	
	// REGISTER STATES
    // For now we have simple move instructions. When we imply 8 bit registers like dh or dl, we have to check the D flag.
	
	u32 Prev_Register_Flags = g_Register_Infos[register_index::Register_flags - 1];
	
	instruction_operand DestinationOperand{};
	instruction_operand SourceOperand{};
	s32 SourceValue = 0;
	
	u32& Register_IP_Ref = g_Register_Infos[register_index::Register_ip - 1];
	u32 InstructionSize = (At->SegmentOffset - Register_IP_Ref);
	s32 IP_Offset_To_Add = InstructionSize;
	
    for(u32 i = 0; i < ArrayCount(Instruction.Operands); ++i)
    {
		
        instruction_operand Operand = Instruction.Operands[i];
		if(Operand.Type == operand_type::Operand_None)
		{
			continue;
		}
		
		bool bZeroFlagSet = Prev_Register_Flags & (1 << register_flags_fields::Flag_zero);
		bool bSignFlagSet = Prev_Register_Flags & (1 << register_flags_fields::Flag_sign);
		bool bParityFlagSet = Prev_Register_Flags & (1 << register_flags_fields::Flag_parity);
		bool bIsWideInstruction = Instruction.Flags & Inst_Wide;
		
		// JUMP OPERATIONS
		if(Instruction.Op == operation_type::Op_jne)
		{
			//@juanes: TODO ORGANIC WAY OF CHECKING FLAGS REGISTER AND ADDING IP_Offset_To_Add.
			if(!bZeroFlagSet)
			{
				PerformInstructionJump(At, InstructionSize, Operand.ImmediateS32);
				IP_Offset_To_Add = Operand.ImmediateS32;
				continue;
			}
		}
		else if(Instruction.Op == operation_type::Op_je)
		{
			if(bZeroFlagSet)
			{
				PerformInstructionJump(At, InstructionSize, Operand.ImmediateS32);
				IP_Offset_To_Add = Operand.ImmediateS32;
				continue;
			}
			
		}
		else if(Instruction.Op == operation_type::Op_jb)
		{
			if(bSignFlagSet)
			{
				PerformInstructionJump(At, InstructionSize, Operand.ImmediateS32);
				IP_Offset_To_Add = Operand.ImmediateS32;
				continue;
			}
		}
		else if(Instruction.Op == operation_type::Op_jp)
		{
			if(bParityFlagSet)
			{
				PerformInstructionJump(At, InstructionSize, Operand.ImmediateS32);
				IP_Offset_To_Add = Operand.ImmediateS32;
				continue;
			}
		}
		else if(Instruction.Op == operation_type::Op_loopnz)
		{
			// Z flag == 0 and CX -= 1 != 0
			if(!bZeroFlagSet)
			{
				if((--g_Register_Infos[register_index::Register_c - 1]) != 0)
				{
					PerformInstructionJump(At, InstructionSize, Operand.ImmediateS32);
					IP_Offset_To_Add = Operand.ImmediateS32;
					continue;
				}
				
			}
		}
		
		/////////////
		
		
		if(DestinationOperand.Type == operand_type::Operand_None)
		{
			DestinationOperand = Operand;
			continue;
		}
		
		SourceOperand = Operand;
		
		
		// READ THE SOURCE OPERAND
		
		if(SourceOperand.Type == operand_type::Operand_Immediate)
		{
			SourceValue = SourceOperand.ImmediateS32;
		}
		else if(SourceOperand.Type == operand_type::Operand_Register)
		{
			SourceValue = g_Register_Infos[Operand.Register.Index - 1];
		}
		else if(SourceOperand.Type == operand_type::Operand_Memory)
		{
			// Read from Memory
			u32 MemoryAddressBase = GetMemoryAddressBaseFromOperand(&SourceOperand);
			
			u8 SourceValueLowBits = ReadMemory(Memory, g_Register_Infos[SourceOperand.Address.Segment - 1], MemoryAddressBase, 0);
			SourceValue = SourceValueLowBits;
			
			if(bIsWideInstruction)
			{
				u16 SourceHighBits = ReadMemory(Memory, g_Register_Infos[SourceOperand.Address.Segment - 1], MemoryAddressBase, 1);
				SourceValue |= (SourceHighBits << 8);
			}
			
		}
		
		
		// OPERATION ON DESTINATION OPERAND
		
		if(DestinationOperand.Type == operand_type::Operand_Register)
		{
			u32& DestinationRegister = g_Register_Infos[DestinationOperand.Register.Index - 1];
			if(Instruction.Op == operation_type::Op_mov)
			{
				// MOV
				DestinationRegister = SourceValue;
			}
			else if(Instruction.Op == operation_type::Op_add)
			{
				// ADD
				DestinationRegister += SourceValue;
			}
			else if(Instruction.Op == operation_type::Op_sub)
			{
				// SUB
				DestinationRegister -= SourceValue;
			}
		}
		else if(DestinationOperand.Type == operand_type::Operand_Memory)
		{
			// DESTINATION AS MEMORY
			
			u32 DestinationMemoryAddressBase = GetMemoryAddressBaseFromOperand(&DestinationOperand);
			if(Instruction.Op == operation_type::Op_mov)
			{
				u8 LowBits = 0xFF & SourceValue;
				WriteMemory(LowBits, 0/* Segment */, DestinationMemoryAddressBase, 0, Memory);
				
				// Operating a wide instruction
				if(Instruction.Flags & Inst_Wide)
				{
					u8 HighBits = 0xFF00 & SourceValue;
					WriteMemory(HighBits, 0/* Segment */, DestinationMemoryAddressBase, 1, Memory);
				}
			}
		}
		
		
		// FLAGS  SETTING
		u32& Current_Register_Flags = g_Register_Infos[register_index::Register_flags - 1];
		if(Instruction.Op != operation_type::Op_mov)
		{
			
			u32 DestinationRegisterFinalValue = g_Register_Infos[DestinationOperand.Register.Index - 1];
			u32 ValueToEvaluate = DestinationRegisterFinalValue;
			
			if(Instruction.Op == operation_type::Op_cmp)
			{
				ValueToEvaluate = DestinationRegisterFinalValue - SourceValue;
			}
			
			if(ValueToEvaluate == 0)
			{
				// Zero
				Current_Register_Flags |= ( 1 << static_cast<u8>(register_flags_fields::Flag_zero));
			}
			else if(IsNegative(ValueToEvaluate))
			{
				// Negative
				Current_Register_Flags |= ( 1 << register_flags_fields::Flag_sign);
			}
			else
			{
				// Clear Flags
				Current_Register_Flags &= (~( 1 << register_flags_fields::Flag_sign));
				Current_Register_Flags &= (~( 1 << register_flags_fields::Flag_zero));
			}
			
			
			// PARITY FLAG (P) - even number of bits set on the result to evaluate
			u16 BitCount = 0;
			for(u16 BitIndex = 0; BitIndex < 16; ++BitIndex)
			{
				if((1 << (BitIndex)) & ValueToEvaluate)
				{
					BitCount++;
				}
			}
			
			if((BitCount % 2) == 0)
			{
				Current_Register_Flags |= 1 << register_flags_fields::Flag_parity;
			}
			else
			{
				Current_Register_Flags &= (~(1 << register_flags_fields::Flag_parity));
			}
			
		}
		
		
		// PRINT FLAGS CHANGE
		if(Current_Register_Flags != Prev_Register_Flags)
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
				if(Current_Register_Flags & (1 << Flag_Index))
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
	
	
	// UPDATING THE IP REGISTER
	Register_IP_Ref += IP_Offset_To_Add;
}
