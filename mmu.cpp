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


void generallyPause();
class MMU
{
	public:
	vector<segment> segments;
	vector<section> allSections;
	int alloLength;
	bool is64Bit;
	BinaryView* bv = NULL;

	MMU(bool is64bit, BinaryView* bc)
	{	
	
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

			// Rose I need help
			segments[i].sections = new vector<section>(5);
		}
		auto sectionlist = bv->GetSections();
		allSections = vector<section>(sectionlist.size());
		char *secName;
		for (int i = 0;i < sectionlist.size();i++)
		{
			//find parent segment
			segment parent = segSearch(sectionlist[i]->GetStart());
			//create section object
			

			secName = (char *)calloc(sectionlist[i]->GetName().size(), sizeof(char));
			strcpy(secName, sectionlist[i]->GetName().c_str());
			printf("name2: %s\t", secName);
			section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName);
			allSections.push_back(token);
			allSections[i] = token;
			allSections[i].parent = (segment *)malloc(sizeof(segment));

			//section token = section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH, secName);

			printf("name2: %s\t", allSections[i].name);
			printf("name3:%x\n", allSections[i].start);
			
			
	
				
			*(allSections[i].parent) = parent;	

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
		vector<section> sorted = vector<section>(allSections.size());
		section least = allSections[0];
		bool *disabled = (bool *)calloc(allSections.size(), sizeof(bool));
		for (int j = 0;j < allSections.size();j++)
		{
			int leastindex = 0;
			for (int i = 1;i < allSections.size();i++)
			{
				if (allSections[i] < least && !disabled[i])
				{
					least = allSections[i];
					leastindex = i;
				}
			}
			sorted[j] = least;
			disabled[leastindex] = true;
		}
		for (int i = 1;i < allSections.size();i++)
		{
			allSections[i] = sorted[i];
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
	
	
	char * getEffectiveAddress(uint64_t address, int numBytes)
	{	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < segments[i].sections->size();j++)
			{
				segment tokenHold = segments[i];
				printf("[FLUSH] %d, %d\n", i, j);
				section token = (*(tokenHold.sections))[j];
				printf("[FLUSH] 1\n");
				//Is it valid? (within bounds)
				if (address >= token.start && address <= token.end)
				{
					printf("[FLUSH] 1\n");
					//Is it readable?
					if (token.readable)
					{
						//Is it writable? Exists to avoid loading unwritable data when unnecessary
						if (token.writable)
						{
							
							//Yes, could have a dirty state in memory:
							//block access arithmetic, token[depth][blockOffset] should be starting point
							printf("[FLUSH] 1\n");
							printf("%ld, %ld token start, width\n", token.start, token.width);
							printf("[FLUSH] 1\n");
							uint64_t offset = address - token.start;
							printf("[FLUSH] 1\n");
							uint64_t depth = (int)(offset / token.width);
							uint64_t blockOffset = depth % token.width;
							printf("[FLUSH] 2\n");
							fflush(stdout);
							//Not even initialized, pull and return 
							if (!token.initialized[depth])
							{
								//token[depth] = binja.access(token.start + depth*width, width);
								if (bv->Read(token.array[depth], address - blockOffset, token.width) != token.width)
									generallyPause();
								token.initialized[depth] = true;

							}
							printf("[FLUSH] 3\n");
							if (blockOffset + numBytes > token.width)
							{
								printf("something weird happened\n");
								generallyPause();
							}
							printf("[FLUSH] 3\n");
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
			printf("Start: %lx, Size: %lx, Name %s\n", allSections[i].start, allSections[i].end - allSections[i].start, allSections[i].name);
		}
	}
};
