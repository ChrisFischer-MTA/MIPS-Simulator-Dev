// Christopher Fischer and Rose Newcomer and Sean Kemp
// 03FEB2022
// Emulator Specific Information

//    .deez...      .....       .eeec.   ..eee..
//   .d*"  """"*e..d*"""""**e..e*""  "*c.d""  ""*e.
//  z"           "$          $""       *F         **e.
// z"             "c        d"          *.           "$.
//.F                        "            "            'F
//d                                                   J%
//3            E L E C T R I C   R O C K              e"
//4r                                                d"
// $     .d"     .        .F             z ..zeeeeed"
// "*beeeP"      P        d      e.      $**""    "
//     "*b.     Jbc.     z*%e.. .$**eeeeP"
//        "*beee* "$$eeed"$$$^$$$""    "
//                      d$$$$$$"
//                    .d$$$$$$"
//                   .$$$$$$$"
//                  z$$$$$$$beeeeee
//                 d$$$$$$$$$$$$$*
//                ^""""""""$$$$$"
//                        d$$$*
//                       d$$$"
//                      d$$*
//                     d$P"
//                   .$$"
//                  .$P"
//
// print cloud lines
// then print bolt emerging from the damn cloud

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <inttypes.h>
#include <iomanip>
#include <signal.h>
#include <time.h>
#include <filesystem>
#include <limits>
#include <algorithm>


#include "mmu.cpp"
#include"argparse/argparse.hpp"






#define BIT16 0x8000
#define BIT32 0x80000000
#define BIT64 0x8000000000000000
#define BIT(n) (uint64_t) 1 << (n-1)
#define LOWERMASK(n) ((uint64_t) 1 << (n)) - 1

#define INT_MAX 2147483647


const short int MIP_ISA_32 = 1;

const short int MIPSI = 1;
const short int MIPSII = 2;
const short int MIPSIII = 3;
const short int MIPSIV = 4;

const short int RTYPE = 1;
const short int ITYPE = 2;
const short int JTYPE = 3;

// Debug oprtions
short int SHUT_UP = 0;


// Exception Types
const short int IntegerOverflow = 1;
const short int MemoryFault = 2;
const short int TrapFault = 3;
const short int ReservedInstructionException = 4;

// Coverage Information
std::vector<uint32_t> basicBlocks;
std::vector<std::string> basicBlockNames;
std::vector<double> instructionTimes;
std::vector<uint32_t> instructionOPs;
std::vector<std::string> batchNames;
FILE* PCPathFile = NULL;
bool pcoutFlag = false;
bool regDumpFlag = false;
bool beQuietFlag = false;
bool timer = false;
bool batchMode = false;
clock_t startOfEmulation, endOfEmulation;
double cpu_time_used;
int globalLogLevel = 0;
int stepsize = 1;

// Function Information (for hooking)
std::vector<uint32_t> functionVirtualAddress;
// Array offset in our hooked functions table which dictates which function the emualator calls
std::vector<short int> functionVirtualFunction; 

