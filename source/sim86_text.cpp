#include "sim86_text.h"
#include "sim86_decode.h"
#include <iostream>


char const *OpCodeMnemonics[] =
{
    "",
	
#define INST(Mnemonic, ...) #Mnemonic,
#define INSTALT(...)
#include "sim86_instruction_table.inl"
};

char const *Names[][3] =
{
    {"", "", ""},
    {"al", "ah", "ax"},
    {"bl", "bh", "bx"},
    {"cl", "ch", "cx"},
    {"dl", "dh", "dx"},
    {"sp", "sp", "sp"},
    {"bp", "bp", "bp"},
    {"si", "si", "si"},
    {"di", "di", "di"},
    {"es", "es", "es"},
    {"cs", "cs", "cs"},
    {"ss", "ss", "ss"},
    {"ds", "ds", "ds"},
    {"ip", "ip", "ip"},
    {"flags", "flags", "flags"}
};

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
PrintFlagsRegister()
{
	printf(" Flags: ");
	u32 FlagsRegister = g_Register_Infos[register_index::Register_flags - 1];
	for(u8 Flag_Index = 0; Flag_Index < register_flags_fields::Flag_Num; ++Flag_Index)
	{
		if(FlagsRegister & (1 << Flag_Index))
		{
			if(const char* FlagName = GetFlagRegisterName(Flag_Index))
			{
				std::printf("%s", FlagName);
			}
		}
		
	}
	
	printf("\n");
}

char const *GetMnemonic(operation_type Op)
{
	char const *Result = OpCodeMnemonics[Op];
    return Result;
}

char const *GetRegName(register_access Reg)
{
	
    static_assert(ArrayCount(Names) == Register_count, "Text table mismatch for register_index");
    
    char const *Result = Names[Reg.Index][(Reg.WidenessID == 2) ? 2 : Reg.Offset&1];
    return Result;
}



b32 IsPrintable(instruction Instruction)
{
    return true;
}

char const *GetEffectiveAddressExpression(effective_address_expression Address)
{
    char const *RMBase[] =
    {
        "",
        "bx+si",
        "bx+di",
        "bp+si",
        "bp+di",
        "si",
        "di",
        "bp",
        "bx",
    };
    static_assert(ArrayCount(RMBase) == EffectiveAddress_count, "Text table mismatch for effective_base_address");
    char const *Result = RMBase[Address.Base];
    return Result;
}

void PrintInstruction(instruction Instruction, FILE* Dest)
{
    u32 Flags = Instruction.Flags;
    u32 W = Flags & Inst_Wide;
    
    if(Flags & Inst_Lock)
    {
        /*
        if(Instruction.Op == Op_xchg)
        {
            // NOTE(casey): This is just a stupidity for matching assembler expectations.
            instruction_operand Temp = Instruction.Operands[0];
            Instruction.Operands[0] = Instruction.Operands[1];
            Instruction.Operands[1] = Temp;
        }
        fprintf(Dest, "lock ");
        */
    }
    
    char const *MnemonicSuffix = "";
    if(Flags & Inst_Rep)
    {
        printf("rep ");
        MnemonicSuffix = W ? "w" : "b";
    }
    
    fprintf(Dest, "%s%s ", GetMnemonic(Instruction.Op), MnemonicSuffix);
    
    char const *Separator = "";
    for(u32 OperandIndex = 0; OperandIndex < ArrayCount(Instruction.Operands); ++OperandIndex)
    {
        instruction_operand Operand = Instruction.Operands[OperandIndex];
        if(Operand.Type != Operand_None)
        {
            fprintf(Dest, "%s", Separator);
            Separator = ", ";
            
            switch(Operand.Type)
            {
                case Operand_None: {} break;
                
                case Operand_Register:
                {
                    fprintf(Dest, "%s", GetRegName(Operand.Register));
                }
                break;
                
                case Operand_Memory:
                {
                    effective_address_expression Address = Operand.Address;
					
                    if(Instruction.Operands[0].Type != Operand_Register)
                    {
                        fprintf(Dest, "%s ", W ? "word" : "byte");
                    }
                    
                    if(Flags & Inst_Segment)
                    {
                        printf("%s:", GetRegName({Address.Segment, 0, 2}));
                    }
                    
                    fprintf(Dest, "[%s", GetEffectiveAddressExpression(Address));
                    if(Address.Displacement != 0)
                    {
                        fprintf(Dest, "%+d", Address.Displacement);
                    }
                    fprintf(Dest, "]");
                } break;
                
                case Operand_Immediate:
                {
                    fprintf(Dest, "%d", Operand.ImmediateS32);
                } break;
                
                case Operand_RelativeImmediate:
                {
                    fprintf(Dest, "$%+d", Operand.ImmediateS32);
                } break;
            }
        }
    }
}

s32 TwosComplementToSigned(u32 Number)
{
	// Clamping the number to a 16 bit, since the 8086 is a 16 bit CPU.
    Number &= 0xFFFF;
    if(IsNegative(Number))
    {
        // Clamping the result from the 2s complement operation to 16 bit. 
        s32 a = (~(Number) + 1) & 0xFFFF;
        
        // adding just the symbol here.
        return -a;
    }
    else
    {
        return Number;
    }
}



void PrintRegistersState(FILE* Dest)
{
    const char* Registers[]
	{
        "ax",
        "bx", 
        "cx",
        "dx",
        "sp", 
        "bp",
        "si", 
        "di", 
        "es", 
        "cs", 
        "ss", 
        "ds", 
        "ip"
    };
	
	printf(" Register States: ");
	printf("\n");
    
    for(u32 i = 0; i< ArrayCount(g_Register_Infos); ++i)
    {
        const char* RegisterName = Registers[i];
		s32 RegisterValue = TwosComplementToSigned(g_Register_Infos[i]);
		if(RegisterValue != 0)
		{
			fprintf(Dest, "      %s: %i", RegisterName, RegisterValue);
			printf("\n");
		}
    }
	
}

