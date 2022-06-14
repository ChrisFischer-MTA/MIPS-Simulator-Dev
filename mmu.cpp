#pragma once
#include <iostream>
#include <stdio.h>
#include <cassert>
#include <string.h>
#include <vector>
#include <unordered_map>


#ifndef MMUCPP
#define MMUCPP 1
#endif

#include "binja.h"
#include "mmu.h"

#define BLOCKWIDTH 1024

using namespace std;

#define R 4
#define W 2
#define X 1

// Heap Constants
#define GUARD_PAGE_LENGTH 8 // In bytes.
#define MAX_HEAP_SIZE 0xfffff
#define GUARD_PAGE_VAL 0xfe
#define UNINITIALIZED_MEMORY_CONST 0b001
#define GUARDPAGE_MEMORY_CONST     0b010
#define FREED_MEMORY_CONS          0b100

//const short int IntegerOverflow = 1;
//const short int MemoryFault = 2;


section::section(int start, int length, char permissions, int width, char *name, segment parent)
{
	this->start = start;
	this->length = length;
	this->end = start + length;
	this->width = width;
	this->name = name;

	this->setPerms(permissions);

	int depth = (length / width) + 1;
	this->array = (char**)calloc(depth, sizeof(char*));
	for (int i = 0;i < depth;i++)
	{
		this->array[i] = (char*)calloc(width, sizeof(char));
	}
	this->initialized = (bool*)calloc(depth, sizeof(bool));
	this->writtenTo = (bool*)calloc(depth, sizeof(bool));
	ID = 0;

	this->parent = (segment *)malloc(sizeof(segment));
	*(this->parent) = parent;
	printf("0x%x segmentstart\n", this->parent->start);
}

typedef struct gap_
	{
		//left boundary, right boundary, index of section to the right
		int l;
		int r;
		int rightSection;
		int width() {return r - l;}
		vector<gap_> subGaps;
	} gap;

// Key is the index, Allocation contains the information.
struct Allocation
{
	// Metadata around the details of the allocation itself.
	unsigned int size; // Size of the allocation itself. Excluding the surrounding guard pages.
	bool isFree;	    // Tracking if we are freed.
	
	// Metadata around the details of what triggered the allocation.
	uint64_t pc;       // PC that called malloc or did the syscall.
};


void generallyPause();

class Heap
{
	private:
		uint64_t heapBase;
		uint64_t heapSize; // Total size of the heap, including guard pages.
		uint64_t maxHeapSize;
		std::vector<uint8_t> backingMemory;
		std::vector<uint8_t> initializedMemory;
		std::unordered_map<uint32_t, Allocation> allocInfo; // uint32_t here is the virtual address.
	
	public:
		
		// This function creates the heap. It trusts that it is given a valid base address
		// to draw the start of the heeap from. It also trusts that it is given a maximum
		// size for the heap from the emulator based off of layout of program memory.
		// Example call heap(0x5fffffff, 0xffffff);
		
		Heap(uint64_t HeapBase, uint64_t MaxHeapSize)
		{
			printf("Allocating heap!\n");
			this->heapBase = HeapBase;
			this->maxHeapSize = MaxHeapSize;
			this->heapSize = 0;
		}

		Heap()
		{
			return;
		}
		
