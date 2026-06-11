#ifndef BTREE
#define BTREE
#include "pager.h"
struct BTree{
    Pager* page;
    int root_page;
};

BTree* btree_open(const char* filename);
BNode* find_parent(BTree* tree, int curr_page, int target_page);
void insert_into_parent(BTree* tree, BNode* left, int key, BNode* right);
void split_leaf(BTree* tree, BNode* node, int key, int value);
void split_internal(BTree* tree, BNode* node, int key, BNode* right);
void insert(BTree* tree, int key, int value);
int search(BTree* tree,int key);

void delete_key(BTree* tree, int key);
void handle_underflow(BTree* tree, BNode* node);


void print_tree(BTree* tree, int page_num, int depth);
#endif