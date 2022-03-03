// Christopher Fischerr
// 03FEB2022
// Emulator Specific Information

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>

#include "asem.cpp"

#define BIT16 0x8000
#define BIT32 0x80000000
#define BIT64 0x8000000000000000

const short int MIP_ISA_32 = 1;

const short int MIPSI = 1;
const short int MIPSII = 2;
const short int MIPSIII = 3;
const short int MIPSIV = 4;

const short int RTYPE = 1;
const short int ITYPE = 2;
const short int JTYPE = 3;


// Exception Types
const short int IntegerOverflow = 1;

class EmulatedCPU
{
	public:
		// The Program Counter value. During the instruction time of an instruction this is the address of the
		// instruction word. The address of the instruction that occurs during the next instruction time 
		// is determined by assigning a value to PC during an instruction time. If no value is assigned to PC
		// during instruction time by any pseudocode statement, it is automatically incremented by four.
		// Read more info when working with this.
		uint32_t pc;
		bool is64bit = false;
		// REGISTERS

		// Hacky solution I do not understand which allows us to have a function table.
		typedef void (EmulatedCPU::* const funct)(uint32_t);
		
		// A list of functions that are rtypes index by their ALU code.
		// Indexed by their ALU code.
		const EmulatedCPU::funct EmulatedCPU::inst_handlers_rtypes[64] = {
			& EmulatedCPU::unimplemented, // 0
			& EmulatedCPU::unimplemented, // 1
			& EmulatedCPU::unimplemented, // 2
			& EmulatedCPU::unimplemented, // 3
			& EmulatedCPU::unimplemented, // 4
			& EmulatedCPU::unimplemented, // 5
			& EmulatedCPU::unimplemented, // 6
			& EmulatedCPU::unimplemented, // 7
			& EmulatedCPU::unimplemented, // 8
			& EmulatedCPU::unimplemented, // 9
			& EmulatedCPU::unimplemented, // 10
			& EmulatedCPU::unimplemented, // 11
			& EmulatedCPU::unimplemented, // 12
			& EmulatedCPU::unimplemented, // 13
			& EmulatedCPU::unimplemented, // 14
			& EmulatedCPU::unimplemented, // 15
			& EmulatedCPU::unimplemented, // 16
			& EmulatedCPU::unimplemented, // 17
			& EmulatedCPU::unimplemented, // 18
			& EmulatedCPU::unimplemented, // 19
			& EmulatedCPU::unimplemented, // 20
			& EmulatedCPU::unimplemented, // 21
			& EmulatedCPU::unimplemented, // 22
			& EmulatedCPU::unimplemented, // 23
			& EmulatedCPU::unimplemented, // 24
			& EmulatedCPU::unimplemented, // 25
			& EmulatedCPU::unimplemented, // 26
			& EmulatedCPU::unimplemented, // 27
			& EmulatedCPU::unimplemented, // 28
			& EmulatedCPU::unimplemented, // 29
			& EmulatedCPU::unimplemented, // 30
			& EmulatedCPU::unimplemented, // 31
			& EmulatedCPU::add, // 32
			& EmulatedCPU::addu, // 33
			& EmulatedCPU::unimplemented, // 34
			& EmulatedCPU::unimplemented, // 35
			& EmulatedCPU::unimplemented, // 36
			& EmulatedCPU::unimplemented, // 37
			& EmulatedCPU::unimplemented, // 38
			& EmulatedCPU::unimplemented, // 39
			& EmulatedCPU::unimplemented, // 40
			& EmulatedCPU::unimplemented, // 41
			& EmulatedCPU::unimplemented, // 42
			& EmulatedCPU::unimplemented, // 43
			& EmulatedCPU::unimplemented, // 44
			& EmulatedCPU::unimplemented, // 45
			& EmulatedCPU::unimplemented, // 46
			& EmulatedCPU::unimplemented, // 47
			& EmulatedCPU::unimplemented, // 48
			& EmulatedCPU::unimplemented, // 49
			& EmulatedCPU::unimplemented, // 50
			& EmulatedCPU::unimplemented, // 51
			& EmulatedCPU::unimplemented, // 52
			& EmulatedCPU::unimplemented, // 53
			& EmulatedCPU::unimplemented, // 54
			& EmulatedCPU::unimplemented, // 55
			& EmulatedCPU::unimplemented, // 56
			& EmulatedCPU::unimplemented, // 57
			& EmulatedCPU::unimplemented, // 58
			& EmulatedCPU::unimplemented, // 59
			& EmulatedCPU::unimplemented, // 60
			& EmulatedCPU::unimplemented, // 61
			& EmulatedCPU::unimplemented, // 62
			& EmulatedCPU::unimplemented, // 63
		};

