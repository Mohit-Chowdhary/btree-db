#ifndef BTREE_PAGER
#define BTREE_PAGER

#include "node.h"
#include <unordered_map>
#include <list>

const int CACHE_CAPACITY = 100; // max pages resident in memory at once

struct CacheEntry {
    BNode* node;
    bool dirty;
    int freq;
    std::list<int>::iterator it; // position within freq_list[freq]
};

struct Pager {
    FILE* file;
    int total_pages;   // pages that exist on disk (unbounded by cache size)
    int capacity;
    int min_freq;

    std::unordered_map<int, CacheEntry> cache;         // page_num -> entry
    std::unordered_map<int, std::list<int>> freq_list;  // freq -> MRU-first list of page_nums

    Pager(FILE* f) : file(f), total_pages(0), capacity(CACHE_CAPACITY), min_freq(0) {}
};

Pager* pager_open(const char* filename);
BNode* get_page(Pager* pager, int page_num);
void mark_dirty(Pager* pager, int page_num);
void flush_page(Pager* pager, int page_num);
void pager_close(Pager* pager);

#endif