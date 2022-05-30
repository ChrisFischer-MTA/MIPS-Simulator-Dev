#pragma once

template <class T>
class ArrayList
{
public:
	T* array;
	int size;
	int capacity;
	ArrayList(int capacity = 10);
	void grow();
	void add(T input);
	void empty();
	T get(int index);
	T operator [](int i) const { return array[i]; }
	T& operator [](int i) { return array[i]; }
};