		// A list of functions that are other types then rtypes. 
		// Index by their opcode.
		const EmulatedCPU::funct EmulatedCPU::inst_handlers_otypes[64] = {
			& EmulatedCPU::unimplemented, // 0
			& EmulatedCPU::unimplemented, // 1
			& EmulatedCPU::unimplemented, // 2
			& EmulatedCPU::unimplemented, // 3
			& EmulatedCPU::unimplemented, // 4
			& EmulatedCPU::unimplemented, // 5
			& EmulatedCPU::unimplemented, // 6
			& EmulatedCPU::unimplemented, // 7
			& EmulatedCPU::addi, // 8
			& EmulatedCPU::addiu, // 9
			& EmulatedCPU::unimplemented, // 10
			& EmulatedCPU::unimplemented, // 11
			& EmulatedCPU::unimplemented, // 12
			& EmulatedCPU::unimplemented, // 13
			& EmulatedCPU::unimplemented, // 14
			& EmulatedCPU::unimplemented, // 15
			& EmulatedCPU::unimplemented, // 16
			& EmulatedCPU::unimplemented, // 17
			& EmulatedCPU::unimplemented, // 18
			& EmulatedCPU::unimplemented, // 19
			& EmulatedCPU::unimplemented, // 20
			& EmulatedCPU::unimplemented, // 21
			& EmulatedCPU::unimplemented, // 22
			& EmulatedCPU::unimplemented, // 23
			& EmulatedCPU::unimplemented, // 24
			& EmulatedCPU::unimplemented, // 25
			& EmulatedCPU::unimplemented, // 26
			& EmulatedCPU::unimplemented, // 27
			& EmulatedCPU::unimplemented, // 28
			& EmulatedCPU::unimplemented, // 29
			& EmulatedCPU::unimplemented, // 30
			& EmulatedCPU::unimplemented, // 31
			& EmulatedCPU::unimplemented, // 32
			& EmulatedCPU::unimplemented, // 33
			& EmulatedCPU::unimplemented, // 34
			& EmulatedCPU::unimplemented, // 35
			& EmulatedCPU::unimplemented, // 36
			& EmulatedCPU::unimplemented, // 37
			& EmulatedCPU::unimplemented, // 38
			& EmulatedCPU::unimplemented, // 39
			& EmulatedCPU::unimplemented, // 40
			& EmulatedCPU::unimplemented, // 41
			& EmulatedCPU::unimplemented, // 42
			& EmulatedCPU::unimplemented, // 43
			& EmulatedCPU::unimplemented, // 44
			& EmulatedCPU::unimplemented, // 45
			& EmulatedCPU::unimplemented, // 46
			& EmulatedCPU::unimplemented, // 47
			& EmulatedCPU::unimplemented, // 48
			& EmulatedCPU::unimplemented, // 49
			& EmulatedCPU::unimplemented, // 50
			& EmulatedCPU::unimplemented, // 51
			& EmulatedCPU::unimplemented, // 52
			& EmulatedCPU::unimplemented, // 53
			& EmulatedCPU::unimplemented, // 54
			& EmulatedCPU::unimplemented, // 55
			& EmulatedCPU::unimplemented, // 56
			& EmulatedCPU::unimplemented, // 57
			& EmulatedCPU::unimplemented, // 58
			& EmulatedCPU::unimplemented, // 59
			& EmulatedCPU::unimplemented, // 60
			& EmulatedCPU::unimplemented, // 61
			& EmulatedCPU::unimplemented, // 62
			& EmulatedCPU::unimplemented, // 63
		};

		uint64_t registers[32];
		uint8_t rs; // 1st Source
		uint8_t rt; // 2nd Source
		uint8_t rd; // Register
		uint64_t immediate; // Immediate
		uint64_t signedImmediate; // Immediate
		int32_t mipsTarget = 1;
		bool debugPrint = true;


		EmulatedCPU()
		{
			int i;
			pc = 0;
			for (i = 0; i < 32; i++)
				registers[i] = 0;		

		}

		// This function is a hacky way for us to freeze in the debug console which will be replaced
		// by a system call in the future.
		void generallyPause()
		{
			while (1)
			{
				// do nothing
			}
		}

		void signalException(int excpt)
		{
			printf("Exception occured! [%d]\n", excpt);
			while (1)
			{
				// do nothing
			}
		}

		void EmulatedCPU::unimplemented(uint32_t opcode)
		{
			printf("\n\nPLEASE HELP ME YOU CALLED AN UNIMPLEMEND HANDLER\n\n");
			printf("TIME TO DIE");
			generallyPause();
			return;
		}

