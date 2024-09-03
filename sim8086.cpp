
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>


// OPCODE 

enum class enum_opcode_masks : std::uint8_t
{
	reg_mem_to_from_reg = 0b11111000,
	immediate_to_register = 0b11110000,
	immediate_to_register_mem = 0b11111110,
	immediate_to_register_mem_w = 0b00000001,
	immediate_to_register_w = 0b00001000,
	immediate_to_register_reg = 0b00000111,
	add_reg_mem = 0b11111100,
	add_immediate_to_register_memory = 0b11111100,
	add_immediate_to_accumulator = 0b11111110,
	d_1 = 0b00000010,
	w_0 = 0b00000001
};

enum class enum_opcode : std::uint8_t
{
	mov_reg_to_reg = 0b10001000, // 100010dw
	mov_immediate_to_reg_mem = 0b11000110, // 1100011w
	mov_immediate_to_reg = 0b10110000,
	add_reg_mem = 0b00000000, // 000000dw
	add_immediate_to_register_memory = 0b10000000, //100000sw
	add_immediate_to_accumulator = 0b00000100, // 0000010w
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
	bool IsDisplacementNegative(instruction_t Instruction)
	{
		// As the 8086/88 use the two's complement for representing signed numbers, we have to check the LSF of the data, to determine the sign.
		return Instruction & 0b10000000;
	}

	bool IsDisplacementNegative_dw(d_instruction_t D_Instruction)
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

		if(false)
		{
			std::cout << std::bitset<8>(Instruction_0).to_string() << std::endl;
			std::cout << std::bitset<8>(Instruction_1).to_string() << std::endl;
			std::cout << std::bitset<8>(Instruction[InstructionIndex + 2]).to_string()<< std::endl;
			std::cout << std::bitset<8>(Instruction[InstructionIndex + 3]).to_string() << std::endl;
		}
		
		
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
							if(IsDisplacementNegative(Displacement_8bit))
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
							if(IsDisplacementNegative(Displacement_8bit))
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
						if(IsDisplacementNegative_dw(Displacement_16bit))
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
						if(IsDisplacementNegative_dw(Displacement_16bit))
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
		else if((Instruction_0 & (instruction_t)enum_opcode_masks::add_reg_mem) == (instruction_t)enum_opcode::add_reg_mem)
		{
			OpcodeBuffer = "add";
			// Check MOD --> Depending on the MOD, then we are going to see the displacement, etc.
			const instruction_t Mod = Instruction_1 & (instruction_t)enum_regions_masks::mod_mask;
			const instruction_t D = Instruction_0 & (instruction_t)enum_opcode_masks::d_1;
			const instruction_t W = Instruction_0 & (instruction_t)enum_opcode_masks::w_0;

			const instruction_t Reg = (Instruction_1 & (instruction_t)enum_regions_masks::reg_mask) >> 3;
			char* DecodedReg = G_RegStrings[W][Reg];

			const instruction_t RM = (Instruction_1 & (instruction_t)enum_regions_masks::rm_mask);

			if(Mod == (instruction_t)enum_mod::mm_no_displacement)
			{
				// Special case for direct address add ax, [334]
				if(RM == 0b00000110)
				{
					const d_instruction_t Offset_L = Instruction[InstructionIndex + 2];
					const d_instruction_t Offset_H = Instruction[InstructionIndex + 3];
					const d_instruction_t Offset_16bit = Offset_L | (Offset_H << 8);

					if(D)
					{
						std::printf("%s %s, [%i]", OpcodeBuffer, DecodedReg, Offset_16bit);
					}
					else
					{
						std::printf("%s [%i], %s", OpcodeBuffer, Offset_16bit, DecodedReg);
					}

					Out_InstructionOffset = 4;
				}

				const char* DecodedRM = G_EffAddressCalcRMTable[RM];
				if(D)
				{
					std::printf("%s %s, %s", OpcodeBuffer, D ? DecodedReg : DecodedRM, D ? DecodedRM : DecodedReg);
				}

				Out_InstructionOffset = 2;
			}
			else if(Mod == (instruction_t)enum_mod::mm_displacement_8_bit)			
			{
				const instruction_t Instruction_2 = Instruction[InstructionIndex + 2];
				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				const instruction_t Offset_8bits = Instruction_2;

				if(D)
				{
					std::printf("%s %s, [%s + %i]", OpcodeBuffer, DecodedReg, EffectiveAddress, Offset_8bits);
				}
				else
				{
					std::printf("%s [%s + %i], %s", OpcodeBuffer, EffectiveAddress, Offset_8bits, DecodedReg);
				}

				Out_InstructionOffset = 3;
			}
			else if(Mod == (instruction_t)enum_mod::mm_displacement_16_bit)
			{
				const d_instruction_t Displacement_L = Instruction[InstructionIndex + 2];
				const d_instruction_t Displacement_H = Instruction[InstructionIndex + 3];
				const char* EffectiveAddress = G_EffAddressCalcRMTable[RM];
				const d_instruction_t Offset_16bits = Displacement_L | (Displacement_H << 8);

				if(D)
				{
					std::printf("%s %s, [%s + %i]", OpcodeBuffer, DecodedReg, EffectiveAddress, Offset_16bits);
				}
				else 
				{
					std::printf("%s [%s + %i], %s", OpcodeBuffer, EffectiveAddress, Offset_16bits, DecodedReg);
				}

				Out_InstructionOffset = 4;
			}
			else if(Mod == (instruction_t)enum_mod::rm_no_displacement)
			{
				char* DecodedRM = G_RegStrings[W][RM];

				std::printf("%s %s, %s", OpcodeBuffer, D ? DecodedReg : DecodedRM, D ? DecodedRM : DecodedReg);
				Out_InstructionOffset = 2;
			}
			
		}
		else if()
		{

		}
		

		if(Out_InstructionOffset == -1)
		{
			std::cout << "Instruction not implemented!\n";			
		}
	}

	
};


int main ()
{
	std::ifstream File("listing_0040_challenge_movs", std::ios::binary);

	if(!File.is_open())
	{
		std::cerr << "The file could not be opened" << std::endl;
		return 1;
	}

	File.seekg(0, std::ios::end);
	std::streampos FileSizeStream = File.tellg();
	File.seekg(0, std::ios::beg);

	// create a buffer to store the data:
	
	std::size_t FileSize = (std::size_t)FileSizeStream;

	// It looks like we have 2 bytes per instruction. But I don't think is always like that.  	
	char* Buffer = new char [FileSize];
	if(File.read(Buffer, FileSize))
	{
		std::cerr << "File read successfully" << std::endl;
	}
	else
	{
		std::cerr << "Error reading the file " << std::endl; 
	}


	File.close();

	std::cout << std::endl;

	instruction AllInstructions = new instruction_t[FileSize];
	for(int j = 0; j < FileSize; ++j)
	{
		AllInstructions[j] = Buffer[j];
	}

	decoder_8086 decoder;
	int i = 0;	
	while(i < FileSize)
	{
		// no displacement instruction		decoder_8086 Processor;
		std::int8_t InstructionOffset = 0;
		decoder.Decode(AllInstructions, i, InstructionOffset);
		if(InstructionOffset < 0)
		{
			break;
		}
		
		i += InstructionOffset;
	}

	
	delete[] Buffer;
	delete[] AllInstructions;

	std::cout << std::endl;

	return 0;
}