		// This is essentially the wrapper for the memory.
		uint32_t allocMem(uint32_t size)
		{
			int i;

			// This is implementation specific, however, we return a NULL pntr.
			if(size == 0)
				return 0;
		
			if((this->heapSize + size) > this->maxHeapSize)
			{
				printf("[ERROR] Our heap has exceeded the maximum size.\n");
				return 0;
			}
		
			// Get the size of two guard pages.
			for(i = 0; i < GUARD_PAGE_LENGTH; i++)
			{
				printf("Guard Page 1: vaddr: [0x%lx] and index [%d].\n", this->heapBase + heapSize, i);
				this->backingMemory.push_back(GUARD_PAGE_VAL);
				this->initializedMemory.push_back(GUARDPAGE_MEMORY_CONST);
				this->heapSize++;
			}
			
			// Get the current value right now of the memory we're at.
			Allocation newAllocationInfo =  {size, 0, 0xfff}; // TODO: Implement PC
			allocInfo[(this->heapSize + this->heapBase)] = newAllocationInfo;
			
			uint32_t toReturn = (this->heapSize + this->heapBase);
			
			for(i = 0; i < size; i++)
			{
				printf("Normal Memory: vaddr: [0x%lx] and index [%d].\n", this->heapBase + heapSize, i);
				this->backingMemory.push_back(0);
				this->initializedMemory.push_back(UNINITIALIZED_MEMORY_CONST);
				this->heapSize++;
			}
			
			
			// Get the size of the second guard page.
			for(i = 0; i < GUARD_PAGE_LENGTH; i++)
			{
				printf("Guard Page 2: vaddr: [0x%lx] and index [%d].\n", this->heapBase + heapSize, i);
				this->backingMemory.push_back(GUARD_PAGE_VAL);
				this->initializedMemory.push_back(GUARDPAGE_MEMORY_CONST);
				this->heapSize++;
			}
			
			// This is unsafe to use after alloc.
			return (uint32_t) toReturn;
		}
		
		// this is our read function for the heap.
		// Restriction: This function assumes that you have validated that either the 
		// VirtualAddress is zero or resides in heap space (guard pages are OK).
		// We validate and return [vaddr, size)
		
		uint8_t* readHeapMemory(uint32_t vaddr, uint32_t size)
		{
			printf("[DEBUG] called readHeapMemory on vaddr:[0x%lx] with size:[%d] \n", vaddr, size);
			int i = 0;
			
			if(vaddr == 0)
			{
				printHeap("Read Heap Memory on a virtual address of NULL (NULL dereference).\n", vaddr);
				return 0;
			}
			
			if(size == 0)
			{
				printf("[ERROR] Read Heap Memory on a size of NULL.\n");
				return 0;
			}
			
			// First, let's ensure that the virtual address is valid.
			// Make sure we're accessing something larger then the heap base
			if(vaddr < (this->heapBase))
			{
				printf("[ERROR] Read Heap Memory on a virtual address of less then heap base.\n");
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			// TODO: This bounds check is insufficent. it checks to ensure that the first vaddr is in range of the heap.
			// But does not check the corrosponding claim to size. We should check this!
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				printf("[ERROR] Read Heap Memory on a virtual address of more then heap base.\n");
				return 0;
			}
			
			// We've validated the bounds. Let's grab memory and look for guard pages.
			// Why isn't this linear? In case of multiple heap allocations being used 
			// linearly (shouldn't EVER happen really)
			
			printf("Preamble. vaddr_base [0x%lx] and vaddr [0x%lx].\n", (vaddr-this->heapBase), (vaddr));
			for(i = (vaddr - this->heapBase); i < ((vaddr - this->heapBase) + size); i++ )
			{
				printf("Checking for guard pages and such at index i:[%d] and vaddr_base [0x%lx] and vaddr [0x%x].\n", i, (vaddr-this->heapBase), (i));
				if(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)
				{
					printHeap("Reading from Guard page memory! (Buffer Overflow!)\n", i, 0, 0);
					return NULL;	
				}
				
				if(this->initializedMemory[i] & UNINITIALIZED_MEMORY_CONST)
				{
					printHeap("Reading from uninitialized memory!\n", i, 0,0);
					return NULL;	
				}
				
				if(this->initializedMemory[i] & FREED_MEMORY_CONS)
				{
					printHeap("Reading from Free'd memory! (Use after free bug)\n", i, 0,0);
					return NULL;	
				}
				
			}
		
			return (uint8_t*) &backingMemory[(vaddr - this->heapBase)];	
		}
		
		// this is our read function for the heap.
		// Restriction: This function assumes that you have validated that either the 
		// VirtualAddress is zero or resides in heap space (guard pages are OK).
		// We validate and return [vaddr, size)
		
