#include "pager.h"
#include <iostream>
#include <string>

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

// Evict the least-frequently-used unpinned page; ties broken by least-recently-used.
// Returns false if every cached page is currently pinned (nothing to evict).
static bool evict_one(Pager* pager) {
    for (auto bucket_it = pager->freq_list.begin(); bucket_it != pager->freq_list.end(); ++bucket_it) {
        std::list<int>& bucket = bucket_it->second;

        for (auto it = bucket.rbegin(); it != bucket.rend(); ++it) {
            int candidate = *it;
            if (pager->cache[candidate].pin_count > 0) continue; // skip pinned pages

            // found an unpinned victim — remove it from this bucket
            bucket.erase(std::next(it).base());
            if (bucket.empty()) pager->freq_list.erase(bucket_it->first);

            CacheEntry& entry = pager->cache[candidate];
            if (entry.dirty) {
                flush_page(pager, candidate);
            }
            delete entry.node;
            pager->cache.erase(candidate);

            // if we just emptied out the old min_freq bucket, recompute it lazily
            if (!pager->freq_list.empty())
                pager->min_freq = pager->freq_list.begin()->first;

            return true;
        }
    }
    return false; // every cached page is pinned
}

BNode* get_page(Pager* pager, int page_num) {
    auto found = pager->cache.find(page_num);
    if (found != pager->cache.end()) {
        touch(pager, page_num);
        return found->second.node;
    }

    if ((int)pager->cache.size() >= pager->capacity) {
        if( !evict_one(pager)){
            throw std::runtime_error(
                "pager: cache full and every page is pinned (capacity=" +
                std::to_string(pager->capacity) + ")");
        }
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

void pin_page(Pager* pager, int page_num){
    auto found = pager->cache.find(page_num);
    if(found != pager->cache.end()) found->second.pin_count++;
}

void unpin_page(Pager* pager, int page_num){
    auto found = pager->cache.find(page_num);
    if(found != pager->cache.end() && found->second.pin_count >0){
        found->second.pin_count--;
    }
}