#pragma once
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>


#include "../binja.h"
#include "AL.h"

using namespace std;

class segment;

//permissions 00000rwx
class section
{
public:
	//Meta
	uint64_t start, end;
	int length;
	bool readable;
	bool writable;
	bool executable;
	uint64_t ID;
	segment parent;

	//Chunk of memory
	char** array;
	int width;
	bool* initialized;
	bool* writtenTo;
	section(int start, int length, char permissions, int width) {
		this->start = start;
		this->length = length;
		this->end = start + length;
		this->width = width;

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

	}
	void setPerms(char permissions)
	{
		readable = (permissions >> 2) & 1;
		writable = (permissions >> 1) & 1;
		executable = permissions & 1;
	}

	friend bool operator<(const section& lhs, const section& rhs) { return lhs.start < rhs.start; }
};

class segment
{
public:
	//Meta
	uint64_t start, end;
	int length;
	char permissions;
	bool readable;
	bool writable;
	bool executable;
	uint64_t ID;

	//Data
	ArrayList<section> sections;
	section operator [](int i) const { return sections[i]; }
	section& operator [](int i) { return sections[i]; }
	void setPerms(char permissions)
	{
		readable = (permissions >> 2) & 1;
		writable = (permissions >> 1) & 1;
		executable = permissions & 1;
	}
};

class MMU
{
public:
	ArrayList<segment> segments;
	ArrayList<section> allSections;
	int alloLength;
	bool is64Bit;
	BinaryView* bv;
	MMU(bool is64bit, BinaryView* binview);
	MMU();
	void secSort();
	segment segSearch(uint64_t index);
	char* getEffectiveAddress(uint64_t address, int numBytes);
	void store(uint64_t address, void* data, int datalength);

};



