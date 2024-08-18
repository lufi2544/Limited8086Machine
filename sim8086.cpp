
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>


// OPCODE 

enum class enum_opcode_masks : std::uint8_t
{
	d = 0b00000010,
	w = 0b00000001
};

enum class enum_opcode : std::uint8_t
{
	mov = 0b10001000
};


// REGISTERS DATA

enum class enum_registers_masks : std::uint8_t
{
	mod_mask = 0b1100000000,
	reg_mask = 0b00111000,
	rm_mask = 0b00000111
};

// Mode field encoding
// mm - memory mode
// rm - register mode
enum class enum_mod : std::uint8_t
{
	mm_no_displacement		= 0b00000000,
	mm_displacement_8_bit	= 0b01000000,
	mm_displacement_16_bit  = 0b10000000,
	rm_no_displacement		= 0b11000000
};

char RegStrings[2][8][3] =
{
	{ {"al"}, {"cl"}, {"dl"}, {"bl"}, {"ah"}, {"ch"}, {"dh"}, {"bh"}}, // W = 0
	{ {"ax"}, {"cx"}, {"dx"}, {"bx"}, {"sp"}, {"bp"}, {"si"}, {"di"}}  // W = 1
};


typedef std::uint8_t instruction_t;
typedef const instruction_t* instruction;

struct processor_8086
{
	// It gives us a buffer with the assembly code decoded.
	// @param FileBuff is the buffer loaded from the encoded binary. 
	void Decode(instruction Instruction)
	{
		if(!Instruction)
		{
			return;
		}


		// Instruction0
		const char* OpcodeBuffer;
		instruction_t Instruction_0 = Instruction[0];
		if(Instruction_0 & (instruction_t)enum_opcode::mov)
		{
			OpcodeBuffer = "mov";
		}

		std::size_t W = (std::size_t)(Instruction_0 & (instruction_t)enum_opcode_masks::w);
		std::size_t D = (std::size_t)(Instruction_0 & (instruction_t)enum_opcode_masks::d);


		// Instrution1
		instruction_t Instruction_1 = Instruction[1];
		const instruction_t Mod = Instruction_1 & (instruction_t)enum_registers_masks::mod_mask;
		if(Instruction_1 & (instruction_t)(enum_mod::rm_no_displacement))
		{
			// Figure out if the mode changes anything in these examples.
		}

		const char* DecodedSource;
		const char* DecodedDestination;

		// Reg
		const instruction_t Reg = Instruction_1 & (instruction_t)enum_registers_masks::reg_mask;
		const instruction_t Reg_Raw = Reg >> 3;
		const char* DecodedReg = RegStrings[W][Reg_Raw];

		// R/M	
		const instruction_t RM = Instruction_1 & (instruction_t)enum_registers_masks::rm_mask;
		const char* DecodedRM = RegStrings[W][RM];

		// D == 1 -> Instruction destination in REG
		// D == 0 -> Instruction source in REG
		DecodedDestination = D ? DecodedReg : DecodedRM;
		DecodedSource = D ? DecodedRM : DecodedReg;

		std::printf(("%s %s, %s \n"), OpcodeBuffer, DecodedDestination, DecodedSource);
	}

};




int main ()
{
	std::ifstream File("listing_0038_many_register_mov", std::ios::binary);

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


	for(int i = 0; i < FileSize; i+=2)
	{
		const std::uint8_t Num = Buffer[i];
		const std::uint8_t Num1 = Buffer[i + 1];

		std::uint8_t Instruction[2];
		Instruction[0] = Num;
		Instruction[1] = Num1;

		processor_8086 Processor;
		Processor.Decode(Instruction);
	}

	
	delete[] Buffer;

	std::cout << std::endl;

	return 0;
}
