
/*

Note: This is a draft version of the heap implementation that we will be implementing into MMU.CPP

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
				printf("[ERROR] Read Heap Memory on a virtual address of NULL.\n");
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
					printf("reading from Guard page memory!\n");
					return NULL;	
				}
				
				if(this->initializedMemory[i] & UNINITIALIZED_MEMORY_CONST)
				{
					printf("reading from Uninit memory!\n");
					return NULL;	
				}
				
				if(this->initializedMemory[i] & FREED_MEMORY_CONS)
				{
					printf("reading from Free'd memory!\n");
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
				printf("[ERROR] Write Heap Memory on a virtual address of NULL.\n");
				return 0;
			}
			
			if(size == 0)
			{
				printf("[ERROR] Write Heap Memory on a size of NULL.\n");
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
			
			
			for(i = (vaddr - this->heapBase); i < ((vaddr - this->heapBase) + size); i++ )
			{
				printf("Checking for guard pages and such at index i:[%d] and vaddr_base [0x%lx] and vaddr [0x%x].\n", i, (vaddr-this->heapBase), (i));
				if(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)
				{
					printf("writing to guard page memory!\n");
					return NULL;	
				}
				
				if(this->initializedMemory[i] & FREED_MEMORY_CONS)
				{
					printf("reading from Free'd memory!\n");
					return NULL;	
				}
				
				this->initializedMemory[i] = 0;
			}
		
			return (uint8_t*) &backingMemory[(vaddr - this->heapBase)];	
		}
		
		uint8_t freeHeapMemory(uint32_t vaddr)
		{
			printf("Freeing vaddr [0x%lx].\n", (vaddr-this->heapBase));
			// First thing we need to do is check to ensure we're in range.
			int index = vaddr - this->heapBase;
			int i = 0;
			
			if(vaddr == 0)
			{
				printf("[ERROR] Write Heap Memory on a virtual address of NULL.\n");
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
			if(vaddr >= (this->heapBase + this->heapSize))
			{
				printf("[ERROR] Read Heap Memory on a virtual address of more then heap base.\n");
				return 0;
			}
			
			// Now, let's validate that the address is the start of a chunk.
			// This is a very hacky (and clever!) way to do so by taking advantage
			// of guard pages.
			
			if(this->initializedMemory[index] & FREED_MEMORY_CONS)
			{
				printf("We have found a dobule free!\n");
				return 0;
			}
			
			if(this->initializedMemory[index] & GUARDPAGE_MEMORY_CONST)
			{
				printf("We are attempting to free memory in a guard page!\n");
				return 0;
			}
			
			if(!(this->initializedMemory[index-1] & GUARDPAGE_MEMORY_CONST))
			{
				printf("We are attempting to free memory that is not the first chunk of the allocation.\n");
				return 0;
			}
			
			
			for(i = index; (!(this->initializedMemory[i] & GUARDPAGE_MEMORY_CONST)); i++)
			{
				this->initializedMemory[i] = FREED_MEMORY_CONS;
			}
			printf("just freed [%d] bytes!\n", i-index);
			return i;
			
		}
		
		// Stub
		void printHeap();
		
};


int main(int argn, char** args)
{
	int HEAP_BASE = 0x5fffffff;
	int i;
	
	
	
	// Demonstration on how the heap works.
	Heap heapyboi = Heap(HEAP_BASE, 0xffffff);
	uint32_t pntr;
	
	
	// Allocate some memory, get a virtual address.
	pntr = heapyboi.allocMem(24);
	printf("Addr: %0x\n", pntr);
	
	// Current state of the heap (allegedly)
	// Each block is 8 bytes
	// ----------------------------
	// |       GUARD PAGE         | 0-7
	// ----------------------------
	// |                          | 
	// |          BYTES           | 8-24
	// |                          |
	// ----------------------------
	// |       GUARD PAGE         | 25-31
	// ----------------------------
	
	
	
	// Turn the virtual address into a real one via writing to it
	printf("Addr: %0x\n", heapyboi.writeHeapMemory(pntr, 6));
	uint8_t* array = heapyboi.writeHeapMemory(pntr, 6);
	
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
	
	
	
	array = heapyboi.readHeapMemory(pntr, 6);
	printf("test print: %s\n", array);
	
	// This throws an error for uninit memory usage
	array = heapyboi.readHeapMemory(pntr, 7);
	
	heapyboi.freeHeapMemory(pntr);
	heapyboi.readHeapMemory(pntr, 6);
	
	
	
	
	
	
	/*
	// Should fail. We are reading 0 bytes.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+0, 0);
	
	// Should fail. We are reading 1 byte, but that byte is in guard page 1.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+1, 1);
	
	// Should fail. We are reading 1 byte, but that byte is in guard page 1.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+7, 1);
	
	// Should fail. We are reading a guard page.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE, 8);
	
	
	// Should fail. We are reading a guard page and some valid memory.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE, 9);
	
	
	// Should win.
	printf("Should win!\n\n\n");
	heapyboi.writeHeapMemory(HEAP_BASE+8, 1);
	heapyboi.readHeapMemory(HEAP_BASE+8, 1);
	
	// Should win.
	heapyboi.writeHeapMemory(HEAP_BASE+8, 23);
	
	printf("Should win!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+8, 23);
	
	
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+8, 24);
	
	// Should fail. We are reading into a guard page.
	printf("Should fail!\n\n\n");
	heapyboi.readHeapMemory(HEAP_BASE+8, 25);
	*/
	
	
	/*
	// Tests for freeing memory.
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+0);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+7);
	
	printf("Should Win!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+8);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+9);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+24);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+25);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+32);
	
	printf("Should fail!\n\n\n");
	heapyboi.freeHeapMemory(HEAP_BASE+48);
	
	*/
	
	/*heapyboi.allocMem(0x24);
	heapyboi.allocMem(0x24);
	heapyboi.allocMem(0x24);*/
}

