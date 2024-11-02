#include "stack.h"
#include "sim86_memory.h"
#include "sim86_decode.h"


void stack::Push(register_index Register)
{
	u32 RegisterToPush = GetRegisterValue(Register);
	u32 StackSegment = GetRegisterValue(register_index::Register_ss);
	u32& StackRegisterValue = GetRegisterValue(register_index::Register_sp);
	
	// Writing low bits and high bits in stack memory
	WriteMemory(RegisterToPush & 0xFF, StackSegment, StackRegisterValue, 0, Memory);
	WriteMemory((((RegisterToPush & 0x00FF) >> 8)), StackSegment, StackRegisterValue, 1, Memory);
	
	// @TODO would be cool to have a list for checking the order of pop and push, to check stack memory restoring order.
	// sp - 2
	StackRegisterValue -= 2;
	
}

void stack::Pop(register_index Register, u8 Num/* = 1*/)
{
	u32& RegisterToPop = GetRegisterValue(Register);
	u32 StackSegment = GetRegisterValue(register_index::Register_ss);
	u32& StackRegisterValue = GetRegisterValue(register_index::Register_sp);
	
	u8 LowBits = ReadMemory(Memory, StackSegment, StackRegisterValue, 0);
	u16 HighBits = ReadMemory(Memory, StackSegment, StackRegisterValue, 1);
	
	// Restore the value to the Regiter to POP
	RegisterToPop = (HighBits << 8) & LowBits;
	
	// Add Up 2 to the Register
	StackRegisterValue += 2;
}
