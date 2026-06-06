#ifndef BTREE
#define BTREE
#include "pager.h"
struct BTree{
    Pager* page;
    int root_page;
};

BTree* btree_open(const char* filename);
void insert(BTree* tree, int key, int value);
int search(BTree* tree,int key);
#endif