// AVL tree implementation in C++

#include <iostream>
#include "avl.h"
using namespace std;




template <typename T>
void tree<T>::insertNode(T key)
{
    insertNodeR(head, key);
    return;
}
template <typename T>
void tree<T>::deleteNode(T key)
{
    deleteNodeR(head, key);
    return;
}
template <typename T>
void tree<T>::printTree()
{
    printTreeP(head, "", true);
}

template <typename T>
tree<T>::tree()
{
    return this;
}




// Calculate height
template <typename T>
int height(Node<T>* N) {
    if (N == NULL)
        return 0;
    return N->height;
}
int max(int a, int b) {
    return (a > b) ? a : b;
} 
// New node creation
template <typename T>
Node<T>* newNode(T key) {
    Node* node = new Node();
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return (node);
}

// Rotate right
template <typename T>
Node<T>* rightRotate(Node<T>* y) {
    Node* x = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(height(y->left),
        height(y->right)) +
        1;
    x->height = max(height(x->left),
        height(x->right)) +
        1;
    return x;
}

// Rotate left
template <typename T>
Node<T>* leftRotate(Node<T>* x) {
    Node* y = x->right;
    Node* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(height(x->left),
        height(x->right)) +
        1;
    y->height = max(height(y->left),
        height(y->right)) +
        1;
    return y;
}

// Get the balance factor of each node
template <typename T>
int getBalanceFactor(Node<T>* N) {
    if (N == NULL)
        return 0;
    return height(N->left) -
        height(N->right);
}

// Insert a node
template <typename T>
Node<T>* insertNodeR(Node<T>* node, int key)
{
    // Find the correct position and insert the node
    if (node == NULL)
        return (newNode(key));
    if (key < node->key)
        node->left = insertNodeR(node->left, key);
    else if (key > node->key)
        node->right = insertNodeR(node->right, key);
    else
        return node;

    // Update the balance factor of each node and
    // balance the tree
    node->height = 1 + max(height(node->left),
        height(node->right));
    int balanceFactor = getBalanceFactor(node);
    if (balanceFactor > 1)
    {
        if (key < node->left->key)
        {
            return rightRotate(node);
        }
        else if (key > node->left->key)
        {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }
    }
    if (balanceFactor < -1)
    {
        if (key > node->right->key)
        {
            return leftRotate(node);
        }
        else if (key < node->right->key)
        {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }
    }
    return node;
}

// Node with minimum value
template <typename T>
Node<T>* nodeWithMimumValue(Node<T>* node) {
    Node* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

// Delete a node
template <typename T>
Node<T>* deleteNodeR(Node<T>* root, int key) {
    // Find the node and delete it
    if (root == NULL)
        return root;
    if (key < root->key)
        root->left = deleteNodeR(root->left, key);
    else if (key > root->key)
        root->right = deleteNodeR(root->right, key);
    else
    {
        if ((root->left == NULL) ||
            (root->right == NULL))
        {
            Node* temp = root->left ? root->left : root->right;
            if (temp == NULL)
            {
                temp = root;
                root = NULL;
            }
            else
                *root = *temp;
            free(temp);
        }
        else
        {
            Node* temp = nodeWithMimumValue(root->right);
            root->key = temp->key;
            root->right = deleteNodeR(root->right,
                temp->key);
        }
    }

    if (root == NULL)
        return root;

    // Update the balance factor of each node and
    // balance the tree
    root->height = 1 + max(height(root->left),
        height(root->right));
    int balanceFactor = getBalanceFactor(root);
    if (balanceFactor > 1) {
        if (getBalanceFactor(root->left) >= 0) {
            return rightRotate(root);
        }
        else {
            root->left = leftRotate(root->left);
            return rightRotate(root);
        }
    }
    if (balanceFactor < -1) {
        if (getBalanceFactor(root->right) <= 0) {
            return leftRotate(root);
        }
        else {
            root->right = rightRotate(root->right);
            return leftRotate(root);
        }
    }
    return root;
}

// Print the tree
template <typename T>
void printTreeP(Node<T>* root, string indent, bool last) {
    if (root != nullptr) {
        cout << indent;
        if (last) {
            cout << "R----";
            indent += "   ";
        }
        else {
            cout << "L----";
            indent += "|  ";
        }
        cout << root->key << endl;
        printTree(root->left, indent, false);
        printTree(root->right, indent, true);
    }
}