// Christopher Fischerr
// 03FEB2022
// Instruction Implementation


class instruction
{
	public:
		short int targetVersion;
		short int targetISA;
		short int instructionType;
		void* funcPntr;
		char* memonic;
};

instruction* getITypes()
{
	instruction* instrcts = new instruction[64];
}