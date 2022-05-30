#include <iostream>
using namespace std;


template <typename T>
class Node {
public:
    T key;
    Node* left;
    Node* right;
    int height;
};

template <typename T>
class tree
{
    public:
        Node<T>* head;
        int max(int a, int b);
        void insertNode(T key);
        void deleteNode(T key);
        void printTree();
        tree();

};