		uint8_t* writeHeapMemory(uint32_t vaddr, uint32_t size)
		{
			
			int i = 0;
			
			if(vaddr == 0)
			{
				printHeap("[ERROR] Write Heap Memory on a virtual address of NULL. (Improper Malloc/Null Dereference)\n", vaddr);
				return 0;
			}
			
			if(size == 0)
			{
				printHeap("[ERROR] Write Heap Memory on a size of NULL. (Improper Malloc/Null Dereference)\n", vaddr);
				return 0;
			}
			
			// First, let's ensure that the virtual address is valid.
			// Make sure we're accessing something larger then the heap base
			if(vaddr < (this->heapBase))
			{
				printHeap("[ERROR] Read Heap Memory on a virtual address of less then heap base.\n", vaddr);
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			// TODO: This bounds check is insufficent. it checks to ensure that the first vaddr is in range of the heap.
			// But does not check the corrosponding claim to size. We should check this!
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				printHeap("Read Heap Memory on a virtual address of more then heap base.\n", vaddr);
				return 0;
			}
			
			// We've validated the bounds. Let's grab memory and look for guard pages.
			// Why isn't this linear? In case of multiple heap allocations being used 
			// linearly (shouldn't EVER happen really)
			
			
			for(i = (vaddr - this->heapBase); i < ((vaddr - this->heapBase) + size); i++ )
			{
				printf("Checking for guard pages and such at index i:[%d] and vaddr_base [0x%lx] and vaddr [0x%x].\n", i, (vaddr-this->heapBase), (i));
				if(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)
				{
					printHeap("writing to guard page memory (Buffer Overflow)!\n", i, 0, 0);
					return NULL;	
				}
				
				if(this->initializedMemory[i] & FREED_MEMORY_CONS)
				{
					printHeap("reading from previously free'd memory (Use After Free)!\n", i, 0, 0);
					return NULL;	
				}
				
				this->initializedMemory[i] = 0;
			}
		
			return (uint8_t*) &backingMemory[(vaddr - this->heapBase)];	
		}
		
		uint8_t freeHeapMemory(uint32_t vaddr)
		{
			//printf("Freeing vaddr [0x%lx].\n", (vaddr-this->heapBase));
			// First thing we need to do is check to ensure we're in range.
			int index = vaddr - this->heapBase;
			int i = 0;
			
			if(vaddr == 0)
			{
				printHeap("Write Heap Memory on a virtual address of NULL (Improper MALLOC fail check).\n", vaddr);
				return 0;
			}
			
			// First, let's ensure that the virtual address is valid.
			// Make sure we're accessing something larger then the heap base
			if(vaddr < (this->heapBase))
			{
				printHeap("Read Heap Memory on a virtual address of less then heap base (Possible Heap Underflow!).\n", vaddr);
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				printHeap("Read Heap Memory on a virtual address of more then heap base. (Possible Heap Overflow!)\n", vaddr);
				return 0;
			}
			
			// Now, let's validate that the address is the start of a chunk.
			// This is a very hacky (and clever!) way to do so by taking advantage
			// of guard pages.
			
			if(this->initializedMemory[index] & FREED_MEMORY_CONS)
			{
				printHeap("We have found a dobule free!\n", index, 0, 0);
				return 0;
			}
			
			if(this->initializedMemory[index] & GUARDPAGE_MEMORY_CONST)
			{
				printHeap("We are attempting to free memory in a guard page!\n", index, 0,0);
				return 0;
			}
			
			if(!(this->initializedMemory[index-1] & GUARDPAGE_MEMORY_CONST))
			{
				printHeap("We are attempting to free memory that is not the first chunk of the allocation.\n", index-1, 0,0);
				return 0;
			}
			
			
			for(i = index; (!(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)); i++)
			{
				this->initializedMemory[i] = FREED_MEMORY_CONS;
			}
			
			return i;
			
		}

		bool isInHeap(uint32_t vaddr)
		{
			// Make sure we're accessing something larger then the heap base
			if(vaddr < (this->heapBase))
			{
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				return 0;
			}
			return 1;
		}
		
		// Stub
		void printHeap(char* warning, int triggeringVirtualAddress)
		{
			printHeap(warning, triggeringVirtualAddress, 0);
		}
		
