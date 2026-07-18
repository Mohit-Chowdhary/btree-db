#ifndef BTREE_PAGER
#define BTREE_PAGER

#include "node.h"
#include <unordered_map>
#include <list>

const int CACHE_CAPACITY = 100; // max pages resident in memory at once

struct Pager;

Pager* pager_open(const char* filename);
BNode* get_page(Pager* pager, int page_num);
void mark_dirty(Pager* pager, int page_num);
void flush_page(Pager* pager, int page_num);
void pager_close(Pager* pager);
void pin_page(Pager* pager, int page_num);
void unpin_page(Pager* pager, int page_num);
static void touch(Pager* pager, int page_num);
static bool evict_one(Pager* pager);


class PageHandle{
    Pager* pager;
    int page_num;
    BNode* node;

public:
    PageHandle(Pager* p, int pn): pager(p), page_num(pn), node(get_page(p,pn)){
        pin_page(pager,page_num);
    }
    PageHandle(std::nullptr_t, int) : pager(nullptr), page_num(-1), node(nullptr) {}
        bool valid() const { return node != nullptr; }

    ~PageHandle(){
        if(pager) unpin_page(pager, page_num);
    }

    // disable copying
    PageHandle(const PageHandle&) = delete;
    PageHandle& operator=(const PageHandle&) = delete;

    //moveable
    PageHandle(PageHandle&& other) noexcept
    : pager(other.pager), page_num(other.page_num), node(other.node){
        other.pager = nullptr;
        other.page_num = -1;
    }
    PageHandle& operator=(PageHandle&& other) noexcept{
        if( this != &other){
            if(pager) unpin_page(pager,page_num);
            pager = other.pager;
            page_num = other.page_num;
            node = other.node;
            other.pager = nullptr;
        }   

        return *this;
    }

    BNode* operator->() const { return node;}
    BNode& operator*()  const { return *node;}
    BNode* get()        const { return node;}
    int page()          const { return page_num;}

    void mark_dirty()   {::mark_dirty(pager,page_num);}
    void flush()        {::flush_page(pager,page_num);}
};



struct CacheEntry {
    BNode* node;
    bool dirty;
    int freq;
    int pin_count = 0;
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



#endif