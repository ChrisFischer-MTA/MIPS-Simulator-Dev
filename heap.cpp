
/*
Global TODOs for the heap.
TODO: Implement free checks when writing and reading.
TODO: Implement PC metadata.
TODO: Implement use of metadata period.
TODO: 
*/


#include <iostream>
#include <stdio.h>
#include <cassert>
#include <string.h>
#include <vector>
#include <unordered_map>



// Heap Constants
#define GUARD_PAGE_LENGTH 8 // In bytes.
#define MAX_HEAP_SIZE 0xfffff
#define GUARD_PAGE_VAL 0xfe
#define UNINITIALIZED_MEMORY_CONST 0b001
#define GUARDPAGE_MEMORY_CONST     0b010
#define FREED_MEMORY_CONS          0b100



// Key is the index, Allocation contains the information.
struct Allocation
{
	// Metadata around the details of the allocation itself.
	unsigned int size; // Size of the allocation itself. Excluding the surrounding guard pages.
	bool isFree;	    // Tracking if we are freed.
	
	// Metadata around the details of what triggered the allocation.
	uint64_t pc;       // PC that called malloc or did the syscall.
};


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
		
		// This is essentially the wrapper for the memory.
		uint32_t allocMem(uint32_t size)
		{
			// This is implementation specific, however, we return a NULL pntr.
			if(size == 0)
				return 0;
		
			if((this->heapSize + size) > this->maxHeapSize)
			{
				printf("[ERROR] Our heap has exceeded the maximum size.\n");
				return 0;
			}
		
			int i = 0;
			
			// Get the size of two guard pages.
			for(i = 0; i < GUARD_PAGE_LENGTH; i++)
			{
				int totalHeap = this->heapBase + heapSize;
				printf("Guard Page 1: vaddr: [0x%x] and index [%d].\n", totalHeap, i);
				this->backingMemory.push_back(GUARD_PAGE_VAL);
				this->initializedMemory.push_back(GUARDPAGE_MEMORY_CONST);
				this->heapSize++;
			}
			
			// Get the current value right now of the memory we're at.
			Allocation newAllocationInfo =  {size, 0, 0xfff}; // TODO: Implement PC
			allocInfo[(this->heapSize + this->heapBase)] = newAllocationInfo;
			
			uint32_t toReturn = (this->heapSize + this->heapBase);
			
			for(; i < size + GUARD_PAGE_LENGTH; i++)
			{
				int totalHeap = this->heapBase + heapSize;
				printf("Normal Memory: vaddr: [0x%x] and index [%d].\n", totalHeap, i);
				this->backingMemory.push_back(0);
				this->initializedMemory.push_back(UNINITIALIZED_MEMORY_CONST);
				this->heapSize++;
			}
			
			
			// Get the size of the second guard page.
			for(; i < GUARD_PAGE_LENGTH + size + GUARD_PAGE_LENGTH; i++)
			{
				int totalHeap = this->heapBase + heapSize;
				printf("Guard Page 2: vaddr: [0x%x] and index [%d].\n", totalHeap, i);
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
				printHeap("[ERROR] Read Heap Memory on a virtual address of NULL (NULL dereference).\n", vaddr);
				return 0;
			}
			
			if(size == 0)
			{
				printHeap("[ERROR] Read Heap Memory on a size of NULL.\n", vaddr);
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
				printHeap("[ERROR] Read Heap Memory on a virtual address of more then heap base.\n", vaddr);
				return 0;
			}
			
			// We've validated the bounds. Let's grab memory and look for guard pages.
			// Why isn't this linear? In case of multiple heap allocations being used 
			// linearly (shouldn't EVER happen really)
			
			printf("Preamble. vaddr_base [0x%lx] and vaddr [0x%lx].\n", (vaddr-this->heapBase), (vaddr));
			for(i = (vaddr - this->heapBase); i < ((vaddr - this->heapBase) + size); i++ )
			{
				int tmp = vaddr-this->heapBase;
				printf("Checking for guard pages and such at index i:[%d] and vaddr_base [0x%lx] and vaddr [0x%x].\n", i, tmp, i);
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
				printHeap("[ERROR] Write Heap Memory on a virtual address of less then heap base.\n", vaddr);
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			// TODO: This bounds check is insufficent. it checks to ensure that the first vaddr is in range of the heap.
			// But does not check the corrosponding claim to size. We should check this! (vaddr+size)
			if((vaddr+size) >= (this->heapBase + this->heapSize))
			{
				printHeap("[ERROR] Write Heap Memory on a virtual address of more then heap base.\n", vaddr);
				return 0;
			}
			
			// We've validated the bounds. Let's grab memory and look for guard pages.
			// Why isn't this linear? In case of multiple heap allocations being used 
			// linearly (shouldn't EVER happen really)
			
			
			for(i = (vaddr - this->heapBase); i < ((vaddr - this->heapBase) + size); i++ )
			{
				int tmp = vaddr-this->heapBase;
				printf("Checking for guard pages and such at index i:[%d] and vaddr_base [0x%lx] and vaddr [0x%x].\n", i, tmp, i);

				if(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)
				{
					printHeap("[ERROR] Writing to guard page memory (Buffer Overflow)!\n", i, 0, 0);
					return NULL;	
				}
				
				if(this->initializedMemory[i] & FREED_MEMORY_CONS)
				{
					printHeap("[ERROR] Reading from previously free'd memory (Use After Free)!\n", i, 0, 0);
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
				printHeap("[ERROR] Free Heap Memory on a virtual address of NULL (Improper MALLOC fail check).\n", vaddr);
				return 0;
			}
			
			// First, let's ensure that the virtual address is valid.
			// Make sure we're accessing something larger then the heap base
			if(vaddr < (this->heapBase))
			{
				printHeap("[ERROR] Free Heap Memory on a virtual address of less then heap base (Possible Heap Underflow!).\n", vaddr);
				return 0;
			}
			
			// Make sure we're accessing something less then the heap size.
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				printHeap("[ERROR] Free Heap Memory on a virtual address of more then heap base. (Possible Heap Overflow!)\n", vaddr);
				return 0;
			}
			
			// Now, let's validate that the address is the start of a chunk.
			// This is a very hacky (and clever!) way to do so by taking advantage
			// of guard pages.
			
			if(this->initializedMemory[index] & FREED_MEMORY_CONS)
			{
				printHeap("[ERROR] We have found a double free!\n", index, 0, 0);
				return 0;
			}
			
			if(this->initializedMemory[index] & GUARDPAGE_MEMORY_CONST)
			{
				printHeap("[ERROR] We are attempting to free memory in a guard page!\n", index, 0,0);
				return 0;
			}
			
			if(!(this->initializedMemory[index-1] & GUARDPAGE_MEMORY_CONST))
			{
				printHeap("[ERROR] We are attempting to free memory that is not the first chunk of the allocation.\n", index-1, 0,0);
				return 0;
			}
			
			
			for(i = index; (!(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)); i++)
			{
				this->initializedMemory[i] = FREED_MEMORY_CONS;
			}
			
			return i;
			
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
			
			printf("\n----------------------------------------------------------------\n");
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


int main(int argn, char** args)
{
	int HEAP_BASE = 0x60000000;
	int i;

	// Demonstration on how the heap works.
	Heap heapyboi = Heap(HEAP_BASE, 0xffffff);
	uint32_t pntr;

	/*
	// Case 1: Allocate too much memory (Should fail)
	pntr = heapyboi.allocMem(0xffffffff);
	*/
	
	// Allocate some memory, get a virtual address.
	pntr = heapyboi.allocMem(24);
	printf("Memory Base Addr: %0x\n", pntr);
	
	// Current state of the heap (allegedly)
	// Each block is 8 bytes
	// ----------------------------
	// |       GUARD PAGE         | 0-7
	// ----------------------------
	// |                          | 
	// |          BYTES           | 8-31
	// |                          |
	// ----------------------------
	// |       GUARD PAGE         | 32-39
	// ----------------------------
	
	
	
	// Turn the virtual address into a real one via writing to it
	//uint8_t* array = heapyboi.writeHeapMemory(pntr, 6);
	
	// Writes 'hello' to memory
	uint8_t *array = NULL;
	array = heapyboi.writeHeapMemory(pntr+0, 1);
	array[0] = 'h';
	array = heapyboi.writeHeapMemory(pntr+1, 1);
	array[0] = 'e';
	array = heapyboi.writeHeapMemory(pntr+2, 1);
	array[0] = 'l';
	array = heapyboi.writeHeapMemory(pntr+3, 1);
	array[0] = 'l';
	array = heapyboi.writeHeapMemory(pntr+4, 1);
	array[0] = 'o';
	array = heapyboi.writeHeapMemory(pntr+5, 1);
	array[0] = '\0';

	// Read 'hello'
	array = heapyboi.readHeapMemory(pntr, 6);
	printf("test print 1: %s\n", array);
	
	/*
	// Case 2: Buffer Overflow into Guard Page (index 32)
	array = heapyboi.writeHeapMemory(pntr+19, 1);
	array[0] = 'w';
	array = heapyboi.writeHeapMemory(pntr+20, 1);
	array[0] = 'o';
	array = heapyboi.writeHeapMemory(pntr+21, 1);
	array[0] = 'r';
	array = heapyboi.writeHeapMemory(pntr+22, 1);
	array[0] = 'l';
	array = heapyboi.writeHeapMemory(pntr+23, 1);
	array[0] = 'd';
	array = heapyboi.writeHeapMemory(pntr+24, 1);
	array[0] = '\0';
	
	// Read 'world'
	array = heapyboi.readHeapMemory(pntr+19, 6);
	printf("test print 2: %s\n", array);

	// Case 3: Double Free (Should throw error)
	heapyboi.freeHeapMemory(pntr);
	heapyboi.freeHeapMemory(pntr);

	// Case 4: Use after Free (Should throw error)
	heapyboi.freeHeapMemory(pntr);
	array = heapyboi.readHeapMemory(pntr+0, 6);
	printf("test print 2: %s\n", array);

	// Case 5: NULL values passed to readHeap and writeHeap
	array = heapyboi.readHeapMemory(pntr+0, 0);
	array = heapyboi.readHeapMemory(0, 6);
	array = heapyboi.writeHeapMemory(0, 1);
	array[0] = 'h';
	array = heapyboi.writeHeapMemory(pntr+1, 0);
	array[0] = 'i';

	// Case 6: Heap Underflow Check
	array = heapyboi.readHeapMemory(pntr-20, 1);

	// Case 7: Heap Overflow Check
	array = heapyboi.readHeapMemory(pntr+40, 1);

	// Case 8: Free Guard Page Memory
	heapyboi.freeHeapMemory(pntr-1);

	// Case 9: Free Memory not in the Chunk
	heapyboi.freeHeapMemory(pntr+1);

	// Case 10: Alloc lots of times
	heapyboi.freeHeapMemory(pntr);
	uint32_t pntr1;
	uint32_t pntr2;
	pntr1 = heapyboi.allocMem(32);
	pntr2 = heapyboi.allocMem(40); 
	heapyboi.freeHeapMemory(pntr1);
	heapyboi.freeHeapMemory(pntr2);
	*/
	// Case 11: 


	// TODO: Fix warning generator to have accurate PC and addr references

	// Free memory previously allocated
	heapyboi.freeHeapMemory(pntr);
	array = heapyboi.writeHeapMemory(pntr+0, 1);
	
	//heapyboi.printHeap("Heap Dump",0,0);

}
