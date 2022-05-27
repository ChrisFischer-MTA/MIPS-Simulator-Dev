#pragma once
#include <iostream>
#include <stdio.h>
#include <cassert>

#ifndef MMUCPP
#define MMUCPP 1
#endif

#include "mmu.h"
#include "../binja.h"

#define BLOCKWIDTH 1024

using namespace std;

#define R 4
#define W 2
#define X 1

const short int IntegerOverflow = 1;
const short int MemoryFault = 2;


void generallyPause();

MMU::MMU(bool is64bit, EmulatedCPU rock)
{
	bv = binview;
	auto buttsegs = bv->GetSegments();
	segments = ArrayList<segment>(buttsegs.size());
	this->is64Bit = is64bit;
	for (int i = 0;i < buttsegs.size(); i++)
	{
		segments[i].start = buttsegs[i]->GetStart();
		segments[i].end = buttsegs[i]->GetEnd();
		segments[i].length = buttsegs[i]->GetLength();
		int flags = buttsegs[i]->GetFlags();
		segments[i].setPerms(flags & 7);
		segments[i].permissions = flags;
		segments[i].ID = 0;
		segments[i].sections = ArrayList<section>(5);
	}

	auto sectionlist = bv->GetSections();
	allSections = ArrayList<section>(sectionlist.size());
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

	for (int i = 0;i < allSections.size;i++)
	{
		//couple segment and section
		allSections[i].parent->sections.add(allSections[i]);
	}
			

		
}

MMU::MMU()
{

}

void MMU::secSort()
{
	ArrayList<section> sorted = ArrayList<section>(allSections.size);
	section least = allSections[0];
	for (int j = 0;j < allSections.size;j++)
	{
		int leastindex = 0;
		for (int i = 1;i < allSections.size;i++)
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
	for (int i = 1;i < allSections.size;i++)
	{
		allSections[i] = sorted[i];
	}
	return;
		
}

segment MMU::segSearch(uint64_t index)
{
	for (int i = 0;i < segments.size;i++)
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
char * MMU::getEffectiveAddress(uint64_t address, int numBytes)
{
	//For each segment,
	for (int i = 0;i < segments.size;i++)
	{
		//For each section,
		for (int j = 0;j < allSections.size;j++)
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
	// Chris and Rose I will need your help on a more permanent solution for this - Sean
	return NULL;
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