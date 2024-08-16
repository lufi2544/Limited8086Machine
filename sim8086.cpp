
// This is a simulation of a 8086 processor. We are going to use the assembly language for this.

#include <iostream>
#include <fstream>
#include <bitset>

enum class enum_processor_register
{
	NONE,
	OPCODE = 0b0000000,

};

struct processor_8086
{
	// It gives us a buffer with the assembly code decoded.
	// @param FileBuff is the buffer loaded from the encoded binary. 
	char* Decode(char* FileBuff)
	{
		if(!FileBuff)
		{
			return nullptr;	
		}		
		
	}
};


int main ()
{
	std::ifstream File("listing_0037_single_register_mov", std::ios::binary);

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

	char* Buffer = new char(FileSize);
	if(File.read(Buffer, FileSize))
	{
		std::cerr << "File read successfully" << std::endl;
	}
	else
	{
		std::cerr << "Error reading the file " << std::endl; 
	}


	File.close();

	for(int i = 0; i < FileSize; ++i)
	{
		const int Num = (int)Buffer[i];
		std::bitset<8> bitset(Num);
		std::cout << bitset << std::endl;
	}

	delete Buffer;

	std::cout << std::endl;

	return 0;
}
