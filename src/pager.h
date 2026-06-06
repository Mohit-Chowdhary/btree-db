#ifndef BTREE_PAGER
#define BTREE_PAGER

#include "node.h"

const int MAX_PAGES = 100;

struct Pager{
    FILE* file;
    int total_pages;
    BNode* cache[MAX_PAGES];
    Pager(FILE* f){
        file = f;
    }
};

Pager* pager_open(const char* filename);
BNode* get_page(Pager* pager, int page_num);
void flush_page(Pager* pager, int page_num);
void pager_close(Pager* pager);

#endif