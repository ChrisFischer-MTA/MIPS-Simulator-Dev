#pragma once
#include <iostream>
#include <stdio.h>
#include <cassert>
#include <string.h>

#ifndef MMUCPP
#define MMUCPP 1
#endif

#include "binja.h"
#include "mmu.h"

#define BLOCKWIDTH 1024

using namespace std;

#define R 4
#define W 2
#define X 1

//const short int IntegerOverflow = 1;
//const short int MemoryFault = 2;


section::section(int start, int length, char permissions, int width, char *name, segment parent)
{
	this->start = start;
	this->length = length;
	this->end = start + length;
	this->width = width;
	this->name = name;

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

	this->parent = (segment *)malloc(sizeof(segment));
	*(this->parent) = parent;
	printf("%xsegmentstart\n", this->parent->start);
}

void generallyPause();
class MMU
{
	public:
	vector<segment> segments;
	vector<section> allSections;
	vector<char> stack;
	uint64_t stackBase;
	int alloLength;
	bool is64Bit;
	BinaryView* bv = NULL;

	MMU(bool is64bit, BinaryView* bc, uint64_t stackBase=0)
	{	
		//Allocate the stack
		this->stack = vector<char>();
		this->stackBase = stackBase;

		// Assign the passed BinaryView into our class.
		bv = bc;
		
		
		// At some point need to check bv->platform->architecture for is64bit bool
		auto tmp = bv->GetSegments();
		segments = vector<segment>(tmp.size());
		
		this->is64Bit = is64bit;
		for (int i = 0;i < tmp.size(); i++)
		{
			segments[i].start = tmp[i]->GetStart();
			segments[i].end = tmp[i]->GetEnd();
			segments[i].length = tmp[i]->GetLength();
			int flags = tmp[i]->GetFlags();
			
			segments[i].setPerms(flags & 7);
			segments[i].permissions = flags;
			segments[i].ID = 0;

			segments[i].sections = new vector<section>();
		}
		auto sectionlist = bv->GetSections();
		allSections = vector<section>();
		char *secName;

		for (int i = 0;i < sectionlist.size();i++)
		{
			//find parent segment
			segment parent = segSearch(sectionlist[i]->GetStart());
			//create section object
			
			secName = (char *)calloc((sectionlist[i]->GetName().size())+1, sizeof(char));
			
			// Ironically, this is a heap overflow.
			
			strncpy(secName, sectionlist[i]->GetName().c_str(), sectionlist[i]->GetName().size());

			section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName, parent);
			
			allSections.push_back(token);
			//allSections[i].parent = (segment *)malloc(sizeof(segment));

			//section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName);
			
		}
		//sort sections
		this->secSort();

