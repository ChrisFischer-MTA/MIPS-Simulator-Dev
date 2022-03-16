#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>

#include "AL.h"

using namespace std;


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
	allocation(string type)
	{
		if (strcmp(type.c_str(), "guard"))
		{
			physAddress = 0;
			offset = 0;
			length = 1;
			permissions = 0;
			ID = 0;
		}
	}
};



//1 is for table, 0 is for data
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

