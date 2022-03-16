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
		}
	}
	void allocate(int length, uint64_t ID, char permissions)
	{
		allocation token = allocation(0, 0, length, permissions, ID);
		int numPages = (length - (length % 4096)) / 4096;
		if (depth == 0)
		{
			for (int i = 0;i < 512; i++)
			{
				if (BasedPageTable.table[i].permissions == 42)
				{
					page hold = page(false, ID, permissions);
					token.offset = 1;
					hold.tokens.add(token);


					BasedPageTable.table[i] = hold;
				}
				else if (BasedPageTable.table[i].maxfree > length)
				{

				}
			}
		}
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
