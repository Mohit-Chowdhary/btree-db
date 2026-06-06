#include "btree.h"

BTree* btree_open(const char* filename){
    Pager* pager = pager_open(filename);
}


void insert(BTree* tree, int key, int value);


int search(BTree* tree,int key){
    BNode* node = get_page(tree->page, tree->root_page);
    // get root node
    while(!node->is_leaf){
        int i = 0;
        while(i<ORDER-1 && key>node->keys[i]) i++;
        // check which index to lookinto
        node = get_page(tree->page, node->children[i]);
        // get into that, do this untill a leaf is found
    }
    for(int i=0; i< ORDER-1; i++){
        if(node->keys[i] == key) return node->values[i];
    }

    return -1;
}