const short int NUM_FUNCTIONS_HOOKED = 4;

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
			&EmulatedCPU::hooked_libc_free,	
			&EmulatedCPU::hooked_libc_scanf
			//&EmulatedCPU::hooked_libc_fwrite	
		};
		
		const std::string static_function_hook_matching[NUM_FUNCTIONS_HOOKED] = 
		{
			"__stdio_WRITE",
			"__libc_malloc",
			"free",
			"scanf"
			//"__stdio_fwrite"
		};

		// Registers and Instruction Fields
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
		uint64_t instructionsRun = 0;
		uint32_t endOfMain, startOfMain;
		vector<uint32_t> instructionPointerBreakpoints;
		vector<char *> symbolBreakpoints;

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
			//gpr[29] = memUnit->stackBase - 28 - 396;
			gpr[29] = memUnit->stackBase - 0xf20;

			uint32_t UserLocalPtr = memUnit->MMUHeap.allocMem(12, true) + 6;
			hwr[29] = UserLocalPtr;
			
			if(globalLogLevel >= 7)
				memUnit->printSections();
			
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
						//printNotifs(7, "%x, %x", startOfMain, endOfMain);
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
							// printf("hooking: %s @ 0x%x \n", currentFunctionName.c_str(), func->GetStart());
						}
					}
				}
			}
			
			
		}

		// This function is a hacky way for us to freeze in the debug console which will be replaced
		// by a system call in the future.
		void generallyPause()
		{
			// --timer stuff
			endOfEmulation = clock();
			if (timer == true)
			{
				cpu_time_used = ((float)(endOfEmulation - startOfEmulation)/ CLOCKS_PER_SEC); 
				printf("\nTotal time for emulation: %f seconds\n", cpu_time_used);
				printf("Instruction count: %x\n", instructionsRun);
				printf("Instructions per second: %f inst/s\n", instructionsRun/cpu_time_used);
			}

			// TODO: Import please no steppy.
			char *pweasenosteppy = (char *) calloc(1024, sizeof(char));
			int next, skip = 0;
			if(SHUT_UP)
			{
				printf("Gracefully exiting.\n");
				BNShutdown();
				raise(SIGKILL);
			}
			while(true)
			{
				//Handle the input to the emulation
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
		}

		uint32_t debugGetValue(int address, int retVal)
		{	
			unsigned char* bytes = memUnit->getBytes(address);
				
			retVal += ((bytes[0] & 0xff) << 24);
			retVal += ((bytes[1] & 0xff) << 16);
			retVal += ((bytes[2] & 0xff) << 8);
			retVal += ((bytes[3] & 0xff));

			printNotifs(7,"%x ", (bytes[0] & 0xff));
			printNotifs(7,"%x ", (bytes[1] & 0xff));
			printNotifs(7,"%x ", (bytes[2] & 0xff));
			printNotifs(7,"%x\n", (bytes[3] & 0xff));

			
			return retVal;
		}

		void signalException(int excpt)
		{
		printNotifs(1,"Exception occured! [%d]\n", excpt);
			while(true) generallyPause();
		}

		// Get instructions located at a memory address and return them in a usable format.
		// While doing some permissions checks.
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

		// Call to bridge unimplemented instruction handlers with helpful debug information.
		void unimplemented(uint32_t opcode)
		{
			printNotifs(4,"Unimplemented emulation instruction was called.\n");
			printNotifs(6,"Operation Code [0x%x], detected index:\n", opcode);
			if ((opcode & 0xfc000000) == 0)
				printNotifs(6,"Rtype [%d]\n", (opcode & 0b111111));
			else if ((opcode & 0xfc000000) >> 26 == 1)
				printNotifs(6,"Regimm [%d]\n", ((opcode & 0x1f0000) >> 16));
			else
				printNotifs(6,"Otype [%d]\n", ((opcode & 0xfc000000) >> 26));
			while(true) generallyPause();
			return;
		}

		// Takes in a program counter that is the entry point.
		// Unused.
		void runEmulation(int entryPoint)
		{
			startOfEmulation = clock();
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

				if (pcoutFlag)
				{
					fprintf(PCPathFile, "0x%x\n", pc);
					fflush(PCPathFile);
				}

				if(findIterator != basicBlocks.end())
				{
					// Get the index
					index = findIterator-basicBlocks.begin();
					printNotifs(6,"Current PC:  0x%lx - Start of a Basic Block in: %s\n", pc, basicBlockNames[index].c_str());
				}
				else
				{
					printNotifs(6,"Current PC:  0x%lx - Last Found Basic Block in: %s\n", pc, basicBlockNames[index].c_str());
				}
				
				// Check to see if we've entered a hooked function
				
				
				// Code to search iteratively through our hooked functions and find if PC is a hooked address.
				
				findIterator = std::find(functionVirtualAddress.begin(), functionVirtualAddress.end(), pc);
				if(findIterator != functionVirtualAddress.end())
				{
					printNotifs(5,"Found a hooked function, calling appropriate hooked implementation!\n");
					index = findIterator-functionVirtualAddress.begin();
					printNotifs(5,"Index: [%d] name of [%s] \n", index, static_function_hook_matching[functionVirtualFunction[index]].c_str());
					(this->*static_function_hooks[functionVirtualFunction[index]])(0x0);
					//registerDump();
					
					//while(true);
					// TODO: Manually set up the stack after intercepting a function call.
				}
				
				// Ensure file exists and overwrite if necessary
				//FILE *fp;
				//fp = fopen("pathing", "w+");
				//fclose(fp);
				
				//If the instruction is not nullified, fetch and run it.
				if (!instructionNullify)
				{
					uint32_t instruction = getInstruction(pc);
					runInstruction(instruction);
					
					instructionsRun++;
					printNotifs(6,"Instructions run: %lld\n\n", instructionsRun);
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
				
				
				//Track the changes to the stack and frame pointer
				//these are set at the end of the last instruction (In this block) and referenced at this block
				uint64_t oldFramePointer, oldStackPointer;
				if(gpr[29] != oldStackPointer)
				{
					memUnit->newStackSection(gpr[29]);

					oldStackPointer = gpr[29];
				}
				if(gpr[30] != oldFramePointer)
				{
					memUnit->newFrameSection(gpr[30]);


					oldFramePointer = gpr[30];
				}
				/*
				if(instructionsRun == 6)
				{
					gpr[28] = memUnit->GOTpointer;
				}*/


				//Handle the input to the emulation
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
					else if(strncmp(pweasenosteppy, "stepsize", 8) == 0)
					{
						int stepsize;
						scanf("%d", &stepsize);
						flags = scanCode(pweasenosteppy, stepsize);
					}
					else if(strncmp(pweasenosteppy, "break", 5) == 0)
					{
						char *breaktag = (char *)calloc(128, sizeof(char));
						scanf("%s", breaktag);
						flags = scanCode(pweasenosteppy, 0, 0, breaktag);
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
		//mem address n to print n bytes (decimal) from address address
		//example of mem accessing close to the beginninng of the heap: mem 10040 32
		int scanCode(char *input, int address = 0, int n = 0, char *breaktag = NULL)
		{
			int LineWidth = 16;
			if(strncmp(input, "stepsize", 8) == 0)
			{
				stepsize = address;
				return 0;
			}
			else if(strncmp(input, "state", 5) == 0)
			{
				stateDump();
				return 0;
			}
			else if(strncmp(input, "stepi", 5) == 0)
				return 1;
			else if(strncmp(input, "step", 4) == 0)
			{
				return stepsize;
			}
			else if(strncmp(input, "s", 1) == 0)
			{
				return 1;
			}
			else if(strncmp(input, "break", 5) == 0)
			{
				if(breaktag == NULL)
					return 0;
				//if the first char is *, then the breaktag is a pointer to an instruction
				if(breaktag[0] == '*')
				{
					//if first char is *, then the string must be *0x(*)
					//breaktag + 3;
					uint32_t breakpoint = std::stoul(breaktag+1, NULL, 16);
					printf("0x%x\n", breakpoint);
					instructionPointerBreakpoints.push_back(breakpoint);
					return 0;
				}
				//else, the breaktag is the name of a symbol
				std::vector<string>::iterator it;
				it = find(basicBlockNames.begin(), basicBlockNames.end(), breaktag);
				if(it != basicBlockNames.end())
				{
					printf("%s\n", breaktag);
					symbolBreakpoints.push_back(breaktag);
				}
				else
				{
					printNotifs(7, "Symbol not found\n");
				}
				return 0;
			}
			else if(strncmp(input, "play", 4) == 0)
			{
				int n;
				scanf("%d", &n);
				return n;
			}
			else if(strncmp(input, "continue", 8) == 0)
			{
				return INT_MAX;
			}
			else if(strncmp(input, "reg", 3) == 0)
			{
				registerDump();
				return 0;
			}
			else if(strncmp(input, "exit", 4) == 0 || strncmp(input, "kill", 4) == 0 || strncmp(input, "quit", 4) == 0)
			{
				if(PCPathFile)
				{
					fflush(PCPathFile);
					fclose(PCPathFile);
				}
				raise(SIGKILL);
			}
			else if(strncmp(input, "help", 4) == 0)
			{
				printf("\nplay [#]:\n\t- Plays the next [#] of instructions forward in execution.\n");
				printf("step:\n\t- Steps forward in execution,\n");
				printf("reg:\n\t- Provides a register dump to the screen.\n");
				printf("state:\n\t- Provides a diagram of the state of the program, including registers, pc, and pointers.\n");
				printf("mem [hexVal] [numBytes]:\n\t- Provides contents of memory at hexVal\n");
				printf("exit:\n\t- Kills program, and exits cleanly.\n");
			}
			
			else if(strncmp(input, "mem", 1) == 0)
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
					memUnit->MMUHeap.printHeap("Debug Print", address, pc, true, address - n, address + n);
					return 0;
				}
				if(memUnit->isInBinary(address))
				{
					printNotifs(7,"bytes:");
					char* hold = (char*)(calloc(n, sizeof(char)));
					if (bv->Read(hold, address, n) != n)
					{
						printNotifs(7, "bv read generally pause\n");
						while(true) generallyPause(); // Why was this put here?
					}
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
			
			else 
			{
				printNotifs(7,"Invalid command! Please enter help for some help.");
			}
			return 0;
		}
		
		//
		// Hooked Functions
		// 
		
		
		void hooked_libc_write(uint32_t opcode)
		{
			// s1 - previous string offset
			// a1 - best friend
			// a2 
			
			//registerDump();
			printNotifs(6,"Getting string address 0x%lx of size 0x%0lx.\n", gpr[5], gpr[6]);
			char *address = memUnit->getEffectiveAddress(gpr[5], gpr[6], 17, gpr[5]);
			if(address == NULL)
				signalException(MemoryFault);
			printf("%s", address);
			this->pc = gpr[31];
			return;
		}
		
		void hooked_libc_malloc(uint32_t opcode)
		{
			gpr[2] = (uint32_t)this->memUnit->MMUHeap.allocMem(gpr[4]);
			this->pc = gpr[31];
		}

		void hooked_libc_free(uint32_t opcode)
		{
			if(memUnit->MMUHeap.freeHeapMemory(gpr[4]) == 0)
			{
				generallyPause();
			}
			// jump to ra
			this->pc = gpr[31];
		}

		// NOTE: This is still not done. This is a complex function that I am still hooking.
		// it's currently on the backburner as we fix an emulation bug.
		void hooked_libc_scanf(uint32_t opcode)
		{
			// a0 is the pointer to the buffer.
			// a1 is the first pointer access.
			// a2 is the second pointer access.
			// a3 is the third pointer access.
			// stack+0x10 is the forth pointer access
			// stack+0x14 is the fifth pointer access/
			int i = 0;
			int formatStrLen = 0;
			int numSpecifiers = 0;
			char *memPtr = memUnit->getEffectiveAddress(gpr[4], 4, 4);
			
			if(memUnit->isInStack(gpr[4]))
			{
				for(i = 0; *memUnit->getEffectiveAddress(gpr[4]-i, 4, 4) != 0;)
				i++;
			}
			else
			{
				for(i = 0; *memUnit->getEffectiveAddress(gpr[4]+i, 4, 4) != 0;)
				i++;
			}
			char *targetFormatStr =(char *)calloc(i+1, sizeof(char));
			
			memUnit->readFromMMU(memPtr, gpr[4], targetFormatStr, i);
			targetFormatStr[i] = 0;

			//Need to convert mem pointer's order in case argument is in stack

			
			
			// Get the string length of the pointer in a0
			// We have to do it this way that way we monitor for a potential overflow if we
			// get passed a corrupted string. This is a computationally expense operation.
			
			
			formatStrLen = i;
			// Now, let's detect how many format specifiers we have in this string.
			// This is a rough way of doing this but this should be correct.
			for(i = 0; i < formatStrLen; i++)
				if(targetFormatStr[i] == '%')
					numSpecifiers++;
			
			
			
			printNotifs(7,"Detected scanf of [%d] with [%d] specifiers.\n", i, numSpecifiers);
			
			if(numSpecifiers == 0)
			{
				this->pc = gpr[31];
				return;
			}
			
			// Allocate some space to store the pointers for a bit.
			void* pntrStorage = malloc(sizeof(void*) * numSpecifiers);
			char *scanToken;
			int numBytes;
			switch(numSpecifiers)
			{
				case 1:
					// get the memory address that a1 points too
					printNotifs(7,"processing scanf with 1 and a str of [%s]. \n", targetFormatStr );
					if(strstr("%s",targetFormatStr) != NULL)
					{
						printNotifs(3, "Found an unbounded %s in scanf. This can always result in a buffer overflow.\n");
						signalException(MemoryFault);
					}
					//Just for example: This establishes that the width of the scan is 4 bytes (and matches the scanf in switch)
					//The idea here is to buffer the result of scanf so it can be hand-fed into the mmu
					//But in order to seperate it like that, we need to know the length in bytes of the scan
					//That's the idea, but the for some reason that is beyond my ken the bytes of scanToken aren't getting 0'd out
					//
					if(targetFormatStr[1] == 'd')
					{
						printf("detected d\n");
						numBytes = 4;
						scanToken = (char *)calloc(4, sizeof(char));
						for(int i = 0;i< 4;i++)
						{
							scanToken[i] = 0;	
						}

							
						printf("value from scanf: [%d%d%d%d]\n", scanToken[0], scanToken[1], scanToken[2], scanToken[3]);
						//I think scanf casts the pointer in the second argument anyway but. it's a peace of mind thing
						scanf(targetFormatStr, (int *)scanToken);
						printf("value from scanf: [%d%d%d%d]\n", scanToken[0], scanToken[1], scanToken[2], scanToken[3]);
					}
					break;
				case 2:
					printNotifs(7,"processing scanf with 2 and a str of [%s]. \n", targetFormatStr);
					scanf(targetFormatStr, memUnit->getEffectiveAddress(gpr[5], 4, 5), memUnit->getEffectiveAddress(gpr[6], 4, 6));
					break;
				case 3:
					printNotifs(7,"processing scanf with 3 and a str of [%s]. \n", targetFormatStr);
					scanf(targetFormatStr, memUnit->getEffectiveAddress(gpr[5], 4, 5), memUnit->getEffectiveAddress(gpr[6], 4, 6), memUnit->getEffectiveAddress(gpr[7], 4, 7));
					break;
			
				default:
					/*for(i = 4; i < numSpecifiers; i++)
					{
						// Get magic number by getting the stack offset
						// in our case, we subtract 4 from i, add 10, then
						// multiply i by 4 
						int magicNum = ( (i-4) * 4 ) + 0x10;
						//uint64_t address, int numBytes, int gpr, uint64_t contents = 0, 
						//	   bool suppressHeap = 0)
						pntrStorage[i] = memUnit->getEffectiveAddress(gpr[29] + magicNumber, 
								4, 29, NULL);
					}*/
					printNotifs(1, "unimplemented scanf\n");
					while(true);
					break;
				
				
			}
			char *writePtr = memUnit->getEffectiveAddress(gpr[5], 4, 5);
			char *test = (char *)calloc(4, sizeof(char));
			memUnit->writeToMMU(writePtr, gpr[5], scanToken, numBytes);
			memUnit->readFromMMU(writePtr, gpr[5], test, 4);
			printf("value from scanf: [%d]\n", *((uint32_t *)scanToken));
			
			
			
			this->pc = gpr[31];
		}

		void hooked_libc_fwrite(uint32_t opcode)
		{//4,5
			//Only set for stream of stdout. Avoids formatted print for consistency.
			int length = gpr[5];
			char *buffer = memUnit->getEffectiveAddress(gpr[4], length, 0);
			if(buffer == NULL | !memUnit->isInMemory(gpr[4] + length))
			{
				printNotifs(2, "Bad length or pointer in fwrite\n");
				signalException(MemoryFault);
			}
			
			
			int i=0;
			while(i<length)
			{
				putc(buffer[i], stdout);
				if(memUnit->isInStack(gpr[4]))
					i--;
				else
					i++;
			}
				


			//printf("The answer is %s")
			//puts("The answer is %s")
			
		}

		// 
		// Emulationed Operation codes for the JIT. 
		//


		// This is the ADD function. Opcode of 0b000000 and ALU code of 0b100 000
		// TODO: Test with negative values.
		void add(uint32_t opcode)
		{

			if(mipsTarget < 1)
			{
				printNotifs(4,"Invalid mips target for ADD\n");
			}

			if(debugPrint)
			{
				printNotifs(7,"ADD %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4,"Invalid mips target for ADDI\n");
			}
			if (debugPrint)
			{
				printNotifs(7,"ADDI %s, %s, %d\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for ADDIU\n");
			}

			if (debugPrint)
			{
				printNotifs(7,"ADDIU %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
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
				printNotifs(4, "Invalid mips target for ADDU\n");
			}

			if (debugPrint)
			{
				printNotifs(7,"ADDU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for AND\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "AND %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			gpr[rd] = gpr[rs] & gpr[rt];
		}
		void andi(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for ANDI\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "ANDI %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), signedImmediate);
			}
			gpr[rt] = gpr[rs] & immediate;
		}
		
		void beq(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for BEQ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BEQ %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
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
				printNotifs(4, "Invalid mips target for BEQL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BEQL %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for BEQ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BGEZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}
			
			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;
			int64_t lhs = gpr[rs];

			delaySlot = true;

			runInstruction(getNextInstruction());
			if (lhs >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgezal(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for BGEZAL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BGEZAL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			int64_t lhs = gpr[rs];

			delaySlot = true;

			// Set return address equal to the value.
			gpr[31] = pc + 8;

			if (lhs >= 0)
			{
				// If the two registers greater than or equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgezall (uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for BGEZALL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BGEZALL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			int64_t lhs = gpr[rs];

			// Set return address equal to the value.
			gpr[31] = pc + 8;

			if (lhs >= 0)
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
				printNotifs(4, "Invalid mips target for bgezl\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "bgezl %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			int64_t lhs = gpr[rs];
			
			if (lhs >= 0)
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
				printNotifs(4, "Invalid mips target for BGTZ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BGTZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;

			int64_t lhs = gpr[rs];

			if (gpr[rs] >= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bgtzl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printNotifs(4, "Invalid mips target for BGTZL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BGTZL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			int64_t lhs = gpr[rs];
			
			if (lhs >= 0)
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
				printNotifs(4, "Invalid mips target for BLEZ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLEZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;
			int lhs = gpr[rs];

			if (lhs <= 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void blezl(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printNotifs(4, "Invalid mips target for BLEZL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLEZL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			int64_t lhs = gpr[rs];

			if (lhs <= 0)
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
				printNotifs(4, "Invalid mips target for BLTZ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLTZ %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;
			int64_t lhs = gpr[rs];

			if (lhs < 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bltzal(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for BLTZAL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLTZAL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			delaySlot = true;
			int64_t lhs = gpr[rs];

			gpr[31] = pc + 8;
			if (lhs < 0)
			{
				// If the two registers equal, we increment PC by the offset.
				tgt_offset = extendedImmediate;
			}
		}
		void bltzall(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printNotifs(4, "Invalid mips target for BLTZALL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLTZALL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;

			gpr[31] = pc + 8;
			int64_t lhs = gpr[rs];
			if (lhs < 0)
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
				printNotifs(4, "Invalid mips target for BLTZL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BLTZL %s, %x\n", getName(rs).c_str(), signedImmediate);
			}

			int32_t extendedImmediate = signedImmediate;
			extendedImmediate <<= 2;
			int64_t lhs = gpr[rs];

			if (lhs < 0)
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
				printNotifs(4, "Invalid mips target for BNE\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "BNE %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
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
				printNotifs(4, "Invalid mips target for bne\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "bne %s, %s, %x\n", getName(rs).c_str(), getName(rt).c_str(), (instruction & 0xFFFF));
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
				printNotifs(4, "Invalid mips target for DADD\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "DADD %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DADDI\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DADDI %s, %s, %d\n", getName(rs).c_str(), getName(rt).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for DADDIU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "DADDIU %s, %s, %dx\n", getName(rs).c_str(), getName(rt).c_str(), immediate);
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
				printNotifs(4, "Invalid mips target for DADDU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "DADDU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			uint64_t temp = gpr[rs] + gpr[rt];

			gpr[rd] = temp;
		}

		//MIPS III
		void ddiv(uint32_t instruction)
		{
			
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DDIV\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DDIV %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DDIVU\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DDIVU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DIV\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DIV %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DIVU\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DIVU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DMULT\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DMULT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DMULT\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DMULT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DSLL\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSLL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}
			
			gpr[rd] = gpr[rt] << sa;
		}

		//MIPS III
		void dsll32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSLL32\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSLL32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] << (sa + 32);
		}

		//MIPS III
		void dsllv(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSLLV\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSLLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] << (gpr[rs] & 0x3f);
		}
		
		//MIPS III
		void dsra(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRA\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRA %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t) (hold >> sa);
		}

		//MIPS III
		void dsra32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRA32\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRA32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t)(hold >> (sa+32));
		}

		//MIPS III
		void dsrav(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRAV\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRA %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			int64_t hold = gpr[rt];
			gpr[rd] = (uint64_t)(hold >> (gpr[rs] & 0x3f));
		}

		//MIPS III
		void dsrl(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRL\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] >> sa;
		}

		//MIPS III
		void dsrl32(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRL32\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRL32 %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] >> (sa+32);
		}

		//MIPS III
		void dsrlv(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSRLV\n");
			}
			if (debugPrint)
			{
				printNotifs(7, "DSRLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] >> (gpr[rs] & 0x3f);
		}

		//MIPS III
		void dsub(uint32_t instruction)
		{
			if (mipsTarget < 3)
			{
				printNotifs(4, "Invalid mips target for DSUB\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "DSUB %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for DSUBU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "DSUBU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for J\n");
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
				printNotifs(4, "Invalid mips target for JAL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "JAL %lx\n", instr_index);
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
				printNotifs(4, "Invalid mips target for JALR\n");
			}

			if (debugPrint)
			{
				
				printNotifs(7, "JALR %s", getName(rd).c_str());
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
				printNotifs(1, "Found a call to an external function, this functionality isn't implemented!\n");
				generallyPause();
			}
			pc = temp;
		}

		//MIPS I
		void jr(uint32_t instruction)
		{

			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for JR\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "JR %s\n", getName(rs).c_str());
			}
			
			uint64_t temp = gpr[rs];

			if(pc >= startOfMain && pc < endOfMain && rs == 31)
			{
				endOfEmulation = clock();
				if (timer == true)
				{
					cpu_time_used = (double)((endOfEmulation - startOfEmulation)/ CLOCKS_PER_SEC); 
					printf("Total time for emulation: %f", cpu_time_used);
				}
				printNotifs(6, "Exiting gracefully\n");
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
				printNotifs(4, "Invalid mips target for LB\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LB %s, %d\n", getName(rs).c_str(), signedImmediate);
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *byte = memUnit->getEffectiveAddress(vAddr, 1, rs, gpr[rs]);
			gpr[rt] = (int64_t)(*byte);
		}
		void lbu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for LBU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LBU %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
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
			bool BigEndian = true;
			is64bit = false;
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for LW\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LW %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			
		
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;
				if(immediate & 1 > 0)
				{
					printNotifs(7, "what?\n");
					signalException(MemoryFault);
				}
					
				
				//Destination address
				uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
				//Get bytes in-order from mmu
				//memUnit->printSections();
				char *bytes = memUnit->getEffectiveAddress(vAddr, 4, rs, gpr[rs]);
				if(bytes == NULL)
				{
					printNotifs(7, "bytes==NULL\n");
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
					gpr[rt] |= ((uint64_t)(bytes[-1] & 0xff)) << 0;
					gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 8;
				}
				else
				{
					//printf("victory? %s, %c%c%c%c, %hhx%hhx%hhx%hhx\n", getName(rt).c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
					//																	bytes[0], bytes[1], bytes[2], bytes[3]);
					gpr[rt] = 0;
					gpr[rt] |= ((uint64_t)(bytes[1] & 0xff)) ;
					gpr[rt] |= ((uint64_t)(bytes[0] & 0xff)) << 8;
				}
			}
		}
		void lhu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for LHU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LHU %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *bytes = memUnit->getEffectiveAddress(vAddr, 2, rs, gpr[rs]);
			if(bytes == NULL)
			{
				printNotifs(7, "bytes==NULL\n");
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
				printNotifs(4, "Invalid mips target for LUI\n");
			}

			if (debugPrint)
			{

				printNotifs(7,"LUI %s, %x\n", getName(rt).c_str(), immediate);
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
				printNotifs(4, "Invalid mips target for LW\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LW %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			
		
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;
				if(immediate & 3 > 0)
				{
					printNotifs(7, "what?\n");
					signalException(MemoryFault);
				}
					
				
				//Destination address
				uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
				//Get bytes in-order from mmu
				//memUnit->printSections();
				char *bytes = memUnit->getEffectiveAddress(vAddr, 4, rs, gpr[rs]);
				if(bytes == NULL)
				{
					printNotifs(7, "bytes==NULL\n");
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
		
		//load word left
		void lwl(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for LWL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LWL %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			bool BigEndian = true;
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			uint32_t lowMask = 0x00ffffff;
			int32_t highMask = 0;
			int i = vAddr & 3, j = 24, k = 0;
			char *bytes = memUnit->getEffectiveAddress(vAddr, 4-i, rs, 0);
			if(bytes == NULL)
				generallyPause();
				
			//printf("%d, %d, %d\n", i, vAddr);

			if(BigEndian)
			{
				
				if(memUnit->isInStack(vAddr))
				{
					while(i <= 3)
					{
						//mask only zeros out the byte being written to, preserving the bytes that dont get written to
						highMask = (~lowMask) << 8;
						gpr[rt] &= lowMask | highMask;
						gpr[rt] |= (uint64_t)bytes[k] << j;
						//bytes[k] = (gpr[rt] >> j) & 0xff;
						i++;
						j -= 8;
						k--;
						lowMask >>= 8;
						highMask >>= 8;

					}
				}
				else
				{
					while(i <= 3)
					{
						highMask = (~lowMask) << 8;
						//printf("i: %d\n", i);
						//mask only zeros out the byte being written to, preserving the bytes that dont get written to
						gpr[rt] &= lowMask | highMask;
						gpr[rt] |= (uint64_t)bytes[k] << j;
						//bytes[k] = (gpr[rt] >> j) & 0xff;
						i++;
						j -= 8;
						k++;
						lowMask >>= 8;
						highMask >>= 8;
					}
				}
				if(gpr[rt] >> 31 && is64bit)
					gpr[rt] |= (uint64_t)0xffffffff << 32;
				
			}
		}
		void lwr(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for LWR\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "LWR %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			bool BigEndian = true;
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			uint32_t lowMask = 0xff;
			int32_t highMask = 0xffffff00;


			int i = vAddr & 3, j = 0, k = 0;
			char *bytes = memUnit->getEffectiveAddress(vAddr - i, i, rs, 0);
			if(bytes == NULL)
				generallyPause();
			/*if(memUnit->isInStack(vAddr))
				bytes -= i;
			else
				bytes += i;*/

			if(bytes == NULL)
			{
				printNotifs(2, "Bytes == null in LWR");
				generallyPause();
			}
			
			if(BigEndian)
			{
					while(i >= 0)
					{
						//printf("Heap/binary lwring\n");
						
						lowMask = (uint64_t)(~highMask);
						lowMask >>= 8;
						bytes = memUnit->getEffectiveAddress(vAddr + k, 1, rs, 0);
						//printf("mask: %x, %x, %x\n", lowMask, highMask, lowMask | highMask);
						gpr[rt] &= lowMask | highMask;
						gpr[rt] |= (uint32_t)(bytes[0]) << j;
						//printf("%x\n", gpr[rt]);
						//printNotifs(7, "writing to %x", bytes+k);
						//bytes[k] = (gpr[rt] >> j) & 0xff;
						i--;
						j += 8;
						highMask <<= 8;
						k--;
					}
				
				if(vAddr & 3 == 3 && gpr[rt] >> 31 == 1)
				{
					gpr[rt] |= (uint64_t)0xffffffff << 32;
				}
			}
		
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
				printNotifs(4, "Invalid mips target for MFHI\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MFHI %s", getName(rd).c_str());
			}

			gpr[rd] = HI;
		}
		void mflo(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for MFLO\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MFLO %s", getName(rd).c_str());
			}

			gpr[rd] = LO;
		}
		// MIPS 4
		void MOVN(uint32_t instruction)
		{

			if (mipsTarget < 4)
			{
				printNotifs(4, "Invalid mips target for MOVN\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MOVN %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for MOVZ\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MOVZ %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for MTHI\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MTHI %s", getName(rs).c_str());
			}

			HI = gpr[rs];
		}

		//MIPS I
		void mtlo(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for MTLO\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MTLO %s", getName(rs).c_str());
			}

			LO = gpr[rs];
		}



		//MIPS I
		void mult(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for MULT\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MULT %s, %s", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for MULTU\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "MULTU %s, %s", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for MUL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "MUL %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			mult(instruction);
			gpr[rd] = LO;

		}

		//MIPS I
		void nor(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for NOR\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "NOR %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = ~(gpr[rs] | gpr[rt]);
		}

		//MIPS I
		void orop(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for OR\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "OR %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			printNotifs(7, "OR = %x | %x = %x\n", gpr[rs], gpr[rt], gpr[rs] | gpr[rt]);
			gpr[rd] = gpr[rs] | gpr[rt];
		}

		//MIPS I
		void ori(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for ORI\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "ORI %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			uint64_t extended = (uint64_t)immediate;
			gpr[rt] = extended | gpr[rs];
		}
		// MIPS 4
		void PREF(uint32_t instruction)
		{
			printNotifs(7, "Passing a PREF...\n");
		}

		// MIPS 32-2
		void rdhwr(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for RDHWR\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "RDHWR %s, %s", getName(rt).c_str(), getName(rd).c_str());
			}
			if(rd > 3 && rd < 29)
				signalException(ReservedInstructionException);
			if(rd >29)
				signalException(ReservedInstructionException);
			printNotifs(7, "hwr[%s] is [%ld]\n", getName(rd).c_str(), hwr[rd]);
			gpr[rt] = hwr[rd];
		}
		// MIPS 1
		void sb(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SB\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SB %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = signedImmediate + gpr[rs];
			printNotifs(7, "%lx\n", vAddr);
			char *bytes = memUnit->getWriteAddresss(vAddr, 1, rs, gpr[rs]);
			if(bytes == NULL)
			{
				printNotifs(7, "%lx, %x, %lld\n", vAddr, signedImmediate, gpr[rs]);
				generallyPause();
			}
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
				printNotifs(4, "Invalid mips target for SH\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SH %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];
			char *bytes = memUnit->getWriteAddresss(vAddr, 2, rs, gpr[rs]);
			if(bytes == NULL)
			{
				printNotifs(7, "bytes==NULL\n");
				signalException(MemoryFault);
			}
			if(memUnit->isInStack(vAddr))
			{
				bytes[0] = (gpr[rt] >> 8) & 0xff;
				bytes[-1] = (gpr[rt]) & 0xff;
			}
			else
			{
				bytes[0] = (gpr[rt] >> 8) & 0xff;
				bytes[1] = (gpr[rt]) & 0xff;
			}
		}

		//MIPS I
		void sll(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLL\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			gpr[rd] = gpr[rt] << sa;
		}

		//MIPS I
		void sllv(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLLV\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
			}

			gpr[rd] = gpr[rt] << (gpr[rs] & 0x1f);
		}

		//MIPS I
		void slt(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLT\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLT %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}
			int64_t lhs = gpr[rs];
			int64_t rhs = gpr[rt];


			gpr[rd] = (lhs < rhs);
		}

		//MIPS I
		void slti(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLTI\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLTI %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
			}

			int64_t extended = (int64_t) immediate;
			int64_t lhs = gpr[rs];
			gpr[rt] = lhs < extended;
		}

		//MIPS I
		//@might not be how casting works.
		void sltui(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLTIU\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLTIU %s, %s, %x\n", getName(rt).c_str(), getName(rs).c_str(), immediate);
			}
			if (is64bit)
			{
				uint64_t extended = (uint64_t)immediate;
				uint64_t rhs = extended, 
						 lhs = gpr[rs];
				gpr[rt] = lhs < rhs;
			}
			else
			{
				uint32_t extended = (uint32_t)signedImmediate;
				uint64_t rhs = extended & 0xffffffff,
						 lhs = gpr[rs] & 0xffffffff;
				gpr[rt] = lhs < rhs;
			}
			
		}

		//MIPS I
		void sltu(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SLTU\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SLTU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			uint64_t a = gpr[rs], b = gpr[rt];
			gpr[rd] = a < b;
		}
		void sra(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SRA\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SRA %s, %s, %x\n", getName(rd).c_str(), getName(rt).c_str(), sa);
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
				printNotifs(4, "Invalid mips target for SRAV\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SRAV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
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
				printNotifs(4, "Invalid mips target for SRL\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SRL %s, %s, %d\n", getName(rd).c_str(), getName(rt).c_str(), sa);
			}

			uint32_t hold = gpr[rt];
			hold >>= sa;
			gpr[rd] = hold;
		}
		void srlv(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SRLV\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "SRLV %s, %s, %s\n", getName(rd).c_str(), getName(rt).c_str(), getName(rs).c_str());
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
				printNotifs(4, "Invalid mips target for SUB\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SUB %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for SUBU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SUBU %s, %s, %s\n", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for SW\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SW %s, %d(%s)\n", getName(rt).c_str(), immediate, getName(rs).c_str());
			}


		
			if (is64bit)
			{

			}
			else
			{
				int32_t offset = immediate;
				if(immediate & 3 > 0)
				{
					printNotifs(2, "Unaligned offset exception in SW\n");
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
					printNotifs(7, "bytes==NULL\n");
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

		//MIPS 1
		void swl(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SWL\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SWL %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			bool BigEndian = true;
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];

			int i = vAddr & 3, j = 24, k = 0;
			char *bytes = memUnit->getWriteAddresss(vAddr, 4-i, rs, 0);
			if(bytes == NULL)
				generallyPause();
				



			if(BigEndian)
			{
				if(memUnit->isInStack(vAddr))
				{
					while(i <= 3)
					{
						bytes[k] = (gpr[rt] >> j) & 0xff;
						i++;
						j -= 8;
						k--;
					}
				}
				else
				{
					while(i <= 3)
					{
						bytes[k] = (gpr[rt] >> j) & 0xff;
						i++;
						j -= 8;
						k++;
					}
				}
			}

			

		}
		void swr(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for SWR\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "SWR %s, %d(%s)\n", getName(rt).c_str(), signedImmediate, getName(rs).c_str());
			}

			bool BigEndian = true;
			uint64_t vAddr = (int64_t)signedImmediate + gpr[rs];

			int i = vAddr & 3, j = 0, k = 0;
			char *bytes = memUnit->getWriteAddresss(vAddr - i, i, rs, 0);
			if(bytes == NULL)
				generallyPause();
			if(memUnit->isInStack(vAddr))
				bytes -= i;
			else
				bytes += i;
			
			if(BigEndian)
			{
				if(memUnit->isInStack(vAddr))
				{
					while(i >= 0)
					{
						bytes[k] = (gpr[rt] >> j) & 0xff;
						i--;
						j += 8;
						k++;
					}
				}
				else
				{
					while(i >= 0)
					{
						printNotifs(7, "writing to %x", bytes+k);
						bytes[k] = (gpr[rt] >> j) & 0xff;
						i--;
						j += 8;
						k--;
					}
				}
			}
		}
		// MIPS 2
		void SYNC(uint32_t instruction)
		{
			unimplemented(instruction);
		}
		// MIPS 1
		void syscall(uint32_t instruction)
		{
			printNotifs(2, "Syscall performed. Unimplemented, generally!\n");
			printNotifs(7, "$v0 is 0x%lx.\n", gpr[2]);
			
			
		}
		// MIPS 2
		void teq(uint32_t instruction)
		{
			if (mipsTarget < 2)
			{
				printNotifs(4, "Invalid mips target for TEQ\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TEQ %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TEQI\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TEQI %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for TGE\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TGE %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TGEI\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TGEI %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for TGEIU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TGEIU %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for TGEU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TGEU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TLT\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TLT %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TLTI\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TLTI %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for TLTIU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TLTIU %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for TLTU\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TLTU %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TNE\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TNE %s, %s\n", getName(rs).c_str(), getName(rt).c_str());
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
				printNotifs(4, "Invalid mips target for TNEI\n");
			}

			if (debugPrint)
			{
				printNotifs(7, "TNEI %s, %d\n", getName(rs).c_str(), signedImmediate);
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
				printNotifs(4, "Invalid mips target for XOR\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "XOR %s, %s, %s", getName(rd).c_str(), getName(rs).c_str(), getName(rt).c_str());
			}

			gpr[rd] = gpr[rs] ^ gpr[rt];
		}

		//MIPS I
		void xori(uint32_t instruction)
		{
			if (mipsTarget < 1)
			{
				printNotifs(4, "Invalid mips target for XORI\n");
			}

			if (debugPrint)
			{

				printNotifs(7, "XORI %s, %s, %x", getName(rd).c_str(), getName(rs).c_str(), immediate);
			}

			gpr[rt] = gpr[rs] ^ immediate;
		}
		
		//@dontuse
		void signExtend(uint64_t* target, int length, int extension = -1)
		{

			uint64_t bitn = (uint64_t)1 << (length-1);
			uint64_t mask = ~(bitn - 1);
			printNotifs(7, "%lx\n", mask);
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
		//|v1:				     	|
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
			vector<int> validRegIndices = vector<int>();
			uint32_t *hold = (uint32_t *)calloc(sizeof(uint32_t), 2);
			printf("+-----------------------+---\n");
			bool isValidMemoryPtr;
			char *memTest;
			//Check registers for pointers
			for(int i=0;i<32;i++)
			{
				isValidMemoryPtr = memUnit->isInMemory(gpr[i], false);
				if(isValidMemoryPtr)
				{
					//printf("%s: 0x%x\n", getName(i).c_str(), gpr[i]);
					validRegIndices.push_back(i);
				}
			}

			for(int i=0;i<32;i++)
			{
				//Print the registers
				isValidMemoryPtr = memUnit->isInMemory(gpr[i], false);
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
					case 2: printf("| Stack Size: 0x%x", memUnit->stack.size());
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
					if(i-22 < validRegIndices.size())
					{
						uint32_t loadedWord;
						uint64_t vAddr = gpr[validRegIndices[i-22]];
						char *bytes = memUnit->getEffectiveAddress(vAddr, 4, 0, 0, true);
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
						
						if(i-22 >= 0 && i-22 < validRegIndices.size())
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

		void printNotifs(int logLevel = 7, const char* notif = "", ...)
		{
			// if (logLevel > 3) return;
			if(beQuietFlag)
				return;

			if(globalLogLevel > logLevel)
				return;
				
			// Print the logLevel before the notification
			switch (logLevel)
			{
				case 0:
					printf("[Emergency] ");
					break;
				case 1:
					printf("[Alert] ");
					break;
				case 2:
					printf("[Critical] ");
					break;
				case 3:
					printf("[Error] ");
					break;
				case 4:
					printf("[Warning] ");
					break;
				case 5:
					printf("[Notice] ");
					break;
				case 6:
					printf("[Informational] ");
					break;
				case 7:
					printf("[Debug] ");
					break;
			}

			// Format passed notif and subsequent arguments as printf would be formated
			va_list args;
			va_start (args, notif);
			vprintf (notif, args);
			va_end (args);
		}

		// pg. 40
		void runInstruction(uint32_t instruction)
		{
			printNotifs(6, "PC: [0x%lx], Instruction: [0x%08x]\n", pc, instruction);
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
	// Argparse testing
	argparse::ArgumentParser program("Electric Rock");

	program.add_argument("path")
		.help("path to the code to be tested by the emulator")
		.nargs(1);

	program.add_argument("--pcout")
		.help("dumps pcs to file pcpathing.txt")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("--reg")
		.help("dumps registers at every pc")
		.default_value(false)
		.implicit_value(true);
		
	program.add_argument("--quiet")
		.help("be quiet and avoid printing debug information")
		.default_value(false)
		.implicit_value(true);
	
	program.add_argument("--timer")
		.help("print execution time of instruction")
		.default_value(false)
		.implicit_value(true);
	
	program.add_argument("--batch")
		.help("run a directory of test cases")
		.default_value(false)
		.implicit_value(true);

	program.add_argument("--loglevel")
		.scan<'i', int>()
		.default_value(7)
		.help("Set the log level globally.");
		
	

	try 
	{
		program.parse_args(argn, args);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	auto code_path = program.get<std::string>("path");
	globalLogLevel = program.get<int>("loglevel");

	// Usage of optional args --pcout and --reg
	if (program["--pcout"] == true)
	{
		printf("Setting flag for pcout to true!\n");
		pcoutFlag = true;
		printf("pcout = %d\n", pcoutFlag);
	}

	if (program["--reg"] == true)
	{
		printf("Setting flag for regDump to true!\n");
		regDumpFlag = true;
		printf("regdumpflag = %d\n", regDumpFlag);
	}

	if (program["--quiet"] == true)
	{
		printf("Setting flag for beQuiet to true!\n");
		beQuietFlag = true;
	}

	if (program["--timer"] == true)
	{
		printf("Setting flag for timer and beQuiet to true!\n");
		timer = true;
		beQuietFlag = true;
		SHUT_UP = 1;
	}

	if (program["--batch"] == true)
	{
		std::cout << "Batch Directory: " << code_path << endl;
		batchMode = true;
	}

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

	if(batchMode)
	{
		// Create list of paths to test cases from batch directory
		for (auto& dirEntry : recursive_directory_iterator(code_path))
		{
     		std::cout << dirEntry << std::endl;
			std::string dirString = dirEntry.path().string();
			batchNames.push_back(dirString);
		}

		// Iterate over all dir entries
		for (auto& test_case: batchNames)
		{	
			// Check if exe
			// Setup BinaryView
			SetBundledPluginDirectory(GetPluginsDirectory());
			InitPlugins();
			printf("[Informational] Plugins initialized!\n");
			Ref<BinaryData> bd = new BinaryData(new FileMetadata(), test_case);
			Ref<BinaryView> bv = NULL;
			printf("[Informational] BV Instantiated!\n");
			fflush(stdout);
			for (auto type : BinaryViewType::GetViewTypes())
			{
				if (type->IsTypeValidForData(bd) && type->GetName() != "Raw")
				{
					bv = type->Create(bd);
					break;
				}
			}
			printf("[Informational] BVs initialized!\n");
			if (!bv || bv->GetTypeName() == "Raw")
			{
				fprintf(stderr, "Input file does not appear to be an exectuable\n");
				return -1;
			}
			printf("[Informational] Starting Analysis.\n");
			bv->UpdateAnalysisAndWait();
			printf("[Informational] Finished Analysis.\n");

			// Begin Emulation
			EmulatedCPU* electricrock = new EmulatedCPU(false, bv);


			// Run emulation
			electricrock->runEmulation(electricrock->startOfMain);
		}
		BNShutdown();
		return 0;
	}




	//
	// Check if file is accessable/exists
	FILE *fp;
	fp = fopen(args[1], "r");
	if (fp == NULL)
	{ 
		fprintf(stderr, "File does not exist"); 
		BNShutdown();
		return 0;
	}
	else
	{
		fclose(fp);
	}

	// Overwriting current file and checking opened correctly, before closing again
	PCPathFile = fopen("pcpathing.txt", "w+");		
	if (PCPathFile == NULL)
	{	
		fprintf(stderr, "File failed to open\n");
		return 0;
	}
			
	// In order to initiate the bundled plugins properly, the location
	// of where bundled plugins directory is must be set. Since
	// libbinaryninjacore is in the path get the path to it and use it to
	// determine the plugins directory
	
	SetBundledPluginDirectory(GetPluginsDirectory());
	InitPlugins();
	printf("[Informational] Plugins initialized!\n");
	Ref<BinaryData> bd = new BinaryData(new FileMetadata(), args[1]);
	Ref<BinaryView> bv = NULL;
	printf("[Informational] BV Instantiated!\n");
	fflush(stdout);
	for (auto type : BinaryViewType::GetViewTypes())
	{
		if (type->IsTypeValidForData(bd) && type->GetName() != "Raw")
		{
			bv = type->Create(bd);
			break;
		}
	}
	printf("[Informational] BVs initialized!\n");
	if (!bv || bv->GetTypeName() == "Raw")
	{
		fprintf(stderr, "Input file does not appear to be an exectuable\n");
		return -1;
	}
	printf("[Informational] Starting Analysis.\n");
	bv->UpdateAnalysisAndWait();
	printf("[Informational] Finished Analysis.\n");

	// Begin Emulation
	EmulatedCPU* electricrock = new EmulatedCPU(false, bv);
	
	//(uint32_t)bv->GetEntryPoint()
	//electricrock->startOfMain
	electricrock->runEmulation(electricrock->startOfMain);

	// Proper shutdown of core
	BNShutdown();

	return 0;	
}
