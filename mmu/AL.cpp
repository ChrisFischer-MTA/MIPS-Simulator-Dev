#include <iostream>

#include "AL.h"

using namespace std;

template <class T>
class ArrayList
{
public:
	T* array;
	int size;
	int capacity;
	ArrayList(int capacity = 10)
	{
		this->capacity = capacity;
		this->array = (T *)calloc(capacity, sizeof(T));
		this->size = 0;
	}

	void grow()
	{
		this->capacity *= 2;
		array = (T*)realloc(array, this->capacity);
		return;
	}

	void add(T input)
	{
		if (size == capacity)
			this->grow();
		array[size] = input;
		size++;
	}
	void empty()
	{
		size = 0;
	}
	T get(int index)
	{
		if (index >= size)
			printf("using arraylist like a weirdo warning\n");
		return array[index];
	}

};