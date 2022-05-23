#include "mmu.h"
#include <iostream>
#include <cassert>
#include "../emulator.cpp"



using namespace std;

#define R 4
#define W 2
#define X 1

class MMU
{
public:
	page BasedPageTable;
	ArrayList<allocation> allocations;
	EmulatedCPU *emulator;
	int alloLength;
	bool is64Bit;
	MMU(EmulatedCPU *electrickRock)
	{
		allocations = ArrayList<allocation>(20);
		emulator = electrickRock;
	}

	
	//Just get the initial pointer, emulator draws line
	//numBytes doesn't make it grab n bytes, it's just there to make sure a block boundary isn't being crossed
	char * getEffectiveAddress(uint64_t address, int numBytes)
	{
		//For each section[sic],
		for (int i = 0;i < allocations.size;i++)
		{
			allocation token = allocations[i];
			//Is it valid? (within bounds)
			if (address >= token.start && address <= token.end)
			{
				//Is it readable?
				if (token.readable)
				{
					//Is it writable? Exists to avoid loading unwritable data when unnecessary
					if (token.writable)
					{
						//Yes, could have a dirty state in memory:
						//block access arithmetic, token[depth][blockOffset] should be starting point
						int offset = address - token.start;
						int depth = offset / token.width;
						int blockOffset = depth % token.width;

						//Not even initialized, pull and return 
						if(!token.initialized[depth])
						{
							//token[depth] = binja.access(token.start + depth*width, width);
						}
						if (blockOffset + numBytes > token.width)
						{
							printf("something weird happened\n");
							emulator->generallyPause();
						}

						return token.array[depth] + blockOffset;

						
					}
					//should just return a pointer to a numBytes-length array of the requested bytes
					else
					{
						//binja.access(address, 4);
					}
				}
			}
			else
			{
				emulator->signalException(MemoryFault);
			}

		}
	}
	void store(uint64_t address, void* data, int datalength)
	{

	}
};
