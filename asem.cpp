
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <inttypes.h>
#include <cassert>

//Macro that compares plain text "opcode" to macro input
//only here because it would otherwise get copy/pasted unreadably like 420 times in this document
#define c(in) !strcmp(tokens[0].c_str(), in)


using namespace std;

typedef struct inst {
	uint32_t asem;
	uint32_t op, rs, rt, imm, address, rd, shamt, funct;
	char type = 0;
	inst(uint32_t asem = 0, uint32_t op = -1, uint32_t  rs = -1, uint32_t rt = -1, uint32_t imm = -1, uint32_t address = -1, 
		 uint32_t rd = -1, uint32_t shamt = -1, uint32_t funct = -1,char type = -1)
	{
		this->asem = asem;
		this->op = op;
		this->rs = rs;
		this->rt = rt;
		this->imm = imm;
		this->address = address;
		this->rd = rd;
		this->shamt = shamt;
		this->funct = funct;
		this->type = type;
	}
} instruction;



string* split(string input)
{
	int n = count(input.begin(), input.end(), ' ');
	string* out = (string*)calloc(n + 1, sizeof(string));
	assert(out != NULL);
	string delim = " ";
	for (int i = 0; i < n + 1;i++)
	{
		out[i] = input.substr(0, input.find(delim));
		input.erase(0, input.find(delim) + delim.length());
	}
	return out;
}



uint32_t reg(string input)
{
	const char* in = input.c_str();
	if (in[1] >= 48 && in[1] <= 57)
	{
		uint32_t out = (in[1] - 48) * 10;
		if (strlen(in) >= 3)
			out += in[2] - 48;
		else
			out /= 10;
		return out;
	}
	if (in[1] == 'a')
	{
		if (in[2] == 't')
			return 1;
		return 4 + in[2] - '0';
	}
	if (in[1] == 'v')
	{
		return 2 + in[2] - '0';
	}
	if (in[1] == 't')
	{
		if (in[2] <= '7')
			return 8 + in[2] - '0';
		else
			return 24 + in[2] - '8';
	}
	if (in[1] == 's')
	{
		if (in[2] == 'p')
			return 29;
		return 16 + in[2] - '0';
	}
	if (in[1] == 'k')
	{
		return 26 + in[2] - '0';
	}
	if (in[1] == 'v')
		return 28;
	if (in[2] == 'f')
		return 30;
	if (in[2] == 'r')
	{
		if (in[3] == '0')
			return 0;
		return 31;
	}
	if (in[2] == 'z')
		return 0;
		


	return -1;
}

instruction rType(string* tokens, int funct)
{
	instruction out = instruction();
	out.type = 'r';
	out.op = 0;
	out.funct = funct;
	out.shamt = 0;
	out.rd = reg(tokens[1]);
	out.rs = reg(tokens[2]);
	out.rt = reg(tokens[3]);
	out.asem |= out.funct;
	//printf("%d, %d, %d\n", out.rs, out.rt, out.rd);
	//printf("%.8x\n", out.asem);
	out.asem |= out.rd << 11;
	//printf("%.8x\n", out.rd << 11);
	out.asem |= out.rt << 16;
	//printf("%.8x\n", out.rt << 16);
	out.asem |= out.rs << 21;
	//printf("%.8x\n", out.rs << 21);
	return out;
}
instruction iType(string* tokens, uint32_t op)
{
	instruction out = instruction();
	out.type = 'i';
	out.op = op;
	out.rt = reg(tokens[1]);
	out.rs = reg(tokens[2]);
	out.imm = stoi(tokens[3]);
	out.asem |= out.imm;
	out.asem |= out.rt << 16;
	out.asem |= out.rs << 21;
	out.asem |= out.op << 26;
	return out;
}

instruction assemble(string input)
{
	string* tokens = split(input);
	if (c("add"))
	{
		return rType(tokens, 32);
	}
	else if (c("addi"))
	{
		return iType(tokens, 8);
	}
	else if (c("addiu"))
	{
		return iType(tokens, 9);
	}
	else if (c("addu"))
	{
		return rType(tokens, 33);
	}



	return instruction();
}




