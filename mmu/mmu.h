#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>

#include "AL.h"

using namespace std;

//permissions 00000rwx
class allocation
{
public:
	uint64_t start, end;
	int length;
	int width;
	bool readable;
	bool writable;
	bool executable;
	uint64_t ID;
	char** array;
	bool* initialized;
	bool* writtenTo;
	allocation(int start, int length, char permissions, int width) {
		this->start = start;
		this->length = length;
		this->end = start + length;
		this->width = width;

		this->readable = permissions >> 2;
		this->writable = (permissions >> 1) & 1;
		this->executable = permissions & 1;

		int depth = (length / width) + 1;
		this->array = (char**)calloc(depth, sizeof(char*));
		for (int i = 0;i < depth;i++)
		{
			this->array[i] = (char*)calloc(width, sizeof(char));
		}
		this->initialized = (bool *)calloc(depth, sizeof(bool));
		this->writtenTo = (bool*)calloc(depth, sizeof(bool));
		ID = 0;
		
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

