#include <iostream>
#include <stdexcept>
#ifndef BTREE_NODE
#define BTREE_NODE

const int ORDER = 4;

struct BNode{
    int page_no;
    int keys[ORDER-1];
    bool is_leaf;
    int num_keys = 0;

    // if not leaf
    int children[ORDER];

    // if leaf
    int values[ORDER-1];
    int next_leaf;

    BNode(int page_no, bool is_leaf){
        this->page_no = page_no;
        this->is_leaf = is_leaf;
        this->next_leaf = -1;
        for(int i=0; i<ORDER-1; i++){
            keys[i] = 0; values[i] = 0;
        }
        for(int i=0; i<ORDER; i++) children[i] = -1;
    }
};

//lets think

#endif