		void printHeap(char* warning, int triggeringVirtualAddress, int _, int trash)
		{
			// Convert index to triggeringVirtualAddress 
			printHeap(warning, this->heapBase+triggeringVirtualAddress, 0);
		}
		
		void printHeap(char* warning, int triggeringVirtualAddress, int triggeringPC)
		{
		
			// ----------------------------
			// | 0x00 0x00 0x00 0x00 0x00 | 0-7
			// ----------------------------
			// |                          | 
			// |          BYTES           | 8-24
			// |                          |
			// ----------------------------
			// |       GUARD PAGE         | 25-31
			// ----------------------------
			int i = 0;
			
			printf("Possible anomolous behavior in the heap has been detected.\n");
			printf("Warning: %s\n", warning);
			printf("This warning was generated by PC 0x%x accessing 0x%08x in memory.\n", triggeringPC, triggeringVirtualAddress);
			
			printf("LEGEND (in order of priority):\n");
			printf("\x1b[31m\x1b[44m- GUARD PAGE\x1b[0m\n");
			printf("\x1b[33m- Free'd Memory\x1b[0m\n");
			printf("\x1b[32m- Unitinialized Memory\x1b[0m\n");
			printf("Note: Memory can have multiple attributes. In this debug view,");
			printf("it'll take the highest priority tag it has.\n\n\n");
			
			printf("           |----------------------------------------- |\n");
			for(i = 0 ; i < this->initializedMemory.size(); i++)
			{
				if(i%8 == 0)
				{
					if(i != 0)
						printf(" |\n0x%08x | ", this->heapBase+i);
					else
						printf("0x%08x | ", this->heapBase+i);
				}
				// Bold the bad byte
				if(this->heapBase+i == triggeringVirtualAddress)
					printf("\u001b[1m");
				
				if(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)
					printf("\x1b[31m\x1b[44m");
				else if(this->initializedMemory[i] & FREED_MEMORY_CONS)
					printf("\x1b[33m");
				else if(this->initializedMemory[i] & UNINITIALIZED_MEMORY_CONST)
					printf("\x1b[32m");
				
				
				printf("0x%02x ", backingMemory[i]);
				
				// Reset the colors.
				printf("\x1b[0m");
				
			}
			printf(" |\n           |----------------------------------------- |\n");
			
			while(true);
		}
		
};

class MMU
{
	public:
	
	vector<segment> segments;
	vector<section> allSections;
	vector<char> stack;
	Heap MMUHeap;
	uint64_t stackBase;
	uint64_t stackMaxLength;
	int alloLength;
	bool is64Bit;
	BinaryView* bv = NULL;

	MMU(bool is64bit, BinaryView* bc, uint64_t stackBase=0)
	{	
		


		// Assign the passed BinaryView into our class.
		bv = bc;
		
		
		
		// At some point need to check bv->platform->architecture for is64bit bool
		auto tmp = bv->GetSegments();
		segments = vector<segment>(tmp.size());
		
		this->is64Bit = is64bit;
		for (int i = 0;i < tmp.size(); i++)
		{
			segments[i].start = tmp[i]->GetStart();
			segments[i].end = tmp[i]->GetEnd();
			segments[i].length = tmp[i]->GetLength();
			int flags = tmp[i]->GetFlags();
			
			segments[i].setPerms(flags & 7);
			segments[i].permissions = flags;
			segments[i].ID = 0;

			segments[i].sections = new vector<section>();
		}
		auto sectionlist = bv->GetSections();
		allSections = vector<section>();
		char *secName;

		for (int i = 0;i < sectionlist.size();i++)
		{
			//find parent segment
			segment parent = segSearch(sectionlist[i]->GetStart());
			//create section object
			
			secName = (char *)calloc((sectionlist[i]->GetName().size())+1, sizeof(char));
			
			// Ironically, this is a heap overflow.
			
			strncpy(secName, sectionlist[i]->GetName().c_str(), sectionlist[i]->GetName().size());

			section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName, parent);
			
			allSections.push_back(token);
			//allSections[i].parent = (segment *)malloc(sizeof(segment));

			//section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName);
			
		}
		//sort sections
		this->secSort();

