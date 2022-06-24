// Christopher Fischer and Rose Newcomer
// 03FEB2022
// Emulator Specific Information

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>
#include <iomanip>
#include <signal.h>


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

// Debug oprtions
const short int SHUT_UP = 1;


// Exception Types
const short int IntegerOverflow = 1;
const short int MemoryFault = 2;
const short int TrapFault = 3;
const short int ReservedInstructionException = 4;

// Coverage Information
std::vector<uint32_t> basicBlocks;
std::vector<std::string> basicBlockNames;

// Function Information (for hooking)
std::vector<uint32_t> functionVirtualAddress;
// Array offset in our hooked functions table which dictates which function the emualator calls
std::vector<short int> functionVirtualFunction; 

const short int NUM_FUNCTIONS_HOOKED = 3;

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
			& EmulatedCPU::sll, // 0
			& EmulatedCPU::unimplemented, // 1
			& EmulatedCPU::srl, // 2
			& EmulatedCPU::sra, // 3
			& EmulatedCPU::sllv, // 4
			& EmulatedCPU::unimplemented, // 5
			& EmulatedCPU::srlv, // 6
			& EmulatedCPU::srav, // 7
			& EmulatedCPU::jr, // 8
			& EmulatedCPU::jalr, // 9
			& EmulatedCPU::MOVZ, // 10
			& EmulatedCPU::MOVN, // 11
			& EmulatedCPU::syscall, // 12
			& EmulatedCPU::break_, // 13
			& EmulatedCPU::unimplemented, // 14
			& EmulatedCPU::SYNC, // 15 SYNC
			& EmulatedCPU::mfhi, // 16
			& EmulatedCPU::mthi, // 17
			& EmulatedCPU::mflo, // 18
			& EmulatedCPU::mtlo, // 19
			& EmulatedCPU::dsllv, // 20
			& EmulatedCPU::unimplemented, // 21
			& EmulatedCPU::dsrlv, // 22
			& EmulatedCPU::dsrav, // 23
			& EmulatedCPU::mult, // 24
			& EmulatedCPU::multu, // 25
			& EmulatedCPU::div, // 26
			& EmulatedCPU::divu, // 27
			& EmulatedCPU::dmult, // 28
			& EmulatedCPU::dmultu, // 29
			& EmulatedCPU::ddiv, // 30
			& EmulatedCPU::ddivu, // 31
			& EmulatedCPU::add, // 32
			& EmulatedCPU::addu, // 33
			& EmulatedCPU::sub, // 34
			& EmulatedCPU::subu, // 35
			& EmulatedCPU::andop, // 36
			& EmulatedCPU::orop, // 37
			& EmulatedCPU::xorop, // 38
			& EmulatedCPU::nor, // 39
			& EmulatedCPU::unimplemented, // 40
			& EmulatedCPU::unimplemented, // 41
			& EmulatedCPU::slt, // 42
			& EmulatedCPU::sltu, // 43
			& EmulatedCPU::dadd, // 44
			& EmulatedCPU::daddu, // 45
			& EmulatedCPU::dsub, // 46
			& EmulatedCPU::dsubu, // 47
			& EmulatedCPU::tge, // 48
			& EmulatedCPU::tgeu, // 49
			& EmulatedCPU::tlt, // 50
			& EmulatedCPU::tltu, // 51
			& EmulatedCPU::teq, // 52
			& EmulatedCPU::unimplemented, // 53
			& EmulatedCPU::tne, // 54
			& EmulatedCPU::unimplemented, // 55
			& EmulatedCPU::dsll, // 56
			& EmulatedCPU::unimplemented, // 57
			& EmulatedCPU::dsrl, // 58
			& EmulatedCPU::dsra, // 59
			& EmulatedCPU::unimplemented, // 60
			& EmulatedCPU::dsll32, // 61
			& EmulatedCPU::dsrl32, // 62
			& EmulatedCPU::dsra32, // 63
		};

		// A list of functions that are other types then rtypes. 
		// Index by their opcode.
		const EmulatedCPU::funct inst_handlers_otypes[64] = {
			& EmulatedCPU::unimplemented, // 0
			& EmulatedCPU::unimplemented, // 1
			& EmulatedCPU::j, // 2
			& EmulatedCPU::jal, // 3
			& EmulatedCPU::beq, // 4
			& EmulatedCPU::bne, // 5
			& EmulatedCPU::blez, // 6
			& EmulatedCPU::bgtz, // 7
			& EmulatedCPU::addi, // 8
			& EmulatedCPU::addiu, // 9
			& EmulatedCPU::slti, // 10
			& EmulatedCPU::sltui, // 11
			& EmulatedCPU::andi, // 12
			& EmulatedCPU::ori, // 13
			& EmulatedCPU::xori, // 14
			& EmulatedCPU::lui, // 15
			& EmulatedCPU::unimplemented, // 16 Coprocessor 1
			& EmulatedCPU::unimplemented, // 17 Coprocessor 2
			& EmulatedCPU::unimplemented, // 18 Coprocessor 3
			& EmulatedCPU::unimplemented, // 19 Coprocessor 4
			& EmulatedCPU::beql, // 20
			& EmulatedCPU::bnel, // 21
			& EmulatedCPU::blezl, // 22
			& EmulatedCPU::bgtzl, // 23
			& EmulatedCPU::daddi, // 24
			& EmulatedCPU::daddiu, // 25
			& EmulatedCPU::LDL, // 26
			& EmulatedCPU::LDR, // 27
			& EmulatedCPU::mul, // 28 ? (skipped)
			& EmulatedCPU::unimplemented, // 29 ? (skipped)
			& EmulatedCPU::unimplemented, // 30 ? (skipped)
			& EmulatedCPU::rdhwr, // 31 ? (skipped)
			& EmulatedCPU::lb, // 32
			& EmulatedCPU::lh, // 33
			& EmulatedCPU::lwl, // 34
			& EmulatedCPU::lw, // 35
			& EmulatedCPU::lbu, // 36
			& EmulatedCPU::lhu, // 37
			& EmulatedCPU::lwr, // 38
			& EmulatedCPU::LWU, // 39
			& EmulatedCPU::sb, // 40
			& EmulatedCPU::sh, // 41
			& EmulatedCPU::swl, // 42
			& EmulatedCPU::sw, // 43
			& EmulatedCPU::SDL, // 44
			& EmulatedCPU::SDR, // 45
			& EmulatedCPU::swr, // 46
			& EmulatedCPU::unimplemented, // 47 ? (skipped)
			& EmulatedCPU::LL, // 48
			& EmulatedCPU::unimplemented, // 49 coprocessor 1
			& EmulatedCPU::unimplemented, // 50 coprocessor 2
			& EmulatedCPU::PREF, // 51
			& EmulatedCPU::LLD, // 52 
			& EmulatedCPU::unimplemented, // 53 coprocessor 1
			& EmulatedCPU::unimplemented, // 54 coprocessor 2
			& EmulatedCPU::LD, // 55
			& EmulatedCPU::SC, // 56
			& EmulatedCPU::unimplemented, // 57 coprocessor 1
			& EmulatedCPU::unimplemented, // 58 coprocessor 2
			& EmulatedCPU::unimplemented, // 59 coprocessor 3
			& EmulatedCPU::SCD, // 60
			& EmulatedCPU::unimplemented, // 61 Coprocessor 1
			& EmulatedCPU::unimplemented, // 62 Coprocessor 2
			& EmulatedCPU::SD, // 63
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

		const char *rtype_names[64] = {
			"sll", // 0
			"unimplemented", // 1
			"srl", // 2
			"sra", // 3
			"sllv", // 4
			"unimplemented", // 5
			"srlv", // 6
			"srav", // 7
			"jr", // 8
			"jalr", // 9
			"MOVZ", // 10
			"MOVN", // 11
			"syscall", // 12
			"break_", // 13
			"unimplemented", // 14
			"SYNC", // 15 SYNC
			"mfhi", // 16
			"mthi", // 17
			"mflo", // 18
			"mtlo", // 19
			"dsllv", // 20
			"unimplemented", // 21
			"dsrlv", // 22
			"dsrav", // 23
			"mult", // 24
			"multu", // 25
			"div", // 26
			"divu", // 27
			"dmult", // 28
			"dmultu", // 29
			"ddiv", // 30
			"ddivu", // 31
			"add", // 32
			"addu", // 33
			"sub", // 34
			"subu", // 35
			"and", // 36
			"or", // 37
			"xor", // 38
			"nor", // 39
			"unimplemented", // 40
			"unimplemented", // 41
			"slt", // 42
			"sltu", // 43
			"dadd", // 44
			"daddu", // 45
			"dsub", // 46
			"dsubu", // 47
			"tge", // 48
			"tgeu", // 49
			"tlt", // 50
			"tltu", // 51
			"teq", // 52
			"unimplemented", // 53
			"tne", // 54
			"unimplemented", // 55
			"dsll", // 56
			"unimplemented", // 57
			"dsrl", // 58
			"dsra", // 59
			"unimplemented", // 60
			"dsll32", // 61
			"dsrl32", // 62
			"dsra32", // 63
		};

		const char *otype_names[64] = {
			"unimplemented", // 0
			"unimplemented", // 1
			"j", // 2
			"jal", // 3
			"beq", // 4
			"bne", // 5
			"blez", // 6
			"bgtz", // 7
			"addi", // 8
			"addiu", // 9
			"slti", // 10
			"sltui", // 11
			"andi", // 12
			"ori", // 13
			"xori", // 14
			"lui", // 15
			"unimplemented", // 16 Coprocessor 1
			"unimplemented", // 17 Coprocessor 2
			"unimplemented", // 18 Coprocessor 3
			"unimplemented", // 19 Coprocessor 4
			"beql", // 20
			"bnel", // 21
			"blezl", // 22
			"bgtzl", // 23
			"daddi", // 24
			"daddiu", // 25
			"LDL", // 26
			"LDR", // 27
			"mul", // 28 ? (skipped)
			"unimplemented", // 29 ? (skipped)
			"unimplemented", // 30 ? (skipped)
			"rdhwr", // 31 ? (skipped)
			"lb", // 32
			"lh", // 33
			"lwl", // 34
			"lw", // 35
			"lbu", // 36
			"lhu", // 37
			"lwr", // 38
			"LWU", // 39
			"sb", // 40
			"sh", // 41
			"swl", // 42
			"sw", // 43
			"SDL", // 44
			"SDR", // 45
			"swr", // 46
			"unimplemented", // 47 ? (skipped)
			"LL", // 48
			"unimplemented", // 49 coprocessor 1
			"unimplemented", // 50 coprocessor 2
			"PREF", // 51
			"LLD", // 52 
			"unimplemented", // 53 coprocessor 1
			"unimplemented", // 54 coprocessor 2
			"LD", // 55
			"SC", // 56
			"unimplemented", // 57 coprocessor 1
			"unimplemented", // 58 coprocessor 2
			"unimplemented", // 59 coprocessor 3
			"SCD", // 60
			"unimplemented", // 61 Coprocessor 1
			"unimplemented", // 62 Coprocessor 2
			"SD", // 63
		};

		const char *regimm_names[32] = {
			"bltz", // 0
			"bgez", // 1
			"bltzl", // 2
			"bgezl", // 3
			"unimplemented", // 4
			"unimplemented", // 5
			"unimplemented", // 6
			"unimplemented", // 7
			"tgei", // 8
			"tgeiu", // 9
			"tlti", // 10
			"tltiu", // 11
			"teqi", // 12
			"unimplemented", // 13
			"tnei", // 14
			"unimplemented", // 15
			"bltzal", // 16
			"bgezal", // 17
			"bltzall", // 18
			"bgezall", // 19
			"unimplemented", // 20
			"unimplemented", // 21
			"unimplemented", // 22
			"unimplemented", // 23
			"unimplemented", // 24
			"unimplemented", // 25
			"unimplemented", // 26
			"unimplemented", // 27
			"unimplemented", // 28
			"unimplemented", // 29
			"unimplemented", // 30
			"unimplemented", // 31
		};


		// These are function hooks included with the emulator, used
		// for common libc functions which are problematic to fully
		// emulate.
		const EmulatedCPU::funct static_function_hooks[NUM_FUNCTIONS_HOOKED] = {
			&EmulatedCPU::hooked_libc_write,
			&EmulatedCPU::hooked_libc_malloc,	
			&EmulatedCPU::hooked_libc_free	
		};
		
		const std::string static_function_hook_matching[NUM_FUNCTIONS_HOOKED] = 
		{
			//"__WRITE",
			"__stdio_WRITE",
			"__libc_malloc",
			"free"
		};

		//registers and instruction fields
		uint64_t gpr[32];
		uint64_t hwr[32];
		uint8_t rs; // 1st Source
		uint8_t rt; // 2nd Source
		uint8_t rd; // Register
		uint8_t sa; // Shift Amount
		uint64_t LO, HI; // Multiplication and division registers
		uint16_t immediate; // Immediate
		int16_t signedImmediate; // Immediate
		vector<uint64_t> hookedFunctions;

		//Meta
		MMU *memUnit = NULL;
		bool instructionNullify = false;
		bool validState = true;
		bool delaySlot = false;
		char* instructions;
		int32_t tgt_offset = 0;
		int instructionsRun = 0;
		uint32_t endOfMain, startOfMain;

		int32_t mipsTarget = 32;
		bool debugPrint = true;
		


		EmulatedCPU(bool is64bit, BinaryView* bc)
		{
			bv = bc;
			//fflush(stdout);
			memUnit = new MMU(is64bit, bc);
			
			//fflush(stdout);
			int i;
			pc = 0;
			for (i = 0; i < 32; i++)
			{
				gpr[i] = 0;	
			}
			//Instantiates the stack pointer;
			gpr[29] = memUnit->stackBase - 28 - 396;

			uint32_t UserLocalPtr = memUnit->MMUHeap.allocMem(12) + 6;
			hwr[29] = UserLocalPtr;
			
			
			for(auto& func : bv ->GetAnalysisFunctionList())
			{
				for(auto& block : func->GetBasicBlocks())
				{
					// add to a vector
					basicBlocks.push_back(block->GetStart());
					basicBlockNames.push_back(func->GetSymbol()->GetFullName().c_str());
					if(strncmp(func->GetSymbol()->GetFullName().c_str(), "main", 4) == 0)
					{
						auto addRanges = func->GetAddressRanges();
						endOfMain = addRanges[0].end;
						startOfMain = addRanges[0].start;
						//printf("%x, %x", startOfMain, endOfMain);
					}
				}
			}
			
			// First, get a list of functions. 
			// Get the name, loop through our hooked functions linked
			for(auto& func : bv ->GetAnalysisFunctionList())
			{
				Ref<Symbol> sym = func->GetSymbol();
				if (sym)
				{
					std::string currentFunctionName = sym->GetFullName();
					for(i = 0; i < NUM_FUNCTIONS_HOOKED; i++)
					{
						if(currentFunctionName == static_function_hook_matching[i])
						{
							functionVirtualAddress.push_back(func->GetStart());
							functionVirtualFunction.push_back(i);
						}
					}
				}
			}
			
			
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
			unsigned char* bytes = memUnit->getBytes(address);
				
			retVal += ((bytes[0] & 0xff) << 24);
			retVal += ((bytes[1] & 0xff) << 16);
			retVal += ((bytes[2] & 0xff) << 8);
			retVal += ((bytes[3] & 0xff));

			printf("%x ", (bytes[0] & 0xff));
			printf("%x ", (bytes[1] & 0xff));
			printf("%x ", (bytes[2] & 0xff));
			printf("%x\n", (bytes[3] & 0xff));

			
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
			// Make PC a memory address
			int address = PC;
			uint32_t retVal = 0;
			
			
			if(!memUnit->segSearch(PC).executable)
			{
				signalException(MemoryFault);
			}
			
			// Get opcode from the mmu using address
			unsigned char* bytes = memUnit->getEffectiveAddress(PC, 4, 0, 0);
			if(bytes == NULL)
			{
				signalException(MemoryFault);
			}
			
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
			int index = 0;
			char *pweasenosteppy = (char *) calloc(1024, sizeof(char));
			// Get the first instruction, execute it, increment by 1, and so forth.
			// Implement memory checks every instruction.
			int next = 0, skip = 0;
			//While nothing has exploded,
			while (validState == true)
			{
				
				// Code to determine if we are able to find the current PC in a basic block.
				auto findIterator = std::find(basicBlocks.begin(), basicBlocks.end(), pc);
				
				if(findIterator != basicBlocks.end())
				{
					// Get the index
					index = findIterator-basicBlocks.begin();
					printf("Current PC:  0x%lx - Start of a Basic Block in: %s\n", pc, basicBlockNames[index].c_str());
				}
				else
				{
					printf("Current PC:  0x%lx - Last Found Basic Block in: %s\n", pc, basicBlockNames[index].c_str());
				}
				
				// Code to search iteratively through our hooked functions and find if PC is a hooked address.
				findIterator = std::find(functionVirtualAddress.begin(), functionVirtualAddress.end(), pc);
				if(findIterator != functionVirtualAddress.end())
				{
					printf("Found a hooked function, calling appropriate hooked implementation!\n");
					index = findIterator-functionVirtualAddress.begin();
					printf("Index: [%d] name of [%s] \n", index, static_function_hook_matching[functionVirtualFunction[index]].c_str());
					(this->*static_function_hooks[functionVirtualFunction[index]])(0x0);
					//registerDump();
					
					//while(true);
					// TODO: Manually set up the stack after intercepting a function call.
				}
				
				
				//
				
				
				//If the instruction is not nullified, fetch and run it.
				if (!instructionNullify)
				{
					uint32_t instruction = getInstruction(pc);
					runInstruction(instruction);
					instructionsRun++;
					printf("Instructions run: %x\n\n", instructionsRun);
				}
				//If the instruction is nullified, cancel the nullification
				else
					instructionNullify = false;

				//If this is the delay slot, remove the flag and do the pc
				if (delaySlot)
				{
					//The instruction that just executed in the function above is a branch.
					//So tgt_offset is loaded, but we refuse entry with delay slot
					//The while loop is going to cycle and execute the next scheduled instruction, and then enter tgt_offset
					delaySlot = false;
					pc += 4;
				}
				//If this is not the delay slot and there's a loaded offset, (It must be the next next instruction from a Branch)
				//execute the loaded offset
				else if (tgt_offset != 0)
				{
					pc += tgt_offset;
					tgt_offset = 0;
				}
				else
					pc += 4;
				
				//registerDump();
				int flags = 0;
				next = 0;
				
				while(next == 0 && skip <= 0 && (SHUT_UP < 1))
				{
					printf("\n>> ");
					scanf("%s", pweasenosteppy);
					if(strncmp(pweasenosteppy, "mem", 3) == 0)
					{
						int address, n;
						scanf("%x", &address);
						scanf("%x", &n);
						flags = scanCode(pweasenosteppy, address, n);
						
					}
					else
					{
						flags = scanCode(pweasenosteppy);
					}
					if(flags == 1)
						next = 1;
					if(flags > 1)
					{
						skip = flags;
					}
					
				}
				skip--;
				
			}
			

			return;
		}

		//goals: step, cancel step mode, print registers, print section of memory
		//s for step
		//play n to cancel step for n steps.
		//reg for registers
		//mem address n to print n bytes from address address
		int scanCode(char *input, int address = 0, int n = 0)
		{
			int LineWidth = 16;
			if(strncmp(input, "state", 5) == 0)
			{
				stateDump();
				return 0;
			}
			if(strncmp(input, "s", 1) == 0)
			{
				return 1;
			}
			if(strncmp(input, "play", 4) == 0)
			{
				int n;
				scanf("%d", &n);
				return n;
			}
			if(strncmp(input, "reg", 3) == 0)
			{
				registerDump();
				return 0;
			}
			if(strncmp(input, "exit", 4) == 0)
			{
				raise(SIGKILL);
			}
			
			if(strncmp(input, "mem", 1) == 0)
			{
				if(memUnit->isInStack(address))
				{
					int startingIndex = memUnit->stackBase-address;
					for(int i = startingIndex;i > startingIndex - n; i--)
					{
						printf("%x", memUnit->stack[i]);
					}
				}
				if(memUnit->MMUHeap.isInHeap(address))
				{
					uint8_t *backingMem = memUnit->MMUHeap.readHeapMemory(address, n, true);
					memUnit->MMUHeap.printHeap("Debug Print", address, pc);
					return 0;
				}
				if(memUnit->isInBinary(address))
				{
					printf("bytes:");
					char* hold = (char*)(calloc(n, sizeof(char)));
					if (bv->Read(hold, address, n) != n)
						generallyPause();
					else
					{
						int linePos=0, word=0;
						for(int i=0;i<n;i++)
						{
							
							printf("%x", hold[i]);
							word++;
							linePos++;
							if(word > 3)
							{
								printf(" ");
								word = 0;
							}
							if(linePos > LineWidth)
							{
								printf("\n");
								linePos = 0;
							}
						}
					}
				}
			}
			return 0;
		}
		
		void hooked_libc_write(uint32_t opcode)
		{
			// Size should be in $a2
			// s0 has the output stream
			// s1 has the buffer
			//printf("Called hooked libc write\n");
			//printf("$v0 0x%x\n", gpr[2]);
			//printf("$s0 0x%x\n", gpr[16]);
			//printf("$s1 0x%x\n", gpr[17]);
			// TODO: Denote the output buffer if we care.
			//printf("Intercepted WRITE call\n");
			//printf("Write size: 0x%0x\n", gpr[2]);
			printf("Getting string address 0x%lx of size 0x%0lx.\n", gpr[17], gpr[6]);
			char *address = memUnit->getEffectiveAddress(gpr[17], gpr[6], 17, gpr[17]);
			//printf("Addr send 0x%x\n", address);
			printf("%s\n", address);
			
			//this->pc = 0x0040a7b0;
			
			/*
			// Do the return.
			// Move $sp, $fp  ($sp=$fp)
			gpr[29] = gpr[30];
			
			printf("getting the first one\n");
			// $ra = [$sp + 0x24 {__saved_$ra}].d
			printf("Address %x:\n", (gpr[29] + 0x24));
			printf("Dereferenced %x:\n",  memUnit->getEffectiveAddress((gpr[29] + 0x24), 1, 31, gpr[29]));
			gpr[31] = (uint32_t)memUnit->getEffectiveAddress((gpr[29] + 0x24), 1, 31, gpr[29]);
			
			printf("getting the second one\n");
			// $fp = [$sp + 0x20 {__saved_$fp}].d
			printf("Address %x:\n", (gpr[29] + 0x20));
			printf("Dereferenced %x:\n",  memUnit->getEffectiveAddress((gpr[29] + 0x20), 1, 31, gpr[29]));
			gpr[30] = (uint32_t)memUnit->getEffectiveAddress((gpr[29] + 0x20), 1, 31, gpr[29]);
			
			// $sp = $sp + 0x28
			gpr[29] = gpr[29] + 0x28;
			*/
			// jump to ra
			this->pc = gpr[31];
			
			// memUnit->getEffectiveAddress(vAddr, 1, rs, gpr[rs]);
			
			return;
		}
		
		// size is at $a0
		// address goes in $v0
		// void* __libc_malloc(int sizeToAllocate)
		void hooked_libc_malloc(uint32_t opcode)
		{
			gpr[2] = (uint32_t)this->memUnit->MMUHeap.allocMem(gpr[4]);
			this->pc = gpr[31];
		}

		void hooked_libc_free(uint32_t opcode)
		{
			memUnit->MMUHeap.freeHeapMemory(gpr[4]);
			// jump to ra
			this->pc = gpr[31];
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
				printf("ADDIU %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
			}

			//this->signExtend(&immediate, 16, 32);
			int64_t temp = gpr[rs] + signedImmediate;
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
				printf("ANDI %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), signedImmediate);
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
				// If the two registers greater than or equal, we increment PC by the offset.
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
			
			// TODO:
			// Before we do, let's verify that we're not about to jump into the GOT
			// Equivilent python coode is bv.get_sections_at(pc)[0].name OR .type (name will be .got and type will be PROGBITS)
			// If we're about to jump to the GOT, let's resolve the external call.
			
			// equivilent code is bv.get_symbol_at(addr).full_name
			// Then use dlsym to bind the symbol and open
			
			if(memUnit->isAddrExtern(temp))
			{
				printf("Found a call to an external function, this functionality isn't implemented!\n");
				generallyPause();
			}
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

			if(pc >= startOfMain && pc < endOfMain && rs == 31)
			{
				printf("Exiting gracefully\n");
				BNShutdown();
				raise(SIGKILL);
			}

			runInstruction(getNextInstruction());

			

			pc = temp;
		}
		void lb(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LB\n");
			}

			if (debugPrint)
			{
				printf("LB %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *byte = memUnit->getEffectiveAddress(vAddr, 1, rs, gpr[rs]);
			gpr[rt] = (int64_t)(*byte);
		}
		void lbu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LBU\n");
			}

			if (debugPrint)
			{
				printf("LBU %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *byte = memUnit->getEffectiveAddress(vAddr, 1, rs, gpr[rs]);
			gpr[rt] = (uint64_t)(*byte);

		}
		// MIPS 3
		void LD(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 2, likely going to be unimplemented
		void LDCz(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 3
		void LDL(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void LDR(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 1
		void lh(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void lhu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LHU\n");
			}

			if (debugPrint)
			{
				printf("LHU %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *bytes = memUnit->getEffectiveAddress(vAddr, 2, rs, gpr[rs]);
			if(bytes == NULL)
			{
				printf("bytes==NULL\n");
				signalException(MemoryFault);
			}
			if(memUnit->isInStack(vAddr))
			{
				//? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[-1], bytes[-2], bytes[-3], 
				//																	bytes[0], bytes[-1], bytes[-2], bytes[-3]);
				gpr[rt] = 0;
				gpr[rt] |= ((uint64_t)(bytes[-1] & 0xff));
				gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 8;
			}
			else
			{
				//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
				//																	bytes[0], bytes[1], bytes[2], bytes[3]);
				gpr[rt] = 0;
				gpr[rt] |= ((uint64_t)(bytes[1] & 0xff));
				gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 8;
			}
		}
		// MIPS 2
		void LL(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 3
		void LLD(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 1
		void lui(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LUI\n");
			}

			if (debugPrint)
			{

				printf("LUI %s, %x", getName(rt).c_str(), immediate);
			}
			int32_t hold = (int32_t) immediate;
			gpr[rt] = hold << 16;
		}

		//MIPS I
		void lw(uint32_t instruction)
		{
			bool BigEndian = true;
			is64bit = false;
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for LW\n");
			}

			if (debugPrint)
			{
				printf("LW %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}


		
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;
				if(immediate & 3 > 0)
				{
					printf("what?\n");
					signalException(MemoryFault);
				}
					
				
				//Destination address
				uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
				//Get bytes in-order from mmu
				//memUnit->printSections();
				char *bytes = memUnit->getEffectiveAddress(vAddr, 4, rs, gpr[rs]);
				if(bytes == NULL)
				{
					printf("bytes==NULL\n");
					signalException(MemoryFault);
				}
				
				fflush(stdout);
				//change their order depending on endianness??

				//Emulated stack pointer is backwards. have to reverse it for writes
				if(memUnit->isInStack(vAddr))
				{
					//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[-1], bytes[-2], bytes[-3], 
					//																	bytes[0], bytes[-1], bytes[-2], bytes[-3]);
					gpr[rt] = 0;
					gpr[rt] |= (uint64_t)(bytes[-3] & 0xff);
					gpr[rt] |= ((uint64_t)(bytes[-2] & 0xff)) << 8;
					gpr[rt] |= ((uint64_t)(bytes[-1] & 0xff)) << 16;
					gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 24;
				}
				else
				{
					//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
					//																	bytes[0], bytes[1], bytes[2], bytes[3]);
					gpr[rt] = 0;
					gpr[rt] |= (uint64_t)(bytes[3] & 0xff);
					gpr[rt] |= ((uint64_t)(bytes[2] & 0xff)) << 8;
					gpr[rt] |= ((uint64_t)(bytes[1] & 0xff)) << 16;
					gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 24;
				}
				
				//printf("%lx\n", gpr[rt]);
				/*if(BigEndian)
				{
					for(int i=0;i<4;i++)
					{
						gpr[rt] &= bytes[i] << i*8;
					}
				}
				else
				{
					for(int i=0;i<4;i++)
					{
						gpr[rt] &= bytes[3-i] << i*8;
					}
				}*/
			}

		}
		// Likely to be unimplemented
		void lwcz(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void lwl(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void lwr(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 3
		void LWU(uint32_t instruction)
		{
			unimplemented(instruction);
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

		//MIPS 32
		void mul(uint32_t instruction)
		{
			if (mipsTarget < 32)
			{
				printf("Invalid mips target for MUL\n");
			}

			if (debugPrint)
			{
				printf("MUL %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			mult(instruction);
			gpr[rd] = LO;

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
		void PREF(uint32_t instruction)
		{
			printf("Passing a PREF...\n");
		}

		// MIPS 32-2
		void rdhwr(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for RDHWR\n");
			}

			if (debugPrint)
			{
				printf("RDHWR %s, %s", getName(rt).c_str(), getName(rd).c_str());
			}
			if(rd > 3 && rd < 29)
				signalException(ReservedInstructionException);
			if(rd >29)
				signalException(ReservedInstructionException);
			printf("hwr[rd] is [%ld]\n", hwr[rd]);
			gpr[rt] = hwr[rd];
		}
		// MIPS 1
		void sb(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SB\n");
			}

			if (debugPrint)
			{
				printf("SB %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *bytes = memUnit->getWriteAddresss(vAddr, 1, rs, gpr[rs]);
			bytes[0] = gpr[rt] & 0xff;
			
		}
		// MIPS 2
		void SC(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 3
		void SCD(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void SD(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 2
		void SDCz(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 3
		void SDL(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void SDR(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 1
		void sh(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SH\n");
			}

			if (debugPrint)
			{
				printf("SH %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *bytes = memUnit->getWriteAddresss(vAddr, 2, rs, gpr[rs]);
			if(bytes == NULL)
			{
				printf("bytes==NULL\n");
				signalException(MemoryFault);
			}
			if(memUnit->isInStack(vAddr))
			{
				//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[-1], bytes[-2], bytes[-3], 
				//																	bytes[0], bytes[-1], bytes[-2], bytes[-3]);
				bytes[0] = (gpr[rt] >> 8) & 0xff;
				bytes[-1] = (gpr[rt]) & 0xff;
			}
			else
			{
				//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
				//																	bytes[0], bytes[1], bytes[2], bytes[3]);
				bytes[0] = (gpr[rt] >> 8) & 0xff;
				bytes[1] = (gpr[rt]) & 0xff;
			}
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

				printf("SLL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
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

				printf("SLLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
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

				printf("SLT %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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

				printf("SLTI %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
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

				printf("SLTIU %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
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
				printf("Invalid mips target for SLTU\n");
			}

			if (debugPrint)
			{

				printf("SLTU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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

				printf("SRA %s, %s, %x\n", getName(rd).c_str(), getName(rt).c_str(), sa);
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

				printf("SRAV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
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

				printf("SRL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
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

				printf("SRLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
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
			bool BigEndian = true;
			is64bit = false;
			if (mipsTarget < 1)
			{
				printf("Invalid mips target for SW\n");
			}

			if (debugPrint)
			{
				printf("SW %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}


		
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;
				if(immediate & 3 > 0)
				{
					printf("what?\n");
					signalException(MemoryFault);
				}
					
				
				//Destination address
				uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
				//Get bytes in-order from mmu
				//memUnit->printSections();
				fflush(stdout);
				char *bytes = memUnit->getWriteAddresss(vAddr, 4, rs, gpr[rs]);
				fflush(stdout);
				if(bytes == NULL)
				{
					printf("bytes==NULL\n");
					signalException(MemoryFault);
				}
				//change their order depending on endianness??

				//The emulated pointer for the stack is backwards. Have to reverse it for writes
				if(memUnit->isInStack(vAddr))
				{
					bytes[0] = (gpr[rt] >> 24) & 0xff;
					bytes[-1] = (gpr[rt] >> 16) & 0xff;
					bytes[-2] = (gpr[rt] >> 8) & 0xff;
					bytes[-3] = (gpr[rt]) & 0xff;
				}
				else 
				{
					bytes[0] = (gpr[rt] >> 24) & 0xff;
					bytes[1] = (gpr[rt] >> 16) & 0xff;
					bytes[2] = (gpr[rt] >> 8) & 0xff;
					bytes[3] = (gpr[rt]) & 0xff;
				}
				//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
				//																		bytes[0], bytes[1], bytes[2], bytes[3]);
				/*if(BigEndian)
				{
					for(int i=0;i<4;i++)
					{
						gpr[rt] &= bytes[i] << i*8;
					}
				}
				else
				{
					for(int i=0;i<4;i++)
					{
						gpr[rt] &= bytes[3-i] << i*8;
					}
				*/
			}
		}
		void swcz(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void swl(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		void swr(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 2
		void SYNC(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 1
		void syscall(uint32_t instruction)
		{
			printf("Syscall preformed. Unimplemented, generally!\n");
			printf("$v0 is 0x%lx.\n", gpr[2]);
			//generallyPause();
			
		}
		// MIPS 2
		void teq(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TEQ\n");
			}

			if (debugPrint)
			{
				printf("TEQ %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if(gpr[rs] == gpr[rt])
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void teqi(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TEQI\n");
			}

			if (debugPrint)
			{
				printf("TEQI %s, %d\n", getName(rs).c_str(), signedImmediate);
			}

			if(gpr[rs] == signedImmediate)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tge(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TGE\n");
			}

			if (debugPrint)
			{
				printf("TGE %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}
			int64_t comp1 = (int64_t) gpr[rs];
			int64_t comp2 = (int64_t) gpr[rt];
			if(comp1 >= comp2)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		//@check comparison
		void tgei(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TGEI\n");
			}

			if (debugPrint)
			{
				printf("TGEI %s, %d\n", getName(rs).c_str(), signedImmediate);
			}

			if(gpr[rs] >= signedImmediate)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tgeiu(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TGEIU\n");
			}

			if (debugPrint)
			{
				printf("TGEIU %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			uint64_t comparison = (int64_t) signedImmediate;
			if(gpr[rs] >= comparison)
			{
				signalException(TrapFault);
			}
		}
		void tgeu(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TGEU\n");
			}

			if (debugPrint)
			{
				printf("TGEU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			if(gpr[rs] >= gpr[rt])
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tlt(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TLT\n");
			}

			if (debugPrint)
			{
				printf("TLT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}

			int64_t comp1 = (int64_t) gpr[rs];
			int64_t comp2 = (int64_t) gpr[rt];
			if(comp1 < comp2)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tlti(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TLTI\n");
			}

			if (debugPrint)
			{
				printf("TLTI %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			int64_t comparison = (int64_t) signedImmediate;
			if(gpr[rs] < comparison)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tltiu(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TLTIU\n");
			}

			if (debugPrint)
			{
				printf("TLTIU %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			uint64_t comparison = (int64_t) signedImmediate;
			if(gpr[rs] < comparison)
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tltu(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TLTU\n");
			}

			if (debugPrint)
			{
				printf("TLTU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}
			
			if(gpr[rs] < gpr[rt])
			{
				signalException(TrapFault);
			}
		}

		//MIPS II
		void tne(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TNE\n");
			}

			if (debugPrint)
			{
				printf("TNE %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
			}
			
			if(gpr[rs] != gpr[rt])
			{
				signalException(TrapFault);
			}
		}
		void tnei(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printf("Invalid mips target for TNEI\n");
			}

			if (debugPrint)
			{
				printf("TNEI %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			
			if(gpr[rs] != signedImmediate)
			{
				signalException(TrapFault);
			}
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

		//Prints the state, shamelessly ripping off that one emulator.
		//+---------------------+-----------------------+
		//|Registers:           |                       |
		//|zero:				|
		//|at:					|
		//|v0:					| Meta informations
		//|v1:					|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						+--------------------------
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|Local instructions from PC
		//|						|
		//|						|
		//|						|
		//|						|
		//|						+-------------------------
		//|						| Memory Increasing from sp
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//|						|
		//+---------------------+----------------

		void stateDump()
		{
			vector<char *> validPointers = vector<char *>();
			vector<int> validRegIndices = vector<int>();
			uint32_t *hold = (uint32_t *)calloc(sizeof(uint32_t), 2);
			printf("+-----------------------+---\n");
			bool isValidMemoryPtr;
			char *memTest;
			//Check registers for pointers
			for(int i=0;i<32;i++)
			{
				isValidMemoryPtr = memUnit->isInMemory(gpr[i]);
				if(isValidMemoryPtr)
				{
					memTest = memUnit->getEffectiveAddress(gpr[i], 4, 0, 0, true);
					validRegIndices.push_back(i);
					validPointers.push_back(memTest);
				}
			}

			for(int i=0;i<32;i++)
			{
				//Print the registers
				isValidMemoryPtr = memUnit->isInMemory(gpr[i]);
				printf("|");
				if(isValidMemoryPtr)
				{
					printf("\x1b[42m");
				}
				printf("%4s -> 0x%08lx\t", getName(i).c_str(), gpr[i]);
				if(isValidMemoryPtr)
					printf("\x1b[0m");

				//For single cases in meta
				switch(i)
				{
					case 0: printf("|");
					break;
					case 1: printf("| pc: 0x%lx", pc);
					break;
					case 2: printf("|");
					break;
					case 3: printf("|");
					break;
					case 4: printf("|");
					break;
					case 5: printf("|");
					break;
					case 6: printf("|");
					break;
					case 7: printf("+---");
					break;
					case 20: printf("+---");
					break;
					case 21: printf("| Pointers:");
				}

				//For the local instructions
				if(i>7 && i <= 19)
				{
					int base = i - 8 - 6;
					int offsetPc = pc + (4 * base);
					uint32_t inst = getInstruction(offsetPc);
					char *test = getInstructionName(inst);
					printf("| ");
					if(offsetPc == pc)
						printf("\x1b[44m");
					printf("0x%x: %s", offsetPc, getInstructionName(inst));
					if(offsetPc == pc)
						printf("\x1b[0m");
				}
				//For the pointers read
				if(i > 21)
				{
					printf("| ");
					//printf("%d, %d", validRegIndices.size(), validPointers.size());
					if(i-22 < validRegIndices.size() && i-22 < validPointers.size())
					{
						uint32_t loadedWord;
						uint64_t vAddr = gpr[validRegIndices[i-22]];
						char *bytes = validPointers[i-22];
						if(memUnit->isInStack(vAddr))
						{
							printf("Thisisinstack");
							loadedWord = 0;
							loadedWord |= (uint64_t)(bytes[-3] & 0xff);
							loadedWord |= ((uint64_t)(bytes[-2] & 0xff)) << 8;
							loadedWord |= ((uint64_t)(bytes[-1] & 0xff)) << 16;
							loadedWord |= ((uint64_t)(bytes[0] & 0xff)) << 24;
						}
						else
						{
							loadedWord = 0;
							loadedWord |= (uint64_t)(bytes[3] & 0xff);
							loadedWord |= ((uint64_t)(bytes[2] & 0xff)) << 8;
							loadedWord |= ((uint64_t)(bytes[1] & 0xff)) << 16;
							loadedWord |= ((uint64_t)(bytes[0] & 0xff)) << 24;
						}
						
						if(i-22 >= 0 && i-22 < validPointers.size())
						{
							printf("%s: 0x%08x", getName(validRegIndices[i-22]).c_str(), loadedWord);
						}
					}
					
				}

				printf("\n");
			}
			printf("+-----------------------+---\n");
			
			
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

		char * getInstructionName(uint32_t instruction)
		{
			if ((instruction & 0xfc000000) == 0)
			{
				// Essentially, this is a list of r type functions indexed by opcode.
				char * out = rtype_names[(instruction & 0b111111)];
				return out;
							
			}
			// If the upper 26-31 bits are set to one, then, we have a REGIMM instruction
			else if ((instruction & 0xfc000000) >> 26 == 1)
			{
				char * out = regimm_names[((instruction & 0x1f0000) >> 16)];
				return out;
			}
			else
			{
				char * out = otype_names[(instruction & 0xfc000000) >> 26];
				return out;
			}	
			return "unimplemented";
		}

		std::string hexFromInt(uint32_t in)
		{
			std::stringstream out;
			out << "0x" 
				<< std::setfill ('0') << std::setw(sizeof(uint32_t)*2) 
				<< std::hex << in;
			return out.str();
		}
		
		// pg. 40
		void runInstruction(uint32_t instruction)
		{
			printf("PC: [0x%lx], Instruction: [0x%08x]\n", pc, instruction);
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
			
			//printf("immediate signed: %d immediate unsigned: %d\n", signedImmediate, immediate);
			//printf("Rtype [%d]\n", (instruction & 0b111111));
			//printf("Regimm [%d]\n", ((instruction & 0x1f0000) >> 16));
			//printf("Otype [%d]\n", ((instruction & 0xfc000000) >> 26));
			// TODO: add bounds checking : )
			// If the upper 26-31 bits are set to zero, then, we have an R-Type instruction
			if ((instruction & 0xfc000000) == 0)
			{
				// Essentially, this is a list of r type functions indexed by opcode.
				//printf("Selected Rtype [%d]\n", (instruction & 0b111111));
				(this->*inst_handlers_rtypes[(instruction & 0b111111)])(instruction);
							
			}
			// If the upper 26-31 bits are set to one, then, we have a REGIMM instruction
			else if ((instruction & 0xfc000000) >> 26 == 1)
			{
				//printf("Selected Regimm [%d]\n", ((instruction & 0x1f0000) >> 16));
				(this->*inst_handlers_regimm[((instruction & 0x1f0000) >> 16)])(instruction);
			}
			else
			{
				//printf("Selected Otype [%d]\n", ((instruction & 0xfc000000) >> 26));
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
	printf("[INFO] BV Instantiated!\n");
	fflush(stdout);
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

	// Name: main Last Address: 56527bb27790
	


	// Begin Emulation
	EmulatedCPU* electricrock = new EmulatedCPU(false, bv);
	
	// This should get us the value of something interesting.
	// Should give us 3c1c0025 in unsigned decimal (using the a.out test file)
	//printf("Entry point:0x%lx\t%x\n", bv->GetEntryPoint(), electricrock->debugGetValue(0x400480, 0));
	//fflush(stdout);

	// Test for isAddrExtern
	//printf("External?: %d\n", electricrock->memUnit->isAddrExtern(0x410810));
	//(uint32_t)bv->GetEntryPoint()
	//electricrock->startOfMain
	electricrock->runEmulation(electricrock->startOfMain);

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