		for (int i = 0;i < allSections.size();i++)
		{
			//couple segment and section
			allSections[i].parent->sections->push_back(allSections[i]);
		}
	}

	
	void secSort()
	{
		vector<section> sorted = vector<section>();
		bool *disabled = (bool *)calloc(allSections.size(), sizeof(bool));
		section least;
		int leastIndex;

		fflush(stdout);
		for (int j = 0;j < allSections.size();j++)
		{
			
			least = section(0xffffffffffffffff);
			
			for (int i = 0;i < allSections.size();i++)
			{
				if (allSections[i] < least && !disabled[i])
				{
					least = allSections[i];
					leastIndex = i;
				}
			}
			sorted.push_back(least);
			disabled[leastIndex] = true;

		}
		for (int i = 1;i < allSections.size();i++)
		{
			allSections = sorted;
		}
		return;
			
	}
	

	
	segment segSearch(uint64_t index)
	{
		for (int i = 0;i < segments.size();i++)
		{
			if (index <= segments[i].end && index >= segments[i].start)
			{
				return segments[i];
			}
		}
		return segment();
	}
	
		
	//Returns a pointer to a stream of bytes that can be read as necessary
	//the stream of bytes is either the mmu memory segments (ideally) or a reconstructed short block
	//of unwritable straight from binja
	//numBytes doesn't make it grab n bytes, it's just there to make sure a block boundary isn't being crossed
	char * getEffectiveAddress(uint64_t address, int numBytes, int gpr, uint64_t contents = 0)
	{
		//For Stack pointer access
		if(gpr == 29)
		{
			if(contents > stackBase + stack.size())
			{
				stack.resize(contents - stackBase + 1);
			}
			if(address > contents)
			{
				stack.resize(address - stackBase + 1);
			}

			uint64_t stackOffset = address - stackBase;
			return stack.data() + stackOffset;
		}
		//For Binja binary accesses	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < segments[i].sections->size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					fflush(stdout);
					//Is it readable?
					if (token.readable)
					{
						//Is it writable? Exists to avoid loading unwritable data when unnecessary
						if (token.writable)
						{
							
							//Yes, could have a dirty state in memory:
							//block access arithmetic, token[depth][blockOffset] should be starting point
							fflush(stdout);
							uint64_t offset = address - token.start;
							uint64_t depth = (int)(offset / token.width);
							uint64_t blockOffset = offset % token.width;
							fflush(stdout);
							//Not even initialized, pull and return 
							if (!token.initialized[depth])
							{
								//token[depth] = binja.access(token.start + depth*width, width);
								if (bv->Read(token.array[depth], address - blockOffset, token.width) != token.width)
									1 + 1;
								token.initialized[depth] = true;

							}
							printf("[FLUSH] 3\n");
							if (blockOffset + numBytes > token.width)
							{
								printf("something weird happened\n");
								generallyPause();
							}
							printf("[FLUSH] %x, %x, %x, %x, %x\n", address, token.start, offset, depth, blockOffset);

							char * testing = token.array[depth];
							for(int i=0;i<16;i++)
							{
								printf("%x", testing[i]);
							}
							printf("\n");

							return token.array[depth] + blockOffset;


						}
						//should just return a pointer to a numBytes-length array of the requested bytes
						else
						{
							char* out = (char*)(calloc(numBytes, sizeof(char)));

							//if(num bytes read by Read() != numBytes)
							if (bv->Read(out, address, numBytes) != numBytes)
								generallyPause();
							return out;
						}
					}
				}
				else
				{
					//emulator->signalException(MemoryFault);
				}

			}
		}
		// It is possible to reach this point so we need to return some value, however I dont believe it should be NULL
		// Rose I will need your help on a more permanent solution for this - Sean
		return NULL;
	}
	
	/*int writeToBytes(char *data, int length)
	{
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < allSections.size();j++)
			{
				segment tokenHold = segments[i];
				section token = (*(tokenHold.sections))[j];
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					if (token.writable)
					{
						//Yes, could have a dirty state in memory:
						//block access arithmetic, token[depth][blockOffset] should be starting point
						int offset = address - token.start;
						int depth = offset / token.width;
						int blockOffset = depth % token.width;
					{

				}
			}
		}
	}*/



	unsigned char* getBytes(int addr)
	{	
		if(bv == NULL)
		{
			printf("Binary View is not initalized in this getBytes call!\n");
		}

		// Allocate bytes and buffer
		size_t numBytesRead;
		
		// Create location for bytes to be read in to
		unsigned char* bytes = (unsigned char*) malloc(sizeof(char) * 4);

		// Check for a null pntr
		if(bytes == NULL)
		{
			printf("Insufficent space on heap for a malloc.\n");
		}

		numBytesRead = bv->Read(bytes, addr, 4);
		
		return bytes;
	}

	void store(uint64_t address, void* data, int datalength)
	{

	}

	void generallyPause()
	{
		while (1)
		{
			// do nothing
		}
	}
	void printSections()
	{
		printf("Sections:\n");
		for(int i=0;i<allSections.size();i++)
		{
			printf("Start: 0x%lx, Size: %lx, Name %s\n", allSections[i].start, allSections[i].end - allSections[i].start, allSections[i].name);
		}
	}
	
};