		for (int i = 0;i < allSections.size();i++)
		{
			//couple segment and section
			allSections[i].parent->sections->push_back(allSections[i]);
		}

		//Allocate the stack
		this->stack = vector<char>(5);
		gap bigGap = getLargestGap();
		this->stackBase = bigGap.r - 0xf;
		this->stackMaxLength = bigGap.r - 0xf - bigGap.l;
		//Fill with fuzzing data
		
		printf("0x%llx, 0x%llx\n", stackBase-20, this->stackBase);
		stackWrite(this->stackBase - 20, "static_hello_world", 19); //18 chars +1\0
		int filenameptr = this->stackBase-20;
		char data[4];
		for(int i=0;i<4;i++)
			data[i] = (filenameptr >> i*8) & 0xff;
		stackWrite(this->stackBase - 24, data, 4);
		for(int i=0;i<3;i++)
			data[i] = 0;
		data[3] = 1;
		stackWrite(this->stackBase - 28, data, 4);
		this->stack.resize(30);

		//Allocate the heap
		bool *excluded = (bool *)calloc(allSections.size() + 1, sizeof(bool));
		excluded[bigGap.rightSection] = true;
		gap secondBiggestGap = getLargestGap(excluded);
		//Left and right bounds of SBG padded by 8 bytes
		MMUHeap = Heap(secondBiggestGap.l + 8, secondBiggestGap.r - secondBiggestGap.r -16);

