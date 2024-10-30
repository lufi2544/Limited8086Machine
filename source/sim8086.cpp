
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>
#include <fstream>

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
    Context.CPUContext.b8086 = false;
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
            
			if(IsPrintable(Instruction))
			{
				PrintInstruction(Instruction, stdout);
			}
			
			s32 PreviousSegmentedAccessOffset = At.SegmentOffset;
			UpdateContext(&Context, Instruction);
			UpdateRegisterValues(&Context, Instruction, &At, Memory);
			
			printf("\n");
			
			
			s32 CountOffset = At.SegmentOffset - PreviousSegmentedAccessOffset;
			if(CountOffset != 0)
			{
				CountOffset = ~CountOffset + 1;
			}
			
			// Updating the bytes to read, depending on the At segmeneted acces, due to jump instructions modifying the IP.
			Count += CountOffset;
		}
		else
		{
			fprintf(stderr, "ERROR: Unrecognized binary in instruction stream.\n");
			break;
		}
	}
    
	PrintRegistersState(stdout);
	printf(" Total Program Cycles: %i \n", Context.CPUContext.TotalCycles);
}

int main (int ArgCount, char** Args)
{
	memory* Memory = (memory*)malloc(sizeof(memory));
	
    char* FileName = "listing_0056_estimating_cycles";
    
    // @BytesRead, number of bytes (Instruction + Additional Instruction flags)
    u32 BytesRead = LoadMemoryFromFile(FileName, Memory, 0);
    printf("; disassembly: %s \n", FileName);
    printf("bits 16\n");
    DisAsm8086(Memory, BytesRead, {});
	
	
	
	/*
	// Dumping memory to a file for reading externaly
	std::ofstream ofs("memory-dump.data", std::ofstream::binary);
	for(int ArgIdx = 0; ArgIdx < ArgCount; ++ArgIdx)
	{
		char* Arg = Args[ArgIdx];
		for(int MemoryByteIdx = 0; MemoryByteIdx < sizeof(memory); ++MemoryByteIdx)
		{
			u8 MemValue = Memory->Bytes[MemoryByteIdx];
			ofs << MemValue;
		}
		
	}
	
	ofs.close();
*/
	
	return 0;
}