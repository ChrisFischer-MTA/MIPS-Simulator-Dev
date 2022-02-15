// Christopher Fischerr
// 03FEB2022
// Emulator Specific Information

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

const short int MIP_ISA_32 = 1;


const short int MIPSI = 1;
const short int MIPSII = 2;
const short int MIPSIII = 3;
const short int MIPSIV = 4;

const short int RTYPE = 1;
const short int ITYPE = 2;
const short int JTYPE = 3;

class instruction
{
public:
	short int targetVersion;
	short int targetISA;
	short int instructionType;
	void* funcPntr;
	char* memonic;
};


class EmulatedCPU
{
	public:
		// The Program Counter value. During the instruction time of an instruction this is the address of the
		// instruction word. The address of the instruction that occurs during the next instruction time 
		// is determined by assigning a value to PC during an instruction time. If no value is assigned to PC
		// during instruction time by any pseudocode statement, it is automatically incremented by four.
		// Read more info when working with this.
		uint32_t pc;
		// REGISTERS
		
		instruction* itypes;

		uint32_t registers[32];
		int32_t rs; // 1st Source
		int32_t rt; // 2nd Source
		int32_t rd; // Register
		int32_t immediate; // Immediate
		int32_t signedImmediate; // Immediate
		int32_t mipsTarget = 1;

		/*
		Where I'm stuck.
		I'd really like to make a function pointer table, indexing each of the operations by it's OPCODE.
		But I can't get C++ to let me declare this. 
		*/

		EmulatedCPU()
		{
			itypes = new instruction[64];
		}

		std::string getName(int offset)
		{
			if ((offset ==	0))	return "zero";
			if (offset ==	1)	return "at";
			if (offset ==	2)	return "v0";
			if (offset ==	3)	return "v1";
			if (offset ==	4)	return "a0";
			if (offset ==	5)	return "a1";
			if (offset ==	6)	return "a2";
			if (offset ==	7)	return "a3";
			if (offset ==	8)	return "t0";
			if (offset ==	9)	return "t1";
			if (offset ==	10)	return "t2";
			if (offset ==	11)	return "t3";
			if (offset ==	12)	return "t4";
			if (offset ==	13)	return "t5";
			if (offset ==	14)	return "t6";
			if (offset ==	15)	return "t7";
			if (offset ==	16)	return "s0";
			if (offset ==	17)	return "s1";
			if (offset ==	18)	return "s2";
			if (offset ==	19)	return "s3";
			if (offset ==	20)	return "s4";
			if (offset ==	21)	return "s5";
			if (offset ==	22)	return "s6";
			if (offset ==	23)	return "s7";
			if (offset ==	24)	return "t8";
			if (offset ==	25)	return "t9";
			if (offset ==	26)	return "k0";
			if (offset ==	27)	return "k1";
			if (offset ==	28)	return "gp";
			if (offset ==	29)	return "sp";
			if (offset ==	30)	return "fp";
			if (offset ==	31)	return "ra";
			return "NULL";
		}
		
		void addiu(uint32_t rs, uint32_t rt, uint32_t immediate)
		{
			

		}

		// pg. 40
		void runInstruction(uint32_t opcode)
		{
			// First, let's determine the instruction type.

			// If the upper 26-31 bits are set to zero, then, we have an R-Type instruction
			if ((opcode & 0xfc000000) == 0)
			{
				rs = (opcode & 0x3E00000) >> 21;
				rt = (opcode & 0x1F0000) >> 16;
				rd = (opcode & 0xf800) >> 11;

				
			}
			




			
			immediate = opcode & 0xffff;
				
			
			pc += 4;			
		}
};


int main(int argn, char ** args)
{

	EmulatedCPU* electricrock = new EmulatedCPU;
	printf("%d %s\n", 31, electricrock->getName(31).c_str());

	// This is a valid opcode extracted from a program
	// memonic: and $v0, $v1, $v0
	electricrock->runInstruction(0x00621024);
	
	// memonic: addu $v0, $s5, $v0
	electricrock->runInstruction(0x02a21021);

	// memonic: addiu $a1,$zero, 1
	electricrock->runInstruction(0x24050001);


	// Just a lazy way to stop things from progressing
	char* str_space = (char*) malloc(sizeof(char) * 1024);
	scanf("%s", str_space);
	
	

}