		printSections();
	}

	
	void secSort()
	{
		vector<section> sorted = vector<section>();
		bool *disabled = (bool *)calloc(allSections.size(), sizeof(bool));
		section least;
		int leastIndex;

		fflush(stdout);
		for (int j = 0;j < allSections.size();j++)
		{
			
			least = section(0xffffffffffffffff);
			
			for (int i = 0;i < allSections.size();i++)
			{
				if (allSections[i] < least && !disabled[i])
				{
					least = allSections[i];
					leastIndex = i;
				}
			}
			sorted.push_back(least);
			disabled[leastIndex] = true;

		}
		for (int i = 1;i < allSections.size();i++)
		{
			allSections = sorted;
		}
		free(disabled);
		return;
			
	}
	

	
	segment segSearch(uint64_t index)
	{
		for (int i = 0;i < segments.size();i++)
		{
			if (index <= segments[i].end && index >= segments[i].start)
			{
				return segments[i];
			}
		}
		return segment();
	}

	//calculates the largest gap between sections
	//returns the left and right border of the gap
	gap getLargestGap(bool * excluded = NULL)
	{
		
		if(excluded == NULL)
		{
			excluded = (bool *) calloc(allSections.size(), sizeof(bool));
		}
		int l=0,r=0;
		int maxl=0,maxr=0, maxWidth = 0, maxI = 0;
		
		for(int i=0;i<allSections.size();i++)
		{
			
			r = allSections[i].start;
			if(r-l > maxWidth && !excluded[i])
			{
				maxWidth = r - l;
				maxr = r;
				maxl = l;
				maxI = i;
			}

			l = allSections[i].end;
			
		}
		fflush(stdout);
		r = 0x0fffffff;
		if(r - l > maxWidth && !excluded[allSections.size()])
		{
			maxWidth = r - l;
			maxr = r;
			maxl = l;
			maxI = allSections.size();
		}
		gap out;

		out.l = l;
		out.r = r;
		out.rightSection = maxI;
		fflush(stdout);
		free(excluded);
		return out;
	}

	//Writes to the stack in increasing memory index (the emulated stack is backwards)
	//^No it doesn't, that's dumb
	//Writes to the stack in increasing memory address. 
	void stackWrite(uint64_t address, char *data, int length)
	{
		fflush(stdout);
		if(stackBase - address + 1 > stack.size())
		{
			stack.resize(stackBase - address + 1);
		}
		int startingIndex = stackBase-address;
		for(int i = startingIndex;i > startingIndex - length; i--)
		{
			stack[i] = data[i - startingIndex + length - 1];
		}
		return;
	}
	
	bool isInStack(uint64_t address)
	{
		if(address <= stackBase && address > stackBase - stackMaxLength)
		{
			return true;
		}
		return false;
	}
	
	bool isInBinary(uint64_t address)
	{
		//For Binja binary accesses	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < segments[i].sections->size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					return true;
				}
			}
		}
		return false;
	}
	
		
	//Returns a pointer to a stream of bytes that can be read as necessary
	//the stream of bytes is either the mmu memory segments (ideally) or a reconstructed short block
	//of unwritable straight from binja
	//numBytes doesn't make it grab n bytes, it's just there to make sure a block boundary isn't being crossed
	//address: ptr to virtual memory
	//gpr: which register is used to access memory
	//contents: contents of gpr
	char * getEffectiveAddress(uint64_t address, int numBytes, int gpr, uint64_t contents = 0)
	{
		//For Stack pointer access
		if(isInStack(address))
		{
			printf("Searching the stack!\n");
			if(contents < stackBase - stack.size())
			{
				stack.resize(contents - stackBase + 1);
			}
			if(address < contents)
			{
				stack.resize(stackBase - address + 1);
			}

			uint64_t stackOffset = stackBase - address;
			printf("stackData: %x\n", stack.data());
			return stack.data() + stackOffset;
		}
		//For Heap Pointer access
		if(MMUHeap.isInHeap(address))
		{
			uint8_t *out = MMUHeap.readHeapMemory(address, numBytes);
			if(out == NULL)
			{
				printf("Fail heap write\n");
			}
			return out;
		}
		//For Binja binary accesses	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < segments[i].sections->size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					
					fflush(stdout);
					//Is it readable?
					if (token.readable)
					{
						//Is it writable? Exists to avoid loading unwritable data when unnecessary
						if (token.writable)
						{
							
							//Yes, could have a dirty state in memory:
							//block access arithmetic, token[depth][blockOffset] should be starting point
							fflush(stdout);
							uint64_t offset = address - token.start;
							uint64_t depth = (int)(offset / token.width);
							uint64_t blockOffset = offset % token.width;
							fflush(stdout);
							//Not even initialized, pull and return 
							if (!token.initialized[depth])
							{
								//token[depth] = binja.access(token.start + depth*width, width);
								if (bv->Read(token.array[depth], address - blockOffset, token.width) != token.width)
									1 + 1;
								token.initialized[depth] = true;

							}
							printf("[FLUSH] 3\n");
							if (blockOffset + numBytes > token.width)
							{
								printf("something weird happened\n");
								generallyPause();
							}
							printf("[FLUSH] %x, %x, %x, %x, %x\n", address, token.start, offset, depth, blockOffset);

							char * testing = token.array[depth];
							for(int i=0;i<16;i++)
							{
								printf("%x", testing[i]);
							}
							printf("\n");

							return token.array[depth] + blockOffset;


						}
						//should just return a pointer to a numBytes-length array of the requested bytes
						else
						{
							char* out = (char*)(calloc(numBytes, sizeof(char)));

							//if(num bytes read by Read() != numBytes)
							if (bv->Read(out, address, numBytes) != numBytes)
								generallyPause();
							return out;
						}
					}
				}
			}
		}
		return NULL;
	}

	char *getWriteAddresss(uint64_t address, int numBytes, int gpr, uint64_t contents = 0)
	{
		printf("address: %lx, gpr: %d\n", address, gpr);
			fflush(stdout);
		//For Stack pointer access
		if(address < stackBase && address > stackBase - stackMaxLength)
		{
			printf("Searching the stack!\n");
			printf("%lx\n", address);
			fflush(stdout);
			if(contents < stackBase - stack.size())
			{
				stack.resize(stackBase - contents + 1);
			}
			if(address < contents)
			{
				stack.resize(stackBase - address + 1);
			}

			uint64_t stackOffset = stackBase - address;
			printf("stackData: %x\n", stack.data());
			fflush(stdout);
			return stack.data() + stackOffset;
		}

		//For Heap Pointer access
		if(MMUHeap.isInHeap(address))
		{
			uint8_t *out = MMUHeap.writeHeapMemory(address, numBytes);
			if(out == NULL)
			{
				printf("Fail heap write\n");
			}
			return out;
		}
		//For Binja binary accesses	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < segments[i].sections->size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					fflush(stdout);
					//Is it readable?
					if (token.readable)
					{
						//Is it writable?
						if (token.writable)
						{
							//Yes, could have a dirty state in memory:
							//block access arithmetic, token[depth][blockOffset] should be starting point
							fflush(stdout);
							uint64_t offset = address - token.start;
							uint64_t depth = (int)(offset / token.width);
							uint64_t blockOffset = offset % token.width;
							fflush(stdout);
							//Not even initialized, pull and return 
							if (!token.initialized[depth])
							{
								//token[depth] = binja.access(token.start + depth*width, width);
								//this if is nullified because when bv reads the right edge of a section it doesn't read a full block of data
								if (bv->Read(token.array[depth], address - blockOffset, token.width) != token.width)
									1 + 1;
								token.initialized[depth] = true;

							}
							printf("[FLUSH] 3\n");
							if (blockOffset + numBytes > token.width)
							{
								printf("something weird happened\n");
								generallyPause();
							}
							printf("[FLUSH] %x, %x, %x, %x, %x\n", address, token.start, offset, depth, blockOffset);

							char * testing = token.array[depth];
							for(int i=0;i<16;i++)
							{
								printf("%x", testing[i]);
							}
							printf("\n");

							return token.array[depth] + blockOffset;
						}
						else
						{
							printf("Unwritable Token On Write Exception");
							return NULL;
						}
					}
				}
			}
		}
	}
	
	/*int writeToBytes(char *data, int length)
	{
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < allSections.size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					if (token.writable)
					{
						//Yes, could have a dirty state in memory:
						//block access arithmetic, token[depth][blockOffset] should be starting point
						int offset = address - token.start;
						int depth = offset / token.width;
						int blockOffset = depth % token.width;
					{

				}
			}
		}
	}*/



	unsigned char* getBytes(int addr)
	{	
		if(bv == NULL)
		{
			printf("Binary View is not initalized in this getBytes call!\n");
		}

		// Allocate bytes and buffer
		size_t numBytesRead;
		
		// Create location for bytes to be read in to
		unsigned char* bytes = (unsigned char*) malloc(sizeof(char) * 4);

		// Check for a null pntr
		if(bytes == NULL)
		{
			printf("Insufficent space on heap for a malloc.\n");
		}

		numBytesRead = bv->Read(bytes, addr, 4);
		
		return bytes;
	}

	void store(uint64_t address, void* data, int datalength)
	{

	}
	
	// This function uses the Binary Ninja API to validate that the address we're about to jump to 
	// is not an external symbol that would need to be resolved by the linker.
	bool isAddrExtern(uint64_t address)
	{
		printf("Checking address [0x%x]!\n", address);
		/*
		// Address is currently for puts() in our a.out
		//address=0x410864;
		printf("Validating the PC of 0x%x\n", address);
		vector<Ref<Section>> sections = bv->GetSectionsAt(address);

		Symbol* s = bv->GetSymbolByAddress(address);
        printf("**%s**\n", s->GetFullName().c_str());
		printf("%d\n", bv->IsOffsetExternSemantics(address));
		
		// Check for sections
		if(sections.size() == 0)
		{
			printf("Invalid section size!\n");
			return false;
		}
		
		printf("GetType: [0x%x] and GetName: [%s]\n", sections[0]->GetType(), sections[0]->GetName().c_str());
		//while(true);
		*/
		
		return bv->IsOffsetExternSemantics(address);
	}

	void generallyPause()
	{
		while (1)
		{
			// do nothing
		}
	}
	void printSections()
	{
		printf("Sections:\n");
		for(int i=0;i<allSections.size();i++)
		{
			printf("Start: 0x%lx, Size: %lx, Name %s\n", allSections[i].start, allSections[i].end - allSections[i].start, allSections[i].name);
		}
	}
	
};
