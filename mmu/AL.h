
template <class T>
class ArrayList
{
public:
	ArrayList(int capacity = 10);
	void grow();
	void add(T input);
	void empty();
	T get(int index);
};