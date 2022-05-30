#pragma once
#include <iostream>
#include <stdio.h>
#include <cassert>

#ifndef MMUCPP
#define MMUCPP 1
#endif


#include "binja.h"

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
	BinaryView* bv;
	MMU(bool is64bit, BinaryView* bc)
	{	
	
		// Assign the passed BinaryView into our class.
		bv = bc;
		
		/*
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
			//vector<section> v(5,section());
			//segments[i].sections = v;
		}

		auto sectionlist = bv->GetSections();
		allSections = vector<section>(sectionlist.size());
		for (int i = 0;i < sectionlist.size();i++)
		{
			//find parent segment
			segment parent = segSearch(sectionlist[i]->GetStart());

			//create section object
			allSections[i] =
				section(sectionlist[i]->GetStart(), sectionlist[i]->GetLength(), parent.permissions, BLOCKWIDTH);
				
			*(allSections[i].parent) = parent;	

		}
		//sort sections
		secSort();

		for (int i = 0;i < allSections.size();i++)
		{
			//couple segment and section
			allSections[i].parent->sections.push_back(allSections[i]);
		}
		*/
	}

	/*
	void secSort()
	{
		vector<section> sorted = vector<section>(allSections.size());
		section least = allSections[0];
		for (int j = 0;j < allSections.size();j++)
		{
			int leastindex = 0;
			for (int i = 1;i < allSections.size();i++)
			{
				if (allSections[i] < least)
				{
					least = allSections[i];
					leastindex = i;
				}
			}
			sorted[j] = least;
			allSections[leastindex].start = 0;
		}
		for (int i = 1;i < allSections.size();i++)
		{
			allSections[i] = sorted[i];
		}
		return;
			
	}
	*/

	/*
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
	*/
		
	//Returns a pointer to a stream of bytes that can be read as necessary
	//the stream of bytes is either the mmu memory segments (ideally) or a reconstructed short block
	//of unwritable straight from binja
	//numBytes doesn't make it grab n bytes, it's just there to make sure a block boundary isn't being crossed
	
	/*
	char * MMU::getEffectiveAddress(uint64_t address, int numBytes)
	{	
		//For each segment,
		for (int i = 0;i < segments.size();i++)
		{
			//For each section,
			for (int j = 0;j < allSections.size();j++)
			{
				section token = segments[i].sections[j];
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
							if (!token.initialized[depth])
							{
								//token[depth] = binja.access(token.start + depth*width, width);
								if (bv->Read(token.array[depth], address - blockOffset, token.width) != token.width)
									generallyPause();
								token.initialized[depth] = true;

							}
							if (blockOffset + numBytes > token.width)
							{
								printf("something weird happened\n");
								generallyPause();
							}

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
	*/



	unsigned char* getBytes(int addr)
	{
		// Allocate bytes and buffer
		size_t numBytesRead;
		unsigned char* bytes = (unsigned char*) malloc(sizeof(char) * 4);
		numBytesRead = bv->Read(bytes, addr, 4);
		
		return bytes;
	}

	void MMU::store(uint64_t address, void* data, int datalength)
	{

	}

	void generallyPause()
	{
		while (1)
		{
			// do nothing
		}
	}
};
