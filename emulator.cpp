// Christopher Fischerr
// 03FEB2022
// Emulator Specific Information

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>


#include "mmu.cpp"





#define BIT16 0x8000
#define BIT32 0x80000000
#define BIT64 0x8000000000000000
#define BIT(n) (uint64_t) 1 << (n-1)
#define LOWERMASK(n) ((uint64_t) 1 << (n)) - 1

const short int MIP_ISA_32 = 1;

const short int MIPSI = 1;
/*
LB Load Byte
LBU Load Byte Unsigned
SB Store Byte
LH Load Halfword
LHU Load Halfword Unsigned
SH Store Halfword
LWL Load Word Left
LWR Load Word Right
SWL Store Word Left
SWR Store Word Right
LWCz Load Word to Coprocessor-z (OUT OF SCOPE FOR NOW)
SWCz Store Word from Coprocessor-z (OUT OF SCOPE FOR NOW)
ADDI Addmmediate Word
ADDIU Addmmediate Unsigned Word
SLTI Set on Less Thanmmediate
SLTIU Set on Less Thanmmediate Unsigned
ANDI Andmmediate
ORI Ormmediate
XORI Exclusive Ormmediate
LUI Load Uppermmediate
ADD Add Word
ADDU Add Unsigned Word
SUB Subtract Word
SUBU Subtract Unsigned Word
SLT Set on Less Than
SLTU Set on Less Than Unsigned
AND And
OR Or
XOR Exclusive Or
NOR Nor

-- Finished at A10

*/
const short int MIPSII = 2;
const short int MIPSIII = 3;
const short int MIPSIV = 4;

const short int RTYPE = 1;
const short int ITYPE = 2;
const short int JTYPE = 3;


// Exception Types
const short int IntegerOverflow = 1;
const short int MemoryFault = 2;

class EmulatedCPU
{
	public:
		// The Program Counter value. During the instruction time of an instruction this is the address of the
		// instruction word. The address of the instruction that occurs during the next instruction time 
		// is determined by assigning a value to PC during an instruction time. If no value is assigned to PC
		// during instruction time by any pseudocode statement, it is automatically incremented by four.
		// Read more info when working with this.
		uint64_t pc;
		bool is64bit = false;
		BinaryView* bv;
		// REGISTERS

		// Hacky solution I do not understand which allows us to have a function table.
		typedef void (EmulatedCPU::* const funct)(uint32_t);
		
