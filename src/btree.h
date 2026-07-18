#ifndef BTREE
#define BTREE
#include "pager.h"
#include <vector>
#include <utility>
struct BTree{
    Pager* page;
    int root_page;
    const char* meta_filename;
};

BTree* btree_open(const char* filename, const char* meta_filename);
PageHandle find_parent(BTree* tree, int curr_page, int target_page);
void insert_into_parent(BTree* tree, BNode* left, int key, BNode* right);
void split_leaf(BTree* tree, BNode* node, int key, int value);
void split_internal(BTree* tree, BNode* node, int key, BNode* right);
void insert(BTree* tree, int key, int value);
int search(BTree* tree,int key);
std::vector<std::pair<int,int>> range_query(BTree* tree, int left, int right);

void btree_close(BTree* tree);
void delete_key(BTree* tree, int key);
void handle_underflow(BTree* tree, BNode* node);


void print_tree(BTree* tree, int page_num, int depth);
#endif