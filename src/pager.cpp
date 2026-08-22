#include "pager.h"
#include "wal.h"
#include <iostream>
#include <string>

bool g_debug_mode = false;


Pager* pager_open(const char* filename, const char* wal_filename) {
    //wal recovery
    FILE* wal_file = fopen(wal_filename, "r+b");
    if (!wal_file) wal_file = fopen(wal_filename, "w+b");

    WALRecord recovered{0, BNode(0, false), 0, false};
    if(wal_read_and_validate(wal_file, recovered)){
        DEBUG_COUT<<"WAL recovering page "<<recovered.page_num<<"\n";
        FILE* recovery_file = fopen(filename, "r+b");
        fseek(recovery_file,(long)recovered.page_num*sizeof(BNode), SEEK_SET);
        fwrite(&recovered.data, sizeof(BNode), 1, recovery_file);
        fflush(recovery_file);
        fclose(recovery_file);
    }
    wal_clear(wal_file);

    //main
    FILE* file = fopen(filename, "r+b");
    if(!file){
        file = fopen(filename,"w+b");
    }
    Pager* pager = new Pager(file);
    pager->wal_filename = wal_filename;
    pager->wal_file = wal_file;   // <-- store the persistent handle

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
    }

    entry.freq = f + 1;
    pager->freq_list[entry.freq].push_front(page_num);
    entry.it = pager->freq_list[entry.freq].begin();

    if (!pager->freq_list.empty()) {
        pager->min_freq = pager->freq_list.begin()->first;
    } else {
        pager->min_freq = 0;
    }
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

    DEBUG_COUT << "GET_PAGE " << page_num << " -> page_no=" << node->page_no << "\n";
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

    DEBUG_COUT << "FLUSH_PAGE " << page_num << "\n";

    if (!wal_write(pager->wal_file, page_num, *found->second.node)) {
        std::cerr << "FLUSH_PAGE: WAL write failed for page " << page_num << " — aborting flush\n";
        return;
    }

    if (page_num >= pager->total_pages) pager->total_pages = page_num + 1;

    fseek(pager->file, (long)page_num * sizeof(BNode), SEEK_SET);
    if (fwrite(found->second.node, sizeof(BNode), 1, pager->file) != 1) {
        std::cerr << "FLUSH_PAGE: fwrite failed for page " << page_num << "\n";
        throw std::runtime_error("pager: failed to write page " + std::to_string(page_num) + " to disk");
    }
    fflush(pager->file);

    wal_clear(pager->wal_file);
    found->second.dirty = false;
}

void pager_close(Pager* pager) {
    for (auto& [page_num, entry] : pager->cache) {
        if (entry.dirty) flush_page(pager, page_num);
        delete entry.node;
    }
    pager->cache.clear();
    fclose(pager->file);
    if (pager->wal_file) fclose(pager->wal_file);
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

int allocate_page(Pager* pager){
    int page_num;
    if(!pager->free_pages.empty()){
        page_num = pager->free_pages.back();
        pager->free_pages.pop_back();
    } else {
        page_num = pager->total_pages++;
    }

    // Invalidate any potential stale cache entry for this page_num
    auto found = pager->cache.find(page_num);
    if (found != pager->cache.end()) {
        CacheEntry& entry = found->second;
        int f = entry.freq;
        pager->freq_list[f].erase(entry.it);
        if (pager->freq_list[f].empty()) {
            pager->freq_list.erase(f);
        }
        delete entry.node;
        pager->cache.erase(found);
    }

    // Initialize the page as a fresh, clean BNode in the cache immediately.
    // This prevents get_page from reading stale disk data for a recycled page.
    BNode* node = new BNode(page_num, false);
    CacheEntry entry;
    entry.node = node;
    entry.dirty = true;
    entry.freq = 1;
    pager->freq_list[1].push_front(page_num);
    entry.it = pager->freq_list[1].begin();
    pager->cache[page_num] = entry;

    if (!pager->freq_list.empty()) {
        pager->min_freq = pager->freq_list.begin()->first;
    } else {
        pager->min_freq = 0;
    }

    return page_num;
}

void free_page(Pager* pager, int page_num){
    auto found = pager->cache.find(page_num);
    if (found != pager->cache.end()) {
        CacheEntry& entry = found->second;
        int f = entry.freq;
        pager->freq_list[f].erase(entry.it);
        if (pager->freq_list[f].empty()) {
            pager->freq_list.erase(f);
        }
        if (!pager->freq_list.empty()) {
            pager->min_freq = pager->freq_list.begin()->first;
        } else {
            pager->min_freq = 0;
        }
        delete entry.node;
        pager->cache.erase(found);
    }
    pager->free_pages.push_back(page_num);
}
