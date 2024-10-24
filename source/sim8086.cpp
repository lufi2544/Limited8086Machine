
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>

#include "sim86_decode.h"
#include "sim86_memory.h"
#include "sim86_decode.h"
#include "sim86_text.h"


u32 SignedToTwosComplement(u16 Number)
{
	u16 a = ((~Number + 1));
	return a;
}

void DisAsm8086(memory* Memory, u32 DisAsmByteCount, segmented_access DisAsmStart)
{
	segmented_access At = DisAsmStart;
	disasm_context Context = DefaultContext();
	u32 Count = DisAsmByteCount;
	while(Count)
	{
		instruction Instruction = DecodeInstruction(&Context, Memory, &At);
		if(Instruction.Op)
		{
			if(Count >= Instruction.Size)
			{
				Count -= Instruction.Size;
			}
			else
			{
				fprintf(stderr, "ERROR: Instruction extends outside disassembly region\n");
				break;
			}
            
			
			s32 PreviousSegmentedAccessOffset = At.SegmentOffset;
			UpdateContext(&Context, Instruction);
			UpdateRegisterValues(&Context, Instruction, &At, Memory);
			
			
			s32 CountOffset = At.SegmentOffset - PreviousSegmentedAccessOffset;
			if(CountOffset != 0)
			{
				CountOffset = ~CountOffset + 1;
			}
			
			// Updating the bytes to read, depending on the At segmeneted acces, due to jump instructions modifying the IP.
			Count += CountOffset;
			
			if(IsPrintable(Instruction))
			{
				PrintInstruction(Instruction, stdout);
				printf("\n");
			}
		}
		else
		{
			fprintf(stderr, "ERROR: Unrecognized binary in instruction stream.\n");
			break;
		}
	}
    
	PrintRegistersState(stdout);
}

int main (int ArgCount, char** Args)
{
	memory* Memory = (memory*)malloc(sizeof(memory));
	
    
    char* FileName = "listing_0052_memory_add_loop";
    
    // @BytesRead, number of bytes (Instruction + Additional Instruction flags)
    u32 BytesRead = LoadMemoryFromFile(FileName, Memory, 0);
    printf("; disassembly: \n", FileName);
    printf("bits 16\n");
    DisAsm8086(Memory, BytesRead, {});
	
	return 0;
}