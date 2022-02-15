#pragma once
class instruction
{
	public:
		short int targetVersion;
		short int targetISA;
		short int instructionType;
		void* funcPntr;
		char* memonic;
};

