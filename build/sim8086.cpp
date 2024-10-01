
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>

#include "sim86_decode.h"
#include "sim86_memory.h"


#define DEBUG_DECODER 1

// OPCODE 

#pragma region OLDCODE
/*
enum class enum_opcode_masks : std::uint8_t
{
	reg_mem_to_from_reg = 0b11111000,
	immediate_to_register = 0b11110000,
	immediate_to_register_mem = 0b11111110,
	immediate_to_register_mem_w = 0b00000001,
	immediate_to_register_w = 0b00001000,
	immediate_to_register_reg = 0b00000111,
	operation_reg_mem = 0b11111100,
	operation_immediate_to_register_memory = 0b11111100,
	operation_immediate_to_accumulator = 0b11111110,
	d_1 = 0b00000010,
	w_0 = 0b00000001
};

enum class enum_opcode : std::uint8_t
{
	mov_reg_to_reg = 0b10001000, // 100010dw
	mov_immediate_to_reg_mem = 0b11000110, // 1100011w
	mov_immediate_to_reg = 0b10110000,
	add_reg_mem = 0b00000000, // 000000dw
	add_immediate_to_register_memory = 0b10000000, // 100000sw
	add_immediate_to_accumulator = 0b00000100, // 0000010w
	adc_reg_mem = 0b00010000, // 000100dw
	adc_immediate_to_register_memory = 0b10000000, // 0b100000sw  
	adc_immediate_to_accumulator = 0b00010100, // 00010100w
	sub_reg_mem = 0b00101000, // 001010dw
	sub_immediate_from_register_memory = 0b10000000, //10000000
	sub_immediate_from_accumulator = 0b00101100, //0010110w
	sbb_reg_mem = 0b00011000, // 000110dw
	sbb_immediate_from_register_memory = 0b10000000, // 100000sw
	sbb_immediate_from_accumulator = 0b00011100, // 0001110w
	cmp_register_memory_and_register = 0b00111000, // 001110dw
	cmp_immediate_with_register_memory = 0b10000000, // 100000sw
	cmp_immediate_with_accumulator = 0b00111100 // 0011110w
};


// REGISTERS DATA

enum class enum_regions_masks : std::uint8_t
{
	mod_mask = 0b11000000,
	reg_mask = 0b00111000,
	rm_mask  = 0b00000111
};

// Mode field encoding
// mm - memory mode
// rm - register mode
enum class enum_mod : std::uint8_t
{
	mm_no_displacement		= 0b00000000, // mod = 00
	mm_displacement_8_bit	= 0b01000000, // mod = 01
	mm_displacement_16_bit  = 0b10000000, // mod = 10
	rm_no_displacement		= 0b11000000  // mod = 11
};

char G_RegStrings[2][8][3] =
{
	{ {"al"}, {"cl"}, {"dl"}, {"bl"}, {"ah"}, {"ch"}, {"dh"}, {"bh"}}, // W = 0
	{ {"ax"}, {"cx"}, {"dx"}, {"bx"}, {"sp"}, {"bp"}, {"si"}, {"di"}}  // W = 1
};

char G_EffAddressCalcRMTable[8][8] =
{
	{"bx + si"}, {"bx + di"}, {"bp + si"}, {"bp + di"}, {"si"}, {"di"}, {"bp"}, {"bx"}
};



typedef std::uint8_t instruction_t;
typedef std::uint16_t d_instruction_t;
typedef instruction_t* instruction;

struct decoder_8086
{
	bool IsNegativeTwosComplement(instruction_t Instruction)
	{
		// As the 8086/88 use the two's complement for representing signed numbers, we have to check the LSF of the data, to determine the sign.
		return Instruction & 0b10000000;
	}

	bool IsNegativeTwosComplement_dw(d_instruction_t D_Instruction)
	{
		return D_Instruction & 0b1000000000000000;
	}

	instruction_t GetNegativeNumber(instruction_t Instruction) const
	{
		Instruction = ~Instruction;
		return Instruction + 1; 
	}

	d_instruction_t GetNegativeNumber_dw(d_instruction_t D_Instruction) const
	{
		D_Instruction = ~D_Instruction;
		return D_Instruction + 1;
	}

	enum class enum_decoder_op : std::uint8_t
	{
		ADD,
		ADC,
		SUB,
		SBB,
		CMP
	};

	enum class enum_decoder_op_mode : std::uint8_t
	{
		Reg_Memory_And_Register_Either,
		Immediate_From_To_Register_Memory,
		Immediate_From_To_Accumulator
	};

	
	// It gives us a buffer with the assembly code decoded.
	// @param FileBuff is the buffer loaded from the encoded binary. 
	void Decode(instruction Instruction, std::size_t InstructionIndex, std::int8_t& Out_InstructionOffset)
	{
		if(!Instruction)
		{
			return;
		}

		Out_InstructionOffset = -1;

		

		// Instruction0
		const char* OpcodeBuffer = nullptr;
		const char* EffectiveAddressBuff = nullptr;
		const instruction_t Instruction_0 = Instruction[InstructionIndex];
		const instruction_t Instruction_1 = Instruction[InstructionIndex + 1];
		
		
		OpcodeBuffer = "mov";
		if((Instruction_0 & (instruction_t)enum_opcode_masks::reg_mem_to_from_reg) == (instruction_t)enum_opcode::mov_reg_to_reg)
		{
			std::size_t W = (std::size_t)(Instruction_0 & (instruction_t)enum_opcode_masks::w_0);
			std::size_t D = (std::size_t)(Instruction_0 & (instruction_t)enum_opcode_masks::d_1);


			// Reg
			const instruction_t Reg = Instruction_1 & (instruction_t)enum_regions_masks::reg_mask;
			const instruction_t Reg_Raw = Reg >> 3;
			const char* DecodedReg = G_RegStrings[W][Reg_Raw];

			// R/M	
			const instruction_t RM = Instruction_1 & (instruction_t)enum_regions_masks::rm_mask;
			const char* DecodedRM = G_RegStrings[W][RM];

			const char* DecodedDestination;

			// TODO: This is not true for the non displacement instructions -> register to memory 
			DecodedDestination = D ? DecodedReg : DecodedRM;

			
			const instruction_t Mod = Instruction_1 & (instruction_t)enum_regions_masks::mod_mask;


			// we can have displacement here if specified the Low bits or the High bits --> ch, cl.
			// in this case we check the 3rd byte for low and 4th byte for high.
			if (Mod == (instruction_t)enum_mod::rm_no_displacement)
			{
				// D == 1 -> Instruction destination in REG
				// D == 0 -> Instruction source in REG
				const char* DecodedSource = D ? DecodedRM : DecodedReg;

				std::printf(("%s %s, %s \n"), OpcodeBuffer, DecodedDestination, DecodedSource);

				Out_InstructionOffset = 2;
			}
			else
			{
				const char* PartialEffectiveAddressBuff = G_EffAddressCalcRMTable[RM];

				// DISPLACEMENT CHECK

				if (Mod == (instruction_t)enum_mod::mm_displacement_8_bit)
				{
					const instruction_t Instruction_2 = Instruction[InstructionIndex + 2];
					instruction_t Displacement_8bit = Instruction_2;

					
					if (D) 
					{
						if (Displacement_8bit > 0)
						{
							if(IsNegativeTwosComplement(Displacement_8bit))
							{
								Displacement_8bit = GetNegativeNumber(Displacement_8bit);
								std::printf("%s %s, [%s - %i] \n", OpcodeBuffer, DecodedDestination, PartialEffectiveAddressBuff, Displacement_8bit);
							}
							else
							{
								std::printf("%s %s, [%s + %i] \n", OpcodeBuffer, DecodedDestination, PartialEffectiveAddressBuff, Displacement_8bit);
							}

							
						}
						else
						{
							std::printf("%s %s, [%s] \n", OpcodeBuffer, DecodedDestination, PartialEffectiveAddressBuff);
						}
					}
					else
					{
						if (Displacement_8bit > 0)
						{
							if(IsNegativeTwosComplement(Displacement_8bit))
							{
								Displacement_8bit = GetNegativeNumber(Displacement_8bit);
								std::printf("%s [%s - %i], %s \n", OpcodeBuffer, PartialEffectiveAddressBuff, Displacement_8bit, DecodedReg);
							}
							else
							{
								std::printf("%s [%s + %i], %s \n", OpcodeBuffer, PartialEffectiveAddressBuff, Displacement_8bit, DecodedReg);
							}
						}
						else
						{
							std::printf("%s [%s], %s \n", OpcodeBuffer, PartialEffectiveAddressBuff, DecodedReg);
						}
					}

					Out_InstructionOffset = 3;
				}
				else if (Mod == (instruction_t)enum_mod::mm_displacement_16_bit)
				{
					const d_instruction_t Instruction_2 = Instruction[InstructionIndex + 2]; // L bits
					const d_instruction_t Instruction_3 = Instruction[InstructionIndex + 3]; // H bits

					d_instruction_t Displacement_16bit = (Instruction_3 << 8) | Instruction_2;
					

					if(D)
					{
						if(IsNegativeTwosComplement_dw(Displacement_16bit))
						{
							Displacement_16bit = GetNegativeNumber_dw(Displacement_16bit);
							std::printf("%s %s, [%s - %i] \n", OpcodeBuffer, DecodedReg, PartialEffectiveAddressBuff, Displacement_16bit);
						}
						else
						{
							std::printf("%s %s, [%s + %i] \n", OpcodeBuffer, DecodedReg, PartialEffectiveAddressBuff, Displacement_16bit);
						}
					}
					else
					{
						if(IsNegativeTwosComplement_dw(Displacement_16bit))
						{
							Displacement_16bit = GetNegativeNumber_dw(Displacement_16bit);
							std::printf("%s [%s - %i], %s \n", OpcodeBuffer, PartialEffectiveAddressBuff, Displacement_16bit, DecodedReg);
						}
						else
						{
							std::printf("%s [%s + %i], %s \n", OpcodeBuffer, PartialEffectiveAddressBuff, Displacement_16bit, DecodedReg);
						}
					}

					Out_InstructionOffset = 4;
				}
				else
				{
					// NO DISPLACEMENT 
					
					// DIRECT ADDRESS. 
					// Here we would handle special case for the Mode == 0b00 and R/M = 110, 16 bit displacement will follow.
					if(RM == 0b00000110)
					{
						const d_instruction_t Instruction_2 = Instruction[InstructionIndex + 2];
						const d_instruction_t Instruction_3 = Instruction[InstructionIndex + 3];
						const d_instruction_t Displacement_16bit = (Instruction_3 << 8) | Instruction_2;

						if(D)
						{
							std::printf("%s %s, [%i] \n", OpcodeBuffer, DecodedReg, Displacement_16bit);
						}
						else
						{
							std::printf("%s [%i], %s \n", OpcodeBuffer, Displacement_16bit, DecodedReg);
						}

						Out_InstructionOffset = 4;
					}
					else
					{
						// mov al, [bx + si]
						EffectiveAddressBuff = PartialEffectiveAddressBuff;
						if(D)
						{
							std::printf("%s %s, [%s] \n", OpcodeBuffer, DecodedReg, EffectiveAddressBuff);
						}
						else
						{
							std::printf("%s [%s], %s \n", OpcodeBuffer, EffectiveAddressBuff, DecodedReg);
						}

						Out_InstructionOffset = 2;
					}
				}

			}

		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::immediate_to_register_mem) == (instruction_t)enum_opcode::mov_immediate_to_reg_mem)
		{
			// mov [bx + 2], 12
			const instruction_t W = Instruction_0 & (instruction_t)(enum_opcode_masks::immediate_to_register_mem_w);
			const instruction_t Mod = (Instruction_1 & (instruction_t)(enum_regions_masks::mod_mask));
			const instruction_t R_M = (Instruction_1 & (instruction_t)(enum_regions_masks::rm_mask));
			char* DecodedEffectiveAddressBuff = G_EffAddressCalcRMTable[R_M];
			
			// TODO Add the direct address 	
			if(Mod == (instruction_t)enum_mod::mm_no_displacement)
			{
				const d_instruction_t Data_L = Instruction[InstructionIndex + 2];
				if(W)
				{
					const d_instruction_t Data_H = Instruction[InstructionIndex + 3];
					const d_instruction_t Data = Data_H << 8 | Data_L;
					
					std::printf("%s [%s], word %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Data);
					Out_InstructionOffset = 4; 
				}
				else
				{
					std::printf("%s [%s], byte %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Data_L);
					Out_InstructionOffset = 3;
				}
			}
			else if(Mod == (instruction_t)enum_mod::mm_displacement_8_bit)
			{
				const instruction_t Displacement_8Bit = Instruction[InstructionIndex + 2];
				const d_instruction_t Data_L = Instruction[InstructionIndex + 3];
				if(W)
				{
					const d_instruction_t Data_H = Instruction[InstructionIndex + 4];
					const d_instruction_t Data = Data_H << 8 | Data_L;
					
					std::printf("%s [%s + %i], word %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Displacement_8Bit, Data);
					Out_InstructionOffset = 5;
				}
				else
				{
					std::printf("%s [%s + %i], byte %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Displacement_8Bit, Data_L);
					Out_InstructionOffset = 4;
				}
			}
			else if(Mod == (instruction_t)enum_mod::mm_displacement_16_bit)
			{
				const d_instruction_t Displacement_16Bit_L = Instruction[InstructionIndex + 2];
				const d_instruction_t Displacement_16Bit_H = Instruction[InstructionIndex + 3];
				const d_instruction_t Displacement_16Bit = Displacement_16Bit_H << 8 | Displacement_16Bit_L;
				
				const d_instruction_t Data_L = Instruction[InstructionIndex + 4];
				if(W)
				{
					
					const d_instruction_t Data_H = Instruction[InstructionIndex + 5];
					const d_instruction_t Data = Data_H << 8 | Data_L;
					std::printf("%s [%s + %i], word %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Displacement_16Bit, Data);
					Out_InstructionOffset = 6;
				}
				else
				{
					std::printf("%s [%s + %i], byte %i \n", OpcodeBuffer, DecodedEffectiveAddressBuff, Displacement_16Bit, Data_L);
					Out_InstructionOffset = 5;
				}
			}
			
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::immediate_to_register) == (instruction_t)enum_opcode::mov_immediate_to_reg)
		{
			// mov cx, 12
			const bool W = (Instruction_0 & (instruction_t)enum_opcode_masks::immediate_to_register_w) >> 3;
			const std::size_t Reg = (Instruction_0 & (instruction_t)enum_opcode_masks::immediate_to_register_reg);
			

			if(W)
			{
				const d_instruction_t DInstruction_1 = Instruction_1;
				const d_instruction_t DInstruction_2 = Instruction[InstructionIndex + 2];
				
				std::printf("mov %s, %i \n", G_RegStrings[W][Reg], (DInstruction_1 | (DInstruction_2 << 8)));
			}
			else
			{					
				std::printf("mov %s, %i \n", G_RegStrings[W][Reg], Instruction_1);
			}
			
			Out_InstructionOffset = W ? 3 : 2;
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_reg_mem) == (instruction_t)enum_opcode::add_reg_mem)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADD, enum_decoder_op_mode::Reg_Memory_And_Register_Either, Out_InstructionOffset);
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::add_immediate_to_register_memory && ((Instruction_1 & (instruction_t)(0b00111000)) == 0b00000000) )
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADD, enum_decoder_op_mode::Immediate_From_To_Register_Memory, Out_InstructionOffset);
			
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_accumulator) == (instruction_t)enum_opcode::add_immediate_to_accumulator)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADD, enum_decoder_op_mode::Immediate_From_To_Accumulator, Out_InstructionOffset);
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_reg_mem) == (instruction_t)enum_opcode::adc_reg_mem)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADC, enum_decoder_op_mode::Reg_Memory_And_Register_Either, Out_InstructionOffset);
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::adc_immediate_to_register_memory && ((Instruction_1 & (instruction_t)(0b00111000)) == 0b00010000))
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADC, enum_decoder_op_mode::Immediate_From_To_Register_Memory, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_accumulator) == (instruction_t)enum_opcode::adc_immediate_to_accumulator)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::ADC, enum_decoder_op_mode::Immediate_From_To_Accumulator, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_reg_mem) == (instruction_t)enum_opcode::sub_reg_mem)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SUB, enum_decoder_op_mode::Reg_Memory_And_Register_Either, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::sub_immediate_from_register_memory && ((Instruction_1 & (instruction_t)(0b00111000)) == 0b00101000))
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SUB, enum_decoder_op_mode::Immediate_From_To_Register_Memory, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_accumulator) == (instruction_t)enum_opcode::sub_immediate_from_accumulator)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SUB, enum_decoder_op_mode::Immediate_From_To_Accumulator, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_reg_mem) == (instruction_t)enum_opcode::sbb_reg_mem)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SBB, enum_decoder_op_mode::Reg_Memory_And_Register_Either, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::sbb_immediate_from_register_memory && ((Instruction_1 & (instruction_t)(0b00111000)) == 0b00011000))
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SBB, enum_decoder_op_mode::Immediate_From_To_Register_Memory, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_accumulator) == (instruction_t)enum_opcode::sbb_immediate_from_accumulator)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::SBB, enum_decoder_op_mode::Immediate_From_To_Accumulator, Out_InstructionOffset);
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::cmp_register_memory_and_register)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::CMP, enum_decoder_op_mode::Reg_Memory_And_Register_Either, Out_InstructionOffset);
		}
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_register_memory) == (instruction_t)enum_opcode::cmp_immediate_with_register_memory && ((Instruction_1 & (instruction_t)(0b00111000)) == 0b00111000))
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::CMP, enum_decoder_op_mode::Immediate_From_To_Register_Memory, Out_InstructionOffset);
		}
		else if ((Instruction_0 & (instruction_t)enum_opcode_masks::operation_immediate_to_accumulator) == (instruction_t)enum_opcode::cmp_immediate_with_accumulator)
		{
			DecodeOpInstruction(Instruction, InstructionIndex, enum_decoder_op::CMP, enum_decoder_op_mode::Immediate_From_To_Accumulator, Out_InstructionOffset);
		}
		// Solve the bug with the word/byte in SUB
		
		if(Out_InstructionOffset == -1)
		{
			std::cout << "Instruction not implemented!\n";			
#ifdef DEBUG_DECODER

			if (DEBUG_DECODER)
			{
				for (int i = 0; i < 4; ++i)
				{
					std::cout << std::bitset<8>(Instruction[InstructionIndex + i]).to_string() << std::endl;
				}
			}

#endif // DEBUG_DECODER
		}
		else 
		{

			
#ifdef DEBUG_DECODER

			if (DEBUG_DECODER) 
			{
				for (int i = 0; i < Out_InstructionOffset; ++i)
				{
					std::cout << std::bitset<8>(Instruction[InstructionIndex + i]).to_string() << std::endl;
				}
			}

#endif // DEBUG_DECODER
		}
	}

	void DecodeOpInstruction(instruction Instruction, instruction_t InstructionIndex, enum_decoder_op Op, enum_decoder_op_mode Op_Mode, int8_t& Out_InstructionOffset)
	{
		char* OpcodeBuffer;
		switch (Op)
		{
		case decoder_8086::enum_decoder_op::ADD:
			OpcodeBuffer = "add";
			break;
		case decoder_8086::enum_decoder_op::ADC:
			OpcodeBuffer = "adc";
			break;
		case decoder_8086::enum_decoder_op::SUB:
			OpcodeBuffer = "sub";
			break;
		case decoder_8086::enum_decoder_op::SBB:
			OpcodeBuffer = "sbb";
			break;

		case decoder_8086::enum_decoder_op::CMP:
			OpcodeBuffer = "cmp";
			break;
		default:
			break;
		}

		const instruction_t Instruction_0 = Instruction[InstructionIndex];
		const instruction_t Instruction_1 = Instruction[InstructionIndex + 1];

		const instruction_t W = Instruction_0 & (instruction_t)enum_opcode_masks::w_0;
		const instruction_t D_Or_S = Instruction_0 & (instruction_t)enum_opcode_masks::d_1;

		switch (Op_Mode)
		{
		case decoder_8086::enum_decoder_op_mode::Reg_Memory_And_Register_Either:

		{
			const instruction_t Mod = Instruction_1 & (instruction_t)enum_regions_masks::mod_mask;
			const instruction_t Reg = (Instruction_1 & (instruction_t)enum_regions_masks::reg_mask) >> 3;
			char* DecodedReg = G_RegStrings[W][Reg];
			const instruction_t RM = (Instruction_1 & (instruction_t)enum_regions_masks::rm_mask);

			if (Mod == (instruction_t)enum_mod::mm_no_displacement)
			{
				// Special case for direct address add ax, [334]
				if (RM == 0b00000110)
				{
					const d_instruction_t Offset_L = Instruction[InstructionIndex + 2];
					const d_instruction_t Offset_H = Instruction[InstructionIndex + 3];
					const d_instruction_t Offset_16bit = Offset_L | (Offset_H << 8);

					if (D_Or_S)
					{
						std::printf("%s %s, [%i]  \n", OpcodeBuffer, DecodedReg, Offset_16bit);
					}
					else
					{
						std::printf("%s [%i], %s  \n", OpcodeBuffer, Offset_16bit, DecodedReg);
					}

					Out_InstructionOffset = 4;
				}

				const char* DecodedRM = G_EffAddressCalcRMTable[RM];
				if (D_Or_S)
				{
					std::printf("%s %s, [%s]  \n", OpcodeBuffer, DecodedReg, DecodedRM);
				}
				else
				{
					std::printf("%s [%s], %s  \n", OpcodeBuffer, DecodedRM, DecodedReg);
				}


				Out_InstructionOffset = 2;
			}
			else if (Mod == (instruction_t)enum_mod::mm_displacement_8_bit)
			{
				const instruction_t Instruction_2 = Instruction[InstructionIndex + 2];
				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				const instruction_t Offset_8bits = Instruction_2;

				if (D_Or_S)
				{
					if (Offset_8bits > 0)
					{
						std::printf("%s %s, [%s + %i]  \n", OpcodeBuffer, DecodedReg, EffectiveAddress, Offset_8bits);
					}
					else
					{
						std::printf("%s %s, [%s]  \n", OpcodeBuffer, DecodedReg, EffectiveAddress);
					}
				}
				else
				{
					std::printf("%s [%s + %i], %s  \n", OpcodeBuffer, EffectiveAddress, Offset_8bits, DecodedReg);
				}

				Out_InstructionOffset = 3;
			}
			else if (Mod == (instruction_t)enum_mod::mm_displacement_16_bit)
			{
				const d_instruction_t Displacement_L = Instruction[InstructionIndex + 2];
				const d_instruction_t Displacement_H = Instruction[InstructionIndex + 3];
				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				const d_instruction_t Offset_16bits = Displacement_L | (Displacement_H << 8);

				if (D_Or_S)
				{
					std::printf("%s %s, [%s + %i]  \n", OpcodeBuffer, DecodedReg, EffectiveAddress, Offset_16bits);
				}
				else
				{
					std::printf("%s [%s + %i], %s  \n", OpcodeBuffer, EffectiveAddress, Offset_16bits, DecodedReg);
				}

				Out_InstructionOffset = 4;
			}
			else if (Mod == (instruction_t)enum_mod::rm_no_displacement)
			{
				char* DecodedRM = G_RegStrings[W][RM];

				std::printf("%s %s, %s  \n", OpcodeBuffer, D_Or_S ? DecodedReg : DecodedRM, D_Or_S ? DecodedRM : DecodedReg);
				Out_InstructionOffset = 2;
			}
		}

		break;
		case decoder_8086::enum_decoder_op_mode::Immediate_From_To_Register_Memory:

		{
			const instruction_t Mod = Instruction_1 & (instruction_t)enum_regions_masks::mod_mask;
			const instruction_t RM = (Instruction_1 & (instruction_t)enum_regions_masks::rm_mask);

			if (Mod == (instruction_t)enum_mod::rm_no_displacement)
			{

				// Here is important to check the S flag, as it means  sign extension.
				//This is a smart way of the encoder to tell the CPU to use a 16 bit word data for the operation,
				//most likely because we are operating on a double word register, but only encode 1 byte in the instruction... So cool.
				//const char* DecodedRM = G_RegStrings[W][RM];

				// check if operating in double word
				if (W)
				{
					d_instruction_t DataL = Instruction[InstructionIndex + 2];

					// Check if sign extension
					if (D_Or_S)
					{
						if (IsNegativeTwosComplement_dw(DataL))
						{
							d_instruction_t Negative_16bit = GetNegativeNumber_dw(DataL);
							std::printf("%s %s, -%i  \n", OpcodeBuffer, DecodedRM, Negative_16bit);
						}
						else
						{
							std::printf("%s %s, %i  \n", OpcodeBuffer, DecodedRM, DataL);
						}

						Out_InstructionOffset = 3;

					}
					else
					{
						d_instruction_t Data_H = Instruction[InstructionIndex + 3];
						d_instruction_t Data_16bit = (Data_H << 8) | (DataL);

						if (IsNegativeTwosComplement_dw(Data_16bit))
						{
							d_instruction_t Negative_16bit = GetNegativeNumber_dw(Data_16bit);
							std::printf("%s %s, -%i  \n", OpcodeBuffer, DecodedRM, Negative_16bit);
						}
						else
						{
							std::printf("%s %s, %i  \n", OpcodeBuffer, DecodedRM, Data_16bit);
						}

						Out_InstructionOffset = 4;
					}
				}
				else
				{
					instruction_t Data_8bit = Instruction[InstructionIndex + 2];
					if (IsNegativeTwosComplement(Data_8bit))
					{
						instruction_t NegativeData_8bit = GetNegativeNumber(Data_8bit);
						std::printf("%s %s, -%i  \n", OpcodeBuffer, DecodedRM, NegativeData_8bit);
					}
					else
					{
						std::printf("%s %s, %i  \n", OpcodeBuffer, DecodedRM, Data_8bit);
					}

					Out_InstructionOffset = 3;
				}
			}
			else if (Mod == (instruction_t)enum_mod::mm_displacement_8_bit)
			{
				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				instruction_t Displacement_8bit = Instruction[InstructionIndex + 2];
				d_instruction_t Data_L = Instruction[InstructionIndex + 3];

				if (W)
				{
					if (D_Or_S)
					{
						if (IsNegativeTwosComplement(Displacement_8bit))
						{
							d_instruction_t Negative_Displacement_8bit = GetNegativeNumber_dw(Displacement_8bit);
							if (IsNegativeTwosComplement(Data_L))
							{
								std::printf("%s word [%s - %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, GetNegativeNumber_dw(Data_L));
							}
							else
							{
								std::printf("%s word [%s - %i], %i  \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, Data_L);
							}

						}
						else
						{
							if (IsNegativeTwosComplement(Data_L))
							{
								std::printf("%s word [%s + %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, GetNegativeNumber_dw(Data_L));
							}
							else
							{
								std::printf("%s word [%s + %i], %i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, Data_L);
							}
						}

						Out_InstructionOffset = 4;
					}
					else
					{
						d_instruction_t Data_H = Instruction[InstructionIndex + 4];
						d_instruction_t Data = (Data_H << 8) | Data_L;

						if (IsNegativeTwosComplement(Displacement_8bit))
						{
							d_instruction_t Negative_Displacement_8bit = GetNegativeNumber_dw(Displacement_8bit);
							if (IsNegativeTwosComplement_dw(Data))
							{
								std::printf("%s word [%s - %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, GetNegativeNumber_dw(Data));
							}
							else
							{
								std::printf("%s word [%s - %i], %i  \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, Data);
							}

						}
						else
						{
							if (IsNegativeTwosComplement_dw(Data))
							{
								std::printf("%s word [%s + %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, GetNegativeNumber_dw(Data));
							}
							else
							{
								std::printf("%s word [%s + %i], %i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, Data);
							}
						}

						Out_InstructionOffset = 5;
					}

				}
				else
				{
					if (IsNegativeTwosComplement(Displacement_8bit))
					{
						if (IsNegativeTwosComplement(Data_L))
						{
							std::printf("%s byte [%s - %i], -%i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber(Displacement_8bit), GetNegativeNumber_dw(Data_L));
						}
						else
						{
							std::printf("%s byte [%s - %i], %i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber(Displacement_8bit), Data_L);
						}
					}
					else
					{
						if (IsNegativeTwosComplement(Data_L))
						{
							std::printf("%s byte [%s + %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, GetNegativeNumber_dw(Data_L));
						}
						else
						{
							std::printf("%s byte [%s + %i], %i  \n", OpcodeBuffer, EffectiveAddress, Displacement_8bit, Data_L);
						}
					}

					Out_InstructionOffset = 4;
				}

			}
			else if (Mod == (instruction_t)enum_mod::mm_displacement_16_bit)
			{

				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				d_instruction_t Displacement_L_16bit = Instruction[InstructionIndex + 2];
				d_instruction_t Displacement_H_16bit = Instruction[InstructionIndex + 3];
				d_instruction_t Displacement_16bit = (Displacement_H_16bit << 8) | Displacement_L_16bit;
				d_instruction_t Data_L = Instruction[InstructionIndex + 4];

				if (W)
				{
					if (D_Or_S)
					{
						if (IsNegativeTwosComplement_dw(Displacement_16bit))
						{
							d_instruction_t Negative_Displacement_8bit = GetNegativeNumber_dw(Displacement_16bit);
							if (IsNegativeTwosComplement(Data_L))
							{
								std::printf("%s word [%s - %i], -%i \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, GetNegativeNumber_dw(Data_L));
							}
							else
							{
								std::printf("%s word [%s - %i], %i \n", OpcodeBuffer, EffectiveAddress, Negative_Displacement_8bit, Data_L);
							}

						}
						else
						{
							if (IsNegativeTwosComplement(Data_L))
							{
								std::printf("%s word [%s + %i], -%i \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, GetNegativeNumber_dw(Data_L));
							}
							else
							{
								std::printf("%s word [%s + %i], %i \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, Data_L);
							}
						}

						Out_InstructionOffset = 5;
					}
					else
					{
						d_instruction_t Data_H = Instruction[InstructionIndex + 4];
						d_instruction_t Data = (Data_H << 8) | Data_L;

						if (IsNegativeTwosComplement_dw(Displacement_16bit))
						{
							if (IsNegativeTwosComplement_dw(Data))
							{
								std::printf("%s word [%s - %i], -%i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber_dw(Displacement_16bit), GetNegativeNumber_dw(Data));
							}
							else
							{
								std::printf("%s word [%s - %i], %i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber_dw(Displacement_16bit), Data);
							}

						}
						else
						{
							if (IsNegativeTwosComplement_dw(Data))
							{
								std::printf("%s word [%s + %i], -%i \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, GetNegativeNumber_dw(Data));
							}
							else
							{
								std::printf("%s word [%s + %i], %i \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, Data);
							}
						}

						Out_InstructionOffset = 6;
					}

				}
				else
				{
					if (IsNegativeTwosComplement(Displacement_16bit))
					{
						if (IsNegativeTwosComplement(Data_L))
						{
							std::printf("%s byte [%s - %i], -%i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber(Displacement_16bit), GetNegativeNumber_dw(Data_L));
						}
						else
						{
							std::printf("%s byte [%s - %i], %i \n", OpcodeBuffer, EffectiveAddress, GetNegativeNumber(Displacement_16bit), Data_L);
						}
					}
					else
					{
						if (IsNegativeTwosComplement(Data_L))
						{
							std::printf("%s byte [%s + %i], -%i  \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, GetNegativeNumber_dw(Data_L));
						}
						else
						{
							std::printf("%s byte [%s + %i], %i  \n", OpcodeBuffer, EffectiveAddress, Displacement_16bit, Data_L);
						}
					}

					Out_InstructionOffset = 5;
				}
			}
			else if (Mod == (instruction_t)enum_mod::mm_no_displacement)
			{
				// Special case for direct address add ax, [334]
				if (RM == 0b00000110)
				{
					const d_instruction_t Offset_L = Instruction[InstructionIndex + 2];
					const d_instruction_t Offset_H = Instruction[InstructionIndex + 3];
					const d_instruction_t Offset_16bit = Offset_L | (Offset_H << 8);
					d_instruction_t Data_L = Instruction[InstructionIndex + 4];

					if (W)
					{
						if(D_Or_S)
						{
							std::printf("%s word [%i], %i  \n", OpcodeBuffer, Offset_16bit, Data_L);
							Out_InstructionOffset = 5;
						}
						else 
						{
							d_instruction_t Data_H = Instruction[InstructionIndex + 5];
							d_instruction_t Data = (Data_H << 8) | Data_L;
							std::printf("%s word [%i], %i  \n", OpcodeBuffer, Offset_16bit, Data);
							Out_InstructionOffset = 6;
						}

					}
					else
					{
						std::printf("%s byte [%i], %i  \n", OpcodeBuffer, Offset_16bit, Data_L);
						Out_InstructionOffset = 5;
					}


				}
				else
				{
					const char* DecodedRM = G_EffAddressCalcRMTable[RM];
					d_instruction_t Data_L = Instruction[InstructionIndex + 2];

					if (W)
					{
						// Sign Extension to pack in 8 bit but working with 16 bit registers
						if(D_Or_S)
						{
							std::printf("%s word [%s], %i  \n", OpcodeBuffer, DecodedRM, Data_L);
							Out_InstructionOffset = 3;
						}
						else
						{
							d_instruction_t Data_H = Instruction[InstructionIndex + 3];
							d_instruction_t Data = (Data_H << 8) | Data_L;
							std::printf("%s word [%s], %i  \n", OpcodeBuffer, DecodedRM, Data);
							Out_InstructionOffset = 4;
						}


					}
					else
					{
						std::printf("%s byte [%s], %i  \n", OpcodeBuffer, DecodedRM, Data_L);
						Out_InstructionOffset = 3;
					}
				}
			}
		}
		break;
		case decoder_8086::enum_decoder_op_mode::Immediate_From_To_Accumulator:

		{
			d_instruction_t Data_8bit_L = Instruction_1;
			if (W)
			{
				d_instruction_t Data_8bit_H = Instruction[InstructionIndex + 2];
				d_instruction_t Data = (Data_8bit_H << 8) | Data_8bit_L;

				std::printf("%s ax, %i \n", OpcodeBuffer, Data);
				Out_InstructionOffset = 3;
			}
			else
			{
				if (IsNegativeTwosComplement(Data_8bit_L))
				{
					std::printf("%s al, -%i \n", OpcodeBuffer, GetNegativeNumber(Data_8bit_L));
				}
				else
				{
					std::printf("%s al, %i \n", OpcodeBuffer, Data_8bit_L);
				}

				Out_InstructionOffset = 2;
			}
		}

			break;
		default:
			break;
		}
	}

};


*/

#pragma endregion OLDCODE

#include "sim86_decode.h"
#include "sim86_text.h"


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

			UpdateContext(&Context, Instruction);
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
	
}

int main (int ArgCount, char** Args)
{
	memory* Memory = (memory*)malloc(sizeof(memory));
	

		char* FileName = "listing_0039_more_movs";

		// @BytesRead, number of bytes (Instruction + Additional Instruction flags)
		u32 BytesRead = LoadMemoryFromFile(FileName, Memory, 0);
		printf("; disassembly: \n", FileName);
		printf("bits 16\n");
		DisAsm8086(Memory, BytesRead, {});
	
	return 0;
}