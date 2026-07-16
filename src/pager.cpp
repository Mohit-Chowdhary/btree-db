#include "pager.h"
#include <iostream>

Pager* pager_open(const char* filename) {
    FILE* file = fopen(filename, "r+b");
    if(!file){
        file = fopen(filename,"w+b");
    }
    Pager* pager = new Pager(file);

    fseek(pager->file, 0, SEEK_END);
    long file_size = ftell(pager->file);
    pager->total_pages = (int)(file_size / sizeof(BNode));

    return pager;
}

// Bump a resident page's frequency by 1, move it to the new bucket (MRU slot).
static void touch(Pager* pager, int page_num) {
    CacheEntry& entry = pager->cache[page_num];
    int f = entry.freq;

    pager->freq_list[f].erase(entry.it);
    if (pager->freq_list[f].empty()) {
        pager->freq_list.erase(f);
        if (pager->min_freq == f) pager->min_freq = f + 1;
    }

    entry.freq = f + 1;
    pager->freq_list[entry.freq].push_front(page_num);
    entry.it = pager->freq_list[entry.freq].begin();
}

// Evict the least-frequently-used page; ties broken by least-recently-used.
static void evict_one(Pager* pager) {
    std::list<int>& victims = pager->freq_list[pager->min_freq];
    int victim_page = victims.back();
    victims.pop_back();
    if (victims.empty()) pager->freq_list.erase(pager->min_freq);

    CacheEntry& entry = pager->cache[victim_page];
    if (entry.dirty) {
        flush_page(pager, victim_page);
    }
    delete entry.node;
    pager->cache.erase(victim_page);
}

BNode* get_page(Pager* pager, int page_num) {
    auto found = pager->cache.find(page_num);
    if (found != pager->cache.end()) {
        touch(pager, page_num);
        return found->second.node;
    }

    if ((int)pager->cache.size() >= pager->capacity) {
        evict_one(pager);
    }

    BNode* node = new BNode(page_num, false);
    if (page_num < pager->total_pages) {
        fseek(pager->file, (long)page_num * sizeof(BNode), SEEK_SET);
        fread(node, sizeof(BNode), 1, pager->file);
        node->page_no = page_num;
    }

    CacheEntry entry;
    entry.node = node;
    entry.dirty = false;
    entry.freq = 1;
    pager->freq_list[1].push_front(page_num);
    entry.it = pager->freq_list[1].begin();
    pager->cache[page_num] = entry;
    pager->min_freq = 1;

    std::cout << "GET_PAGE " << page_num << " -> page_no=" << node->page_no << "\n";
    return node;
}

// Call this wherever you mutate a page's contents through the returned BNode*.
void mark_dirty(Pager* pager, int page_num) {
    auto found = pager->cache.find(page_num);
    if (found != pager->cache.end()) found->second.dirty = true;
}

void flush_page(Pager* pager, int page_num) {
    auto found = pager->cache.find(page_num);
    if (found == pager->cache.end()) return;

    std::cout << "FLUSH_PAGE " << page_num << "\n"; 

    if (page_num >= pager->total_pages) pager->total_pages = page_num + 1;

    fseek(pager->file, (long)page_num * sizeof(BNode), SEEK_SET);
    fwrite(found->second.node, sizeof(BNode), 1, pager->file);
    fflush(pager->file);
    found->second.dirty = false;
}

void pager_close(Pager* pager) {
    for (auto& [page_num, entry] : pager->cache) {
        if (entry.dirty) flush_page(pager, page_num);
        delete entry.node;
    }
    pager->cache.clear();
    fclose(pager->file);
    delete pager;
}