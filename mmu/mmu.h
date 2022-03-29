#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>

#include "AL.h"

using namespace std;

//permissions special codes:
// 8: guard allocation
//rightmost 3 bits still must match page allocation
class allocation
{
public:
	uint64_t physAddress;
	int offset;
	int length;
	char permissions;
	uint64_t ID;
	allocation(uint64_t a, int b, int c, char d, uint64_t e)
	{
		physAddress = a;
		offset = b;
		length = c;
		permissions = d;
		ID = e;
	}
	allocation(string type, int offset = 0, int ID = 0)
	{
		if (!strcmp(type.c_str(), "guard"))
		{
			physAddress = 0;
			this->offset = offset;
			length = 1;
			permissions = 8;
			this->ID = ID;
		}
	}
};



//1 is for table, 0 is for data
//permissions is also used for special codes
//42: empty page

typedef struct page page;
struct page
{
public:
	int depth;
	bool dType;
	uint64_t address;
	char permissions;
	char* data;
	uint32_t* dirtybits;
	page* table;
	ArrayList<allocation> tokens;
	int maxfree;
	page(bool type, uint64_t address, char permissions)
	{
		this->tokens = ArrayList<allocation>(10);
		this->dType = type;
		this->address = address;
		this->permissions = permissions;
		if (type)
		{
			dirtybits = (uint32_t*)calloc(16, sizeof(uint32_t));
			table = (page*)calloc(512, sizeof(page));
			maxfree = -1;
			return;
		}
		else
		{
			data = (char*)calloc(4096, sizeof(char));
			maxfree = 4096;
		}
	}
	page()
	{
		dType = false;
		permissions = 42;
		this->tokens = ArrayList<allocation>(10);
	}
};

/*typedef struct pagetable_
{
	int depth;
	page *data[512];
	uint32_t dirtybits[16];
} PageTable;*/