		// A list of functions that are rtypes index by their ALU code.
		// Indexed by their ALU code.
		const EmulatedCPU::funct inst_handlers_rtypes[64] = {
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
			& EmulatedCPU::andop, // 36
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
		const EmulatedCPU::funct inst_handlers_otypes[64] = {
			& EmulatedCPU::unimplemented, // 0
			& EmulatedCPU::unimplemented, // 1
			& EmulatedCPU::unimplemented, // 2
			& EmulatedCPU::unimplemented, // 3
			& EmulatedCPU::beq, // 4
			& EmulatedCPU::unimplemented, // 5
			& EmulatedCPU::unimplemented, // 6
			& EmulatedCPU::unimplemented, // 7
			& EmulatedCPU::addi, // 8
			& EmulatedCPU::addiu, // 9
			& EmulatedCPU::unimplemented, // 10
			& EmulatedCPU::unimplemented, // 11
			& EmulatedCPU::andi, // 12
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

		const EmulatedCPU::funct inst_handlers_regimm[32] = {
			&EmulatedCPU::bltz, // 0
			&EmulatedCPU::bgez, // 1
			&EmulatedCPU::bltzl, // 2
			&EmulatedCPU::bgezl, // 3
			&EmulatedCPU::unimplemented, // 4
			&EmulatedCPU::unimplemented, // 5
			&EmulatedCPU::unimplemented, // 6
			&EmulatedCPU::unimplemented, // 7
			&EmulatedCPU::tgei, // 8
			&EmulatedCPU::tgeiu, // 9
			&EmulatedCPU::tlti, // 10
			&EmulatedCPU::tltiu, // 11
			&EmulatedCPU::teqi, // 12
			&EmulatedCPU::unimplemented, // 13
			&EmulatedCPU::tnei, // 14
			&EmulatedCPU::unimplemented, // 15
			&EmulatedCPU::bltzal, // 16
			&EmulatedCPU::bgezal, // 17
			&EmulatedCPU::bltzall, // 18
			&EmulatedCPU::bgezall, // 19
			&EmulatedCPU::unimplemented, // 20
			&EmulatedCPU::unimplemented, // 21
			&EmulatedCPU::unimplemented, // 22
			&EmulatedCPU::unimplemented, // 23
			&EmulatedCPU::unimplemented, // 24
			&EmulatedCPU::unimplemented, // 25
			&EmulatedCPU::unimplemented, // 26
			&EmulatedCPU::unimplemented, // 27
			&EmulatedCPU::unimplemented, // 28
			&EmulatedCPU::unimplemented, // 29
			&EmulatedCPU::unimplemented, // 30
			&EmulatedCPU::unimplemented, // 31
		};

		//registers and instruction fields
		uint64_t gpr[32];
		uint8_t rs; // 1st Source
		uint8_t rt; // 2nd Source
		uint8_t rd; // Register
		uint8_t sa; // Shift Amount
		uint64_t LO, HI; // Multiplication and division registers
		uint16_t immediate; // Immediate
		int16_t signedImmediate; // Immediate

		//Meta
		MMU *memUnit = NULL;
		bool instructionNullify = false;
		bool validState = true;
		bool delaySlot = false;
		char* instructions;
		int32_t tgt_offset = 0;

		int32_t mipsTarget = 1;
		bool debugPrint = true;
		


		EmulatedCPU(bool is64bit, BinaryView* bc)
		{
			bv = bc;

			memUnit = &MMU(is64bit, bc);

			int i;
			pc = 0;
			for (i = 0; i < 32; i++)
			{
				gpr[i] = 0;	
			}
					
			//memUnit = &MMU(is64bit, bv);
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

		uint32_t debugGetValue(int address, int retVal)
		{	
			char* bytes = memUnit->getBytes(address);
				
			retVal += (bytes[0] << 24);
			retVal += (bytes[1] << 16);
			retVal += (bytes[2] << 8);
			retVal += (bytes[3]);

			printf("%x ", bytes[0]);
			printf("%x ", bytes[1]);
			printf("%x ", bytes[2]);
			printf("%x\n", bytes[3]);

			
			return retVal;
		}

		void signalException(int excpt)
		{
			printf("Exception occured! [%d]\n", excpt);
			while (1)
			{
				// do nothing
			}
		}

		// TODO: Rose and Sean
		// For this, instruction, we'll take the PC and turn it into a memory address and then get the opcode from binary ninja.
		// and return it. 
		uint32_t getInstruction(int PC) {
			// Make PC a memory address (THIS SHOULD EVENTUALLY BE A CONVERSION FROM PC int TO AN ADDRESS OF MEMORY)
			int address = PC;
			uint32_t retVal = 0;
			
			// Get opcode from binja using address
			size_t numBytesRead;
			unsigned char* bytes = (unsigned char*) malloc(sizeof(char) * 4);
			numBytesRead = bv->Read(bytes, address, 4);
			
			// Convert to uint32_t
			retVal += (bytes[0] << 24);
			retVal += (bytes[1] << 16);
			retVal += (bytes[2] << 8);
			retVal += (bytes[3]);

			// Free malloc call
			free(bytes);

			return retVal;
		}

		// Need help resolving the warnings Im getting and ultimately not messing up other code
		uint32_t getNextInstruction() {
			return getInstruction(pc + 4);
		}

		void unimplemented(uint32_t opcode)
		{
			printf("\n\nPLEASE HELP ME YOU CALLED AN UNIMPLEMEND HANDLER\n\n");
			printf("TIME TO DIE");
			generallyPause();
			return;
		}

		// Takes in a program counter that is the entry point.
		// Unused.
		void runEmulation(int entryPoint)
		{
			pc = entryPoint;
			// Get the first instruction, execute it, increment by 1, and so forth.
			// Implement memory checks every instruction.

			while (validState == true)
			{
				printf("current pc: 0x%lx\n", pc);

				if (!instructionNullify)
				{
					uint32_t instruction = getInstruction(pc);
					runInstruction(instruction);
				}
				else
					instructionNullify = false;

				if (delaySlot)
				{
					delaySlot = false;
					pc += 4;
				}
				else if (tgt_offset != 0)
				{
					pc += tgt_offset;
					tgt_offset = 0;
				}
				else
					pc += 4;
			}
			

			return;
		}

		// This is the ADD function. Opcode of 0b000000 and ALU code of 0b100 000
		// TODO: Test with negative values.
		void add(uint32_t opcode)
		{

			if(mipsTarget < 1)
			{
				printf("Invalid mips target for ADD\n");
			}

			if(debugPrint)
			{
				printf("ADD %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] + gpr[rt];
			uint64_t flag = BIT32;

			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rd] = temp;

		}
		void addi(uint32_t opcode)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDI\n");
			}
			if (debugPrint)
			{
				printf("ADDI %s, %s, %d\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
			}
			
			//this->signExtend(&immediate, 16, 32);

			uint64_t temp = gpr[rs] + signedImmediate;

			// Check for an overflow
			uint64_t flag = is64bit ? BIT64 : BIT32;
			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rt] = temp;

		}
		void addiu(uint32_t opcode)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDIU\n");
			}

			if (debugPrint)
			{
				printf("ADDIU %s, %s, %dx\n", getName(rs).c_str(), getName(rt).c_str(), immediate);
			}

			//this->signExtend(&immediate, 16, 32);
			uint64_t temp = gpr[rs] + immediate;
			if (!is64bit)
				temp &= 0xffffffff;
			gpr[rt] = temp;

		}
		void addu(uint32_t opcode)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ADDU\n");
			}

			if (debugPrint)
			{
				printf("ADDU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] + gpr[rt];
			if (!is64bit)
				temp &= 0xffffffff;
			gpr[rd] = temp;
		}
		void andop(uint32_t opcode)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for AND\n");
			}

			if (debugPrint)
			{
				printf("AND %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			gpr[rd] = gpr[rs] & gpr[rt];
		}
		void andi(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ANDI\n");
			}

			if (debugPrint)
			{
				printf("ADDIU %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), signedImmediate);
			}
			gpr[rt] = gpr[rs] & immediate;
		}
		
		void beq(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BEQ\n");
			}

			if (debugPrint)
			{
				printf("BEQ %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
			}
			// Control branches are going to take a model of instruction memory first.
			// Rose, I removed your "kekwuw". That is not appropriate. I'll be docking your pay!
			// Chris, that's illegal. I'm reporting you to the WHD for wage theft.
			// Also you don't pay me.

			//cast to 32 bits  for auto sigm extend and space fpr shifting
			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;
			
			if (gpr[rs] == gpr[rt])
			{
				// If the two registers equal, we set the pc to increment after an instruction.
				tgt_offset = extendedImmediate;
			}
		}
		void beql(uint32_t instruction)
		{
			// For this, simply execute the next instruction with the EmulatedCPU and the target address.
			// Then we do the branch.

			// todo get instruction wrapper.
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for BEQL\n");
			}

			if (debugPrint)
			{
				printf("BEQL %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;


			
			if (gpr[rs] == gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}


		}
		void bgez(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BEQ\n");
			}

			if (debugPrint)
			{
				printf("BGEZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}
			
			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			runInstruction(getNextInstruction());
			if (gpr[rs] >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgezal(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BGEZAL\n");
			}

			if (debugPrint)
			{
				printf("BGEZAL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			// Set return address equal to the value.
			gpr[31] = pc + 8;

			if (gpr[rs] >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgezall (uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BGEZALL\n");
			}

			if (debugPrint)
			{
				printf("BGEZALL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			

			// Set return address equal to the value.
			gpr[31] = pc + 8;

			if (gpr[rs] >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		void bgezl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for bgezl\n");
			}

			if (debugPrint)
			{
				printf("bgezl %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;
			
			if (gpr[rs] >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}

		}
		void bgtz(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BGTZ\n");
			}

			if (debugPrint)
			{
				printf("BGTZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			if (gpr[rs] >= gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgtzl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for BGTZL\n");
			}

			if (debugPrint)
			{
				printf("BGTZL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;
			
			if (gpr[rs] >= gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		void blez(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for BLEZ\n");
			}

			if (debugPrint)
			{
				printf("BLEZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			if (gpr[rs] <= gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void blezl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for BLEZL\n");
			}

			if (debugPrint)
			{
				printf("BLEZL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			if (gpr[rs] <= gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				runInstruction(getNextInstruction());
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		void bltz(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for bltz\n");
			}

			if (debugPrint)
			{
				printf("bltz %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			if (gpr[rs] < gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bltzal(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for bltzal\n");
			}

			if (debugPrint)
			{
				printf("bltzal %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			gpr[31] = pc + 8;
			if (gpr[rs] < gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bltzall(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for bltzal\n");
			}

			if (debugPrint)
			{
				printf("bltzal %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			gpr[31] = pc + 8;
			if (gpr[rs] < gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		void bltzl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for bltzl\n");
			}

			if (debugPrint)
			{
				printf("bltzl %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			if (gpr[rs] < gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		void bne(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for bne\n");
			}

			if (debugPrint)
			{
				printf("bne %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;
			
			if (gpr[rs] != gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bnel(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for bne\n");
			}

			if (debugPrint)
			{
				printf("bne %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
			}
			
			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;
			
			if (gpr[rs] != gpr[rt])
			{
				// If the two registers equal, we increment PC by the offset.
				delaySlot = true;
				tgt_offset = extendedImmediate;
			}
			else
			{
				instructionNullify = true;
			}
		}
		// break_ because break is a C++ reserved word
		void break_(uint32_t instruction)
		{
			printf("Called Break!\n");
			unimplemented(instruction);
		}
		// Co Processor Operation (should be unimplemented I think)
		void copz(uint32_t instruction)
		{
			printf("Called COPz!\n");
			unimplemented(instruction);
		}
		// MIPS III
		// Assigned to Rose.
		void dadd(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DADD\n");
			}

			if (debugPrint)
			{
				printf("DADD %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] + gpr[rt];
			uint64_t flag = BIT64;

			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rd] = temp;
			return;
		}

		// MIPS III
		void daddi(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DADDI\n");
			}
			if (debugPrint)
			{
				printf("DADDI %s, %s, %d\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
			}

			//this->signExtend(&immediate, 16, 32);

			uint64_t temp = gpr[rs] + signedImmediate;

			// Check for an overflow
			uint64_t flag = BIT64;
			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rt] = temp;
		}

		//MIPS III
		void daddiu(uint32_t opcode)
		{

			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DADDIU\n");
			}

			if (debugPrint)
			{
				printf("DADDIU %s, %s, %dx\n", getName(rs).c_str(), getName(rt).c_str(), immediate);
			}

			//this->signExtend(&immediate, 16, 32);
			uint64_t temp = gpr[rs] + immediate;
			gpr[rt] = temp;

		}

		//MIPS III
		void daddu(uint32_t opcode)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DADDU\n");
			}

			if (debugPrint)
			{
				printf("DADDU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] + gpr[rt];

			gpr[rd] = temp;
		}

		//MIPS III
		void ddiv(uint32_t instruction)
		{
			
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DDIV\n");
			}
			if (debugPrint)
			{
				printf("DDIV %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if (rt == 0)
			{
				return;
			}

			LO = (int64_t) gpr[rs] / (int64_t)gpr[rt];
			HI = rs % rt;
			return;
		}

		//MIPS III
		void ddivu(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DDIVU\n");
			}
			if (debugPrint)
			{
				printf("DDIVU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if (rt == 0)
			{
				return;
			}

			LO = gpr[rs] / gpr[rt];
			HI = gpr[rs] % gpr[rt];
			return;
		}


		// MIPS 1
		void div(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for DIV\n");
			}
			if (debugPrint)
			{
				printf("DIV %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if (rt == 0)
			{
				return;
			}
			int64_t q = ((int64_t)gpr[rs] / (int64_t)gpr[rt]);
			int64_t r = gpr[rs] % gpr[rt];
			if (is64bit)
			{
				if((q & BIT32) > 0)
					q |= (((uint64_t)0xffffffff) << 32);
				if((r & BIT32) > 0)
					r |= (((uint64_t)0xffffffff) << 32);
			}
			LO = q;
			HI = r;
			return;
		}

		//MIPS I
		void divu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for DIVU\n");
			}
			if (debugPrint)
			{
				printf("DIVU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if (rt == 0)
			{
				return;
			}
			uint64_t q = gpr[rs] / gpr[rt];
			uint64_t r = gpr[rs] % gpr[rt];
			if (is64bit)
			{
				if ((q & BIT32) > 0)
					q |= (((uint64_t)0xffffffff) << 32);
				if ((r & BIT32) > 0)
					r |= (((uint64_t)0xffffffff) << 32);
			}

			LO = q;
			HI = r;
			return;
		}
		// MIPS III
		void dmult(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DMULT\n");
			}
			if (debugPrint)
			{
				printf("DMULT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}
			int64_t M = gpr[rs];
			uint64_t Q = gpr[rt];
			int count = 64;
			int64_t A = 0;
			bool Qinv = 0;
			uint64_t ptr = 0;
			while (count != 0)
			{
				printf("A: %.16lx, Q: %.16lx\n", A, Q);
				if ((Q & 1) && !Qinv)
				{
					A = A - M;
				}
				else if (Qinv && !(Q & 1))
				{
					A = A + M;
				}
				Qinv = Q & 1;
				Q >>= 1;

				ptr = A & 1;
				ptr <<= 63;
				Q |= ptr;
				printf("A:%lx\n", A);
				A >>= 1;
				printf("A:%lx\n", A);
				count--;
			}
			HI = A;
			LO = Q;
		}

		//MIPS III
		void dmultu(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DMULT\n");
			}
			if (debugPrint)
			{
				printf("DMULT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}
			int64_t M = gpr[rs];
			uint64_t Q = gpr[rt];
			int count = 64;
			int64_t A = 0;
			uint64_t ptr = 0;
			HI = 0;
			LO = 0;

			while (count > 0)
			{
				if (Q & 1)
				{
					HI += M;
				}
				Q >>= 1;
				LO >>= 1;
				ptr = (HI & 1) << 63;
				LO |= ptr;
				HI >>= 1;
			}
		}

		//MIPS III
		void dsll(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSLL\n");
			}
			if (debugPrint)
			{
				printf("DSLL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}
			
			gpr[rd] = gpr[rt] << sa;
		}

		//MIPS III
		void dsll32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSLL32\n");
			}
			if (debugPrint)
			{
				printf("DSLL32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] << (sa + 32);
		}

		//MIPS III
		void dsllv(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSLLV\n");
			}
			if (debugPrint)
			{
				printf("DSLLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] << (gpr[rs] & 0x3f);
		}
		
		//MIPS III
		void dsra(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRA\n");
			}
			if (debugPrint)
			{
				printf("DSRA %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t) (hold >> sa);
		}

		//MIPS III
		void dsra32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRA32\n");
			}
			if (debugPrint)
			{
				printf("DSRA32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t)(hold >> (sa+32));
		}

		//MIPS III
		void dsrav(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRAV\n");
			}
			if (debugPrint)
			{
				printf("DSRA %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t)(hold >> (gpr[rs] & 0x3f));
		}

		//MIPS III
		void dsrl(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRL\n");
			}
			if (debugPrint)
			{
				printf("DSRL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] >> sa;
		}

		//MIPS III
		void dsrl32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRL32\n");
			}
			if (debugPrint)
			{
				printf("DSRL32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] >> (sa+32);
		}

		//MIPS III
		void dsrlv(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSRLV\n");
			}
			if (debugPrint)
			{
				printf("DSRLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] >> (gpr[rs] & 0x3f);
		}

		//MIPS III
		void dsub(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSUB\n");
			}

			if (debugPrint)
			{
				printf("DSUB %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] - gpr[rt];
			uint64_t flag = BIT64;

			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rd] = temp;
			return;
		}

		//MIPS III
		void dsubu(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printf("Invalid mips target for DSUBU\n");
			}

			if (debugPrint)
			{
				printf("DSUBU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = gpr[rs] - gpr[rt];
		}
		// Jumps
		//MIPS I
		void j(uint32_t instruction)
		{
			uint64_t instr_index = (instruction & 0x3fffff) << 2;

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for J\n");
			}

			if (debugPrint)
			{
				printf("J %lx\n", instr_index);
			}

			runInstruction(getNextInstruction());
			
			uint64_t mask = is64bit ? 0xfffffffff0000000 : 0xf0000000;

			pc = (pc & mask) | (instr_index);
		}

		//MIPS I
		void jal(uint32_t instruction)
		{

			uint64_t instr_index = (instruction & 0x3fffff) << 2;

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for JAL\n");
			}

			if (debugPrint)
			{
				printf("JAL %lx\n", instr_index);
			}

			runInstruction(getNextInstruction());

			gpr[31] = pc + 8;

			uint64_t mask = is64bit ? 0xfffffffff0000000 : 0xf0000000;

			pc = (pc & mask) | (instr_index);
		}

		//MIPS I
		void jalr(uint32_t instruction)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for JALR\n");
			}

			if (debugPrint)
			{
				
				printf("JALR %s", getName(rd).c_str());
				if (rd != 31)
					printf(", %s", getName(rs).c_str());
				printf("\n");
			}
			uint64_t temp = gpr[rs];
			gpr[rd] = pc + 8;

			runInstruction(getNextInstruction());

			pc = temp;
		}

		//MIPS I
		void jr(uint32_t instruction)
		{

			if (mipsTarget < 1)
			{
				printf("Invalid mips target for JR\n");
			}

			if (debugPrint)
			{

				printf("JR %s\n", getName(rs).c_str());
			}
			
			uint64_t temp = gpr[rs];

			runInstruction(getNextInstruction());

			pc = temp;
		}
		void lb(uint32_t instruction)
		{

		}
		void lbu(uint32_t instruction)
		{

		}
		// MIPS 3
		void LD(uint32_t instruction)
		{

		}
		// MIPS 2, likely going to be unimplemented
		void LDCz(uint32_t instruction)
		{

		}
		// MIPS 3
		void LDL(uint32_t instruction)
		{

		}
		void LDR(uint32_t instruction)
		{

		}
		// MIPS 1
		void lh(uint32_t instruction)
		{

		}
		void lhu(uint32_t instruction)
		{

		}
		// MIPS 2
		void LL(uint32_t instruction)
		{

		}
		// MIPS 3
		void LLD(uint32_t instruction)
		{

		}
		// MIPS 1
		void lui(uint32_t instruction)
		{

		}

		//MIPS I
		void lw(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LW\n");
			}

			if (debugPrint)
			{
				printf("LW %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}

			if (mipsTarget == 4)
			{
				if (immediate & 3 > 0)
					1 + 2;
			}
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;

			}

		}
		// Likely to be unimplemented
		void lwcz(uint32_t instruction)
		{

		}
		void lwl(uint32_t instruction)
		{

		}
		void lwr(uint32_t instruction)
		{

		}
		// MIPS 3
		void LWU(uint32_t instruction)
		{

		}

		// MIPS 1
		void mfhi(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MFHI\n");
			}

			if (debugPrint)
			{

				printf("MFHI %s", getName(rd).c_str());
			}

			gpr[rd] = HI;
		}
		void mflo(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MFLO\n");
			}

			if (debugPrint)
			{

				printf("MFLO %s", getName(rd).c_str());
			}

			gpr[rd] = LO;
		}
		// MIPS 4
		void MOVN(uint32_t instruction)
		{

			if (mipsTarget < 4)
			{
				printf("Invalid mips target for MOVN\n");
			}

			if (debugPrint)
			{

				printf("MOVN %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			if (gpr[rt] != 0)
			{
				gpr[rd] = gpr[rs];
			}
		}

		//MIPS IV
		void MOVZ(uint32_t instruction)
		{
			if (mipsTarget < 4)
			{
				printf("Invalid mips target for MOVZ\n");
			}

			if (debugPrint)
			{

				printf("MOVZ %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			if (gpr[rt] == 0)
			{
				gpr[rd] = gpr[rs];
			}
		}
		// MIPS 1
		void mthi(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MTHI\n");
			}

			if (debugPrint)
			{

				printf("MTHI %s", getName(rs).c_str());
			}

			HI = gpr[rs];
		}

		//MIPS I
		void mtlo(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MTLO\n");
			}

			if (debugPrint)
			{

				printf("MTLO %s", getName(rs).c_str());
			}

			LO = gpr[rs];
		}

		//MIPS I
		void mult(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MULT\n");
			}

			if (debugPrint)
			{

				printf("MULT %s, %s", getName(rs).c_str(), getName(rt).c_str());
			}

			int64_t a, b, result;

			a = gpr[rs];
			b = gpr[rt];
			if (is64bit)
			{
				//sign extensions
				a = (gpr[rs] & BIT32) ? gpr[rs] | 0xffffffff00000000 : gpr[rs] & 0xffffffff;
				b = (gpr[rt] & BIT32) ? gpr[rt] | 0xffffffff00000000 : gpr[rt] & 0xffffffff;

			}

			result = a * b;
			int32_t hold = (result & 0xffffffff);
			LO = hold;
			hold = (result & 0xffffffff00000000) >> 32;
			HI = hold;
		}

		//MIPS I
		void multu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for MULTU\n");
			}

			if (debugPrint)
			{

				printf("MULTU %s, %s", getName(rs).c_str(), getName(rt).c_str());
			}

			uint64_t a, b, result;

			//sign extensions
			a = gpr[rs] & 0xffffffff;
			b = gpr[rt] & 0xffffffff;

			//For some reason you still sign extend the result in lo and hi.
			result = a * b;
			int32_t hold = (result & 0xffffffff);
			LO = hold;
			hold = (result & 0xffffffff00000000) >> 32;
			HI = hold;

		}

		//MIPS I
		void nor(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for NOR\n");
			}

			if (debugPrint)
			{

				printf("NOR %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = ~(gpr[rs] | gpr[rt]);
		}

		//MIPS I
		void orop(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for OR\n");
			}

			if (debugPrint)
			{

				printf("OR %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = gpr[rs] | gpr[rt];
		}

		//MIPS I
		void ori(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for ORI\n");
			}

			if (debugPrint)
			{

				printf("ORI %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			uint64_t extended = (uint64_t)immediate;
			gpr[rt] = extended | gpr[rs];
		}
		// MIPS 4
		void PERF(uint32_t instruction)
		{

		}
		// MIPS 1
		void sb(uint32_t instruction)
		{

		}
		// MIPS 2
		void SC(uint32_t instruction)
		{

		}
		// MIPS 3
		void SCD(uint32_t instruction)
		{

		}
		void SD(uint32_t instruction)
		{

		}
		// MIPS 2
		void SDCz(uint32_t instruction)
		{

		}
		// MIPS 3
		void SDL(uint32_t instruction)
		{

		}
		void SDR(uint32_t instruction)
		{

		}
		// MIPS 1
		void sh(uint32_t instruction)
		{

		}

		//MIPS I
		void sll(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLL\n");
			}

			if (debugPrint)
			{

				printf("SLL %s, %s, %s", getName(rd).c_str(), getName(rt).c_str(), getName(sa).c_str());
			}

			gpr[rd] = gpr[rt] << sa;
		}

		//MIPS I
		void sllv(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLLV\n");
			}

			if (debugPrint)
			{

				printf("SLLV %s, %s, %s", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] << (gpr[rs] & 0x1f);
		}

		//MIPS I
		void slt(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLT\n");
			}

			if (debugPrint)
			{

				printf("SLT %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = (gpr[rs] < gpr[rt]);
		}

		//MIPS I
		void slti(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLTI\n");
			}

			if (debugPrint)
			{

				printf("SLTI %s, %s, %x", getName(rt).c_str(), getName(rs).c_str(), immediate);
			}

			int64_t extended = (int64_t) immediate;
			gpr[rd] = gpr[rs] < extended;
		}

		//MIPS I
		//@might not be how casting works.
		void sltui(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLTIU\n");
			}

			if (debugPrint)
			{

				printf("SLTIU %s, %s, %x", getName(rt).c_str(), getName(rs).c_str(), immediate);
			}
			if (is64bit)
			{
				int64_t extended = (int64_t)signedImmediate;
				uint64_t a = extended, 
						 b = gpr[rs];
				gpr[rt] = a < b;
			}
			else
			{
				int32_t extended = (int32_t)signedImmediate;
				uint64_t a = extended & 0xffffffff,
						 b = gpr[rs];
				gpr[rt] = a < b;
			}
			
		}

		//MIPS I
		void sltu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SLUT\n");
			}

			if (debugPrint)
			{

				printf("SLUT %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			uint64_t a = gpr[rs], b = gpr[rt];
			gpr[rd] = a < b;
		}
		void sra(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SRA\n");
			}

			if (debugPrint)
			{

				printf("SRA %s, %s, %x", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			int32_t hold = gpr[rt] & 0xffffffff;
			hold >>= sa;
			if (is64bit)
				gpr[rd] = (int64_t)hold;
			else
			{
				gpr[rd] = hold;
				gpr[rd] &= 0xffffffff;
			}
		}

		//MIPS I
		void srav(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SRAV\n");
			}

			if (debugPrint)
			{

				printf("SRAV %s, %s, %s", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			int32_t hold = gpr[rt] & 0xffffffff;
			hold >>= (gpr[rs] & 0x1f);
			if (is64bit)
				gpr[rd] = (int64_t)hold;
			else
			{
				gpr[rd] = hold;
				gpr[rd] &= 0xffffffff;
			}
		}
		void srl(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SRL\n");
			}

			if (debugPrint)
			{

				printf("SRL %s, %s, %d", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			uint32_t hold = gpr[rt];
			hold >>= sa;
			gpr[rd] = hold;
		}
		void srlv(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SRLV\n");
			}

			if (debugPrint)
			{

				printf("SRLV %s, %s, %s", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			uint32_t hold = gpr[rt];
			hold >>= (gpr[rs] & 0x1f);
			gpr[rd] = hold;
		}

		//MIPS I
		void sub(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SUB\n");
			}

			if (debugPrint)
			{
				printf("SUB %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] - gpr[rt];
			uint64_t flag = BIT32;

			if (((flag & gpr[rs]) == (flag & gpr[rt])) && ((flag & temp) != (flag & gpr[rs])))
			{
				signalException(IntegerOverflow);
			}

			gpr[rd] = temp;
		}
		void subu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SUBU\n");
			}

			if (debugPrint)
			{
				printf("SUBU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] - gpr[rt];
			if (!is64bit)
				temp &= 0xffffffff;
			gpr[rd] = temp;
		}
		void sw(uint32_t instruction)
		{

		}
		void swcz(uint32_t instruction)
		{

		}
		void swl(uint32_t instruction)
		{

		}
		void swr(uint32_t instruction)
		{

		}
		// MIPS 2
		void SYNC(uint32_t instruction)
		{

		}
		// MIPS 1
		void syscall(uint32_t instruction)
		{

		}
		// MIPS 2
		void teq(uint32_t instruction)
		{

		}
		void teqi(uint32_t instruction)
		{

		}
		void tge(uint32_t instruction)
		{

		}
		void tgei(uint32_t instruction)
		{

		}
		void tgeiu(uint32_t instruction)
		{

		}
		void tgeu(uint32_t instruction)
		{

		}
		void tlt(uint32_t instruction)
		{

		}
		void tlti(uint32_t instruction)
		{

		}
		void tltiu(uint32_t instruction)
		{

		}
		void tltui(uint32_t instruction)
		{

		}
		void tltu(uint32_t instruction)
		{

		}
		void tne(uint32_t instruction)
		{

		}
		void tnei(uint32_t instruction)
		{

		}

		//MIPS 1
		void xorop(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for XOR\n");
			}

			if (debugPrint)
			{

				printf("XOR %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = gpr[rs] ^ gpr[rt];
		}

		//MIPS I
		void xori(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for XORI\n");
			}

			if (debugPrint)
			{

				printf("XORI %s, %s, %x", getName(rd).c_str(), getName(rs).c_str(), immediate);
			}

			gpr[rt] = gpr[rs] ^ immediate;
		}
		
		//@dontuse
		void signExtend(uint64_t* target, int length, int extension = -1)
		{

			uint64_t bitn = (uint64_t)1 << (length-1);
			uint64_t mask = ~(bitn - 1);
			printf("%lx\n", mask);
			if (extension != -1)
			{
				if (extension == 32)
				{
					mask &= 0xffffffff;
					*target |= *target & bitn ? mask : 0;
					return;
				}
				else if (extension == 64)
				{
					*target |= *target & bitn ? mask : 0;
					return;
				}
			}
			if (is64bit)
				*target |= *target & bitn ? mask : 0;
			else
			{
				mask &= 0xffffffff;
				*target |= *target & bitn ? mask : 0;
			}
				
			return;
		}


		void registerDump()
		{
			int i;

			printf("=================    CPU STATE:    =================\n");
			printf("PC: [0%lx]\n", pc);

			printf("=================DUMPING REGISTERS:=================\n");
			for (i = 0; i < 32; i++)
			{
				printf("%s -> %lx\n", getName(i).c_str(), gpr[i]);
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
			sa = (instruction & 0x7c0) >> 6;
			immediate = instruction & 0xffff;
			signedImmediate = immediate;

			if (is64bit == false)
			{
				int i;
				for (i = 0; i < 32; i++)
				{
					gpr[i] = gpr[i] & 0xFFFFFFFF;
				}
			}
			
			printf("immediate signed: %d immediate unsigned: %d\n", signedImmediate, immediate);
			
			// If the upper 26-31 bits are set to zero, then, we have an R-Type instruction
			if ((instruction & 0xfc000000) == 0)
			{
				// Essentially, this is a list of r type functions indexed by opcode.
				printf("Rtype [%d]\n", (instruction & 0b111111));
				(this->*inst_handlers_rtypes[(instruction & 0b111111)])(instruction);
							
			}
			// If the upper 26-31 bits are set to one, then, we have a REGIMM instruction
			else if ((instruction & 0xfc000000) == 1)
			{
				printf(" ");
			}
			else
			{
				printf("Otype [%d]\n", ((instruction & 0xfc000000) >> 26));
				(this->*inst_handlers_otypes[(instruction & 0xfc000000) >> 26])(instruction);
			}			
		}
};


#ifndef _WIN32
#include <libgen.h>
#include <dlfcn.h>
static string GetPluginsDirectory()
{
	Dl_info info;
	if (!dladdr((void *)BNGetBundledPluginDirectory, &info))
		return NULL;

	stringstream ss;
	ss << dirname((char *)info.dli_fname) << "/plugins/";
	return ss.str();
}
#else
static string GetPluginsDirectory()
{
	return "C:\\Program Files\\Vector35\\BinaryNinja\\plugins\\";
}
#endif

int main(int argn, char ** args)
{	
	// In order to initiate the bundled plugins properly, the location
	// of where bundled plugins directory is must be set. Since
	// libbinaryninjacore is in the path get the path to it and use it to
	// determine the plugins directory
	SetBundledPluginDirectory(GetPluginsDirectory());
	InitPlugins();
	printf("[INFO] Plugins initialized!\n");

	Ref<BinaryData> bd = new BinaryData(new FileMetadata(), args[1]);
	Ref<BinaryView> bv = NULL;
	for (auto type : BinaryViewType::GetViewTypes())
	{
		if (type->IsTypeValidForData(bd) && type->GetName() != "Raw")
		{
			bv = type->Create(bd);
			break;
		}
	}
	printf("[INFO] BVs initialized!\n");

	if (!bv || bv->GetTypeName() == "Raw")
	{
		fprintf(stderr, "Input file does not appear to be an exectuable\n");
		return -1;
	}
	printf("[INFO] Starting Analysis.\n");

	bv->UpdateAnalysisAndWait();
	
	printf("[INFO] Finished Analysis.\n");

	EmulatedCPU* electricrock = new EmulatedCPU(false, bv);
	
	// This should get us the value of something interesting.
	// Should give us 3c1c0043 in unsigned decimal
	printf("Lover of the russian queen. %lx\n", electricrock->debugGetValue(0x4010e0, 0));
	electricrock->runEmulation((uint32_t)bv->GetEntryPoint());

	// Proper shutdown of core
	BNShutdown();

	// Method for testing getInstruction();
	//uint32_t address = 0;
	//unsigned char* opcode = electricrock->getInstruction(address);
	//printf("opcode:%x %x %x %x",*(opcode+0),*(opcode+1),*(opcode+2),*(opcode+3));

	//MMU* mmu = new MMU(false, bv);

	/* 
	EmulatedCPU* electricrock = new EmulatedCPU(false, bv);
	printf("%d %s\n", 31, electricrock->getName(31).c_str());

	int32_t immediate = -4;
	uint64_t test = 10;
	//immediate  >>= 1;
	//test = 6
	printf("%lld", immediate);

	*/

	/*
	So generally, here's how this should work.
	We load in a program (probably the one fed in via arguments)
	We then map all of the things into memory using BN.
	Every iteration, we exec the opcode and incremenet the PC.
	*/

	/*

	// This is a valid opcode extracted from a program
	// memonic: and $v0, $v1, $v0
	//electricrock->runInstruction(0x00621024);
	
	electricrock->gpr[2] = 0xffffffffffffffff;
	electricrock->gpr[21] = 0x8000000000000000;
	electricrock->is64bit = true;

	//instruction test = assemble("add $v0 $s5 $v0");
	//printf("%.8x\n", test.asem);
	// memonic: addu $v0, $s5, $v0
	//electricrock->runInstruction(0x02a21021);

	//MMU memunit = MMU(PageTable(), (allocation *)NULL);
	//memunit.segment(0, 3);

	// memonic: addi r1, r1, -1
	electricrock->runInstruction(0x1041ffff);
	// addi r1, r1, -32,768
	electricrock->runInstruction(0x20217000);
	
	//electricrock->signExtend(&electricrock->signedImmediate, 32);



	electricrock->registerDump();

	// memonic: addiu $a1,$zero, 1
	electricrock->runInstruction(0x24050001);

	// memonic: add $v0, $s5, $v0
	electricrock->runInstruction(0x00551020);
	
	*/
	// Just a lazy way to stop things from progressing
	//char* str_space = (char*) malloc(sizeof(char) * 1024);
	//scanf("%s", str_space);
	
	
	
}