		// This is the ADD function. Opcode of 0b000000 and ALU code of 0b100 000
		// TODO: Test with negative values.
		void EmulatedCPU::add(uint32_t opcode)
		{

			if(mipsTarget < 1)
			{
				printf("Invalid mips target for ADD\n");
			}

			if(debugPrint)
			{
				printf("ADD %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			





			int64_t temp = registers[rs] + registers[rt];

			

			uint64_t flag = is64bit ? BIT64 : BIT32;
			//printf("%lld, %lld, %lld\n", flag & registers[rs], flag & registers[rt], flag & temp);
			//printf("%lld, %lld, %lld\n", (flag & registers[rs]) == (flag & registers[rt]), (flag & temp) != (flag & registers[rs]), temp);
			//printf("%lld, %lld, %lld\n", registers[rs], registers[rt], temp);

			if (((flag & registers[rs]) == (flag & registers[rt])) && ((flag & temp) != (flag & registers[rs])))
			{
				signalException(IntegerOverflow);
			}

			registers[rd] = temp;

		}
		void EmulatedCPU::addi(uint32_t opcode)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDI\n");
			}

			

			if (debugPrint)
			{
				printf("ADDI %s, %s, %llx\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
			}
			

			int32_t temp = registers[rs] + signedImmediate;

			// Check for an overflow
			uint64_t flag = is64bit ? BIT64 : BIT32;
			if (((flag & registers[rs]) == (flag & registers[rt])) && ((flag & temp) != (flag & registers[rs])))
			{
				signalException(IntegerOverflow);
			}

			registers[rt] = temp;

		}
		void EmulatedCPU::addiu(uint32_t opcode)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDIU\n");
			}

			if (debugPrint)
			{
				printf("ADDIU %s, %s, %" PRIu64 "\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
			}

			int32_t temp = registers[rs] + signedImmediate;			
			registers[rt] = temp;

		}
		void EmulatedCPU::addu(uint32_t opcode)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDU\n");
			}

			if (debugPrint)
			{
				printf("ADDU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			registers[rd] = registers[rs] + registers[rt];
		}

		void signExtend(uint64_t* target, int length)
		{
			uint64_t bitn = (uint64_t)1 << (length-1);
			uint64_t mask = ~(bitn - 1);

			if (is64bit)
				*target |= *target & bitn ? mask : 0;
			else
			{
				mask &= 0xffffffff;
				*target |= *target & bitn ? mask : 0;
			}
				
			return;
		}


		void EmulatedCPU::registerDump()
		{
			int i;

			printf("=================    CPU STATE:    =================\n");
			printf("PC: [0%x]\n", pc);

			printf("=================DUMPING REGISTERS:=================\n");
			for (i = 0; i < 32; i++)
			{
				printf("%s -> %llx\n", getName(i).c_str(), registers[i]);
			}
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
		
		// pg. 40
		void runInstruction(uint32_t instruction)
		{
			// First, let's determine the instruction type.
			rs = (instruction & 0x3E00000) >> 21;
			rt = (instruction & 0x1F0000) >> 16;
			rd = (instruction & 0xf800) >> 11;
			immediate = instruction & 0xffff;
			signedImmediate = instruction & 0xffff;

			
			// If the upper 26-31 bits are set to zero, then, we have an R-Type instruction
			if ((instruction & 0xfc000000) == 0)
			{
				// Essentially, this is a list of r type functions indexed by opcode.
				printf("Rtype [%d]\n", (instruction & 0b111111));
				(this->*inst_handlers_rtypes[(instruction & 0b111111)])(instruction);
							
			}
			else
			{
				printf("Otype [%d]\n", ((instruction & 0xfc000000) >> 26));
				(this->*inst_handlers_otypes[(instruction & 0xfc000000) >> 26])(instruction);
			}			
			pc += 4;			
		}
};


int main(int argn, char ** args)
{

	EmulatedCPU* electricrock = new EmulatedCPU;
	printf("%d %s\n", 31, electricrock->getName(31).c_str());

	// This is a valid opcode extracted from a program
	// memonic: and $v0, $v1, $v0
	//electricrock->runInstruction(0x00621024);
	
	electricrock->registers[2] = 0xffffffffffffffff;
	electricrock->registers[21] = 0x8000000000000000;
	electricrock->is64bit = true;

	instruction test = assemble("add $v0 $s5 $v0");
	printf("%.8x\n", test.asem);
	// memonic: addu $v0, $s5, $v0
	//electricrock->runInstruction(0x02a21021);

	// memonic: addi r1, r1, -1
	electricrock->runInstruction(0x20217FFF);
	// addi r1, r1, -32,768
	electricrock->runInstruction(0x20217000);
	
	electricrock->signExtend(&electricrock->signedImmediate, 32);



	electricrock->registerDump();

	// memonic: addiu $a1,$zero, 1
	electricrock->runInstruction(0x24050001);

	// memonic: add $v0, $s5, $v0
	electricrock->runInstruction(0x00551020);
	
	// Just a lazy way to stop things from progressing
	char* str_space = (char*) malloc(sizeof(char) * 1024);
	scanf("%s", str_space);
	
	

}

