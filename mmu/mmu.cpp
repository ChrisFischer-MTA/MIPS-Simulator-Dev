#include "mmu.h"
#include <iostream>
#include <cassert>



using namespace std;

#define R 4
#define W 2
#define X 1

class MMU
{
public:
	page BasedPageTable;
	allocation* allocations;
	int alloLength;
	bool is64Bit;
	int depth;
	MMU(page BPT, allocation* allos)
	{
		this->BasedPageTable = BPT;
		this->allocations = allos;
		this->depth = 1;
	}
	MMU()
	{
		this->BasedPageTable = page(true, 0, 0);
		this->BasedPageTable.depth = 0;
		this->depth = 0;
		this->allocations = (allocation *)calloc(100, sizeof(allocation));
		for (int i = 0;i < 512;i++)
		{
			BasedPageTable.table[i] = page(false, (uint64_t)i, 42);
		}
		this->depth = 1;
	}

	page getPage(uint64_t virtualAdd)
	{
		int localDepth = 0;
		page pagePtr = BasedPageTable;
		while (localDepth > depth)
		{
			int addPtr = virtualAdd & 0x1ff;
			pagePtr = pagePtr.table[addPtr];
			addPtr >>= 9;
			localDepth = pagePtr.depth;
		}
		return pagePtr;
	}

	//allocation's primary goal is to find an unallocated section of contiguous memory and declare it allocated
	//and attach the allocation tag to the mmu object in some way
	//the allocator must find a contiguous block that is on one page if the allocation is less than the size of one page
	//the allocator must find pages on a single section of a level if the allocation is less than the size of that section
	//otherwise go wet go wild just reduce the impact as much as possible
	void allocate(int length, uint64_t ID, char permissions)
	{
		
		allocation token = allocation(0, 0, length, permissions, ID);
		int occupation = length + 2;
		int numPages = (occupation - (occupation % 4096)) / 4096;
		numPages++;
		//assuming the MMU is single-layered,
		if (depth == 1)
		{
			if(numPages)
			//search through each page, and
			for (int i = 0;i < 512; i++)
			{
				//if the page is completely empty, add an allocation at 0
				if (BasedPageTable.table[i].permissions == 42 && numPages == 1)
				{
					
					page hold = page(false, ID, permissions);
					token.offset = 1;
					guardGen(&hold.tokens, token);
					hold.depth = 1;
					hold.maxfree -= occupation;
					BasedPageTable.table[i] = hold;
					return;
				}
				//if the page self reports that it has available space, 
				else if (BasedPageTable.table[i].maxfree > length)
				{
					page hold = BasedPageTable.table[i];
					int seperation = 0;
					//check if that space is at the very end, and squeeze yourself in. if not,
					seperation = 4095 - hold.tokens.get(hold.tokens.size - 1).offset;
					if (4095 - hold.tokens.get(hold.tokens.size - 1).offset > occupation)
					{
						int base = hold.tokens.get(hold.tokens.size - 1).offset + 1;
						token.offset = base + 1;
						guardGen(&hold.tokens, token);
						BasedPageTable.table[i] = hold;
						return;

					}
					//check the distance between each allocation and find the open one.
					for (int i = 0;i < hold.tokens.size-2;i++)
					{
						seperation = hold.tokens.get(i + 1).offset - hold.tokens.get(i).offset;
						if (seperation > length + 2)
						{
							int base = hold.tokens.get(i).offset + 1;
							token.offset = base; +1;
							guardGen(&hold.tokens, token);
							return;
						}
					}


				}
			}
		}
	}
	void guardGen(ArrayList<allocation> *tokens, allocation token)
	{
		allocation test = allocation("guard", token.offset - 1, token.ID);
		tokens->add(allocation("guard", token.offset-1, token.ID));
		tokens->add(token);
		tokens->add(allocation("guard", token.offset + token.length + 1, token.ID));
		return;
	}
	
	void writeByte(uint64_t ID, char data)
	{
		page samplePage = getPage(ID);
		if (samplePage.permissions & W)
		{

		}
	}

	char getByte(page input, unsigned int index)
	{
		unsigned int offset = index >> 3;
		uint64_t block = input.data[offset];
		return 'a';
	}
	uint16_t* segment(uint64_t ptr, int width)
	{
		int n;
		int len = is64Bit ? 64 : 32;
		n = (len - len % width) / width + 1;
		uint16_t* out = (uint16_t *)calloc(n, sizeof(uint16_t));


		uint64_t mask = (uint64_t) 1 << width;
		mask--;
		mask = ~mask;
		printf("%lx\n", mask);
		for (int i = 0;i < n;i++)
		{
			out[i];
		}
		return (uint16_t*)NULL;

	}
};
