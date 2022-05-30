#include <iostream>

#include "AL.h"

using namespace std;

template <class T>
ArrayList<T>::ArrayList(int capacity = 10)
{
	this->capacity = capacity;
	this->array = (T *)calloc(capacity, sizeof(T));
	this->size = 0;
}

	template <class T>
void ArrayList<T>::grow()
	{
		this->capacity *= 2;
		array = (T*)realloc(array, this->capacity);
		return;
	}

template <class T>
void ArrayList<T>::add(T input)
	{
		if (size == capacity)
			grow();
		array[size] = input;
		size++;
	}
template <class T>
void ArrayList<T>::empty()
	{
		size = 0;
	}
template <class T>
T ArrayList<T>::get(int index)
	{
		if (index >= size)
			printf("using arraylist like a weirdo warning\n");
		return array[index];
	}


/*template<>
class ArrayList<uint32_t> {
public:
	uint32_t* array;
	int size;
	int capacity;
	ArrayList(int capacity = 10)
	{
		this->capacity = capacity;
		this->array = (uint32_t*)calloc(capacity, sizeof(uint32_t));
		this->size = 0;
	}

	void grow()
	{
		this->capacity *= 2;
		array = (uint32_t*)realloc(array, this->capacity);
		return;
	}

	void add(uint32_t input)
	{
		if (size == capacity)
			grow();
		array[size] = input;
		size++;
	}
	void empty()
	{
		size = 0;
	}
	uint32_t get(int index)
	{
		if (index >= size)
			printf("using arraylist like a weirdo warning\n");
		return array[index];
	}
};*/