#pragma once
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>
#include <vector>


#include "binja.h"
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
	segment* parent;
	char *name;

	//Chunk of memory
	char** array;
	int width;
	bool* initialized;
	bool* writtenTo;
	section(int start, int length, char permissions, int width, char *name, segment parent);
	section()
	{
		this->start = 0;
		this->length = 0;
		this->end = 0;
		this->width = 0;
		this->setPerms(0);
		ID = 0;
	}

	section(uint64_t start)
	{
		this->start = start;
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
	vector<section> *sections;
	
	void setPerms(char permissions)
	{
		readable = (permissions >> 2) & 1;
		writable = (permissions >> 1) & 1;
		executable = permissions & 1;
	}
};

/*
class MMU
{
public:
	//ArrayList<segment> segments;
	//ArrayList<section> allSections;
	vector<segment> segments;
	vector<section> allSections;
	int alloLength;
	bool is64Bit;
	BinaryNinja::BinaryView* bv;
	MMU(bool);
	void secSort();
	segment segSearch(uint64_t index);
	char* getEffectiveAddress(uint64_t address, int numBytes);
	void store(uint64_t address, void* data, int datalength);

};
*/


