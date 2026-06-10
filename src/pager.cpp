#include "pager.h"

Pager* pager_open(const char* filename){
    FILE* file = fopen(filename,"a+b");
    Pager* pager = new Pager(file);
    for(int i=0; i<MAX_PAGES; i++) pager->cache[i] = nullptr;
    fseek(pager->file, 0, SEEK_END);
    int file_size = ftell(pager->file);
    pager->total_pages = file_size / sizeof(BNode);

    return pager;
}


BNode* get_page(Pager* pager, int page_num){
    if(pager->cache[page_num] != nullptr){
        return pager->cache[page_num];
    }
    
    BNode* node = new BNode(page_num, false);

    if(page_num < pager->total_pages){
        fseek(pager->file, page_num *sizeof(BNode), SEEK_SET);
        size_t n = fread(node, sizeof(BNode), 1, pager->file);
    }
    pager->cache[page_num] = node;
    std::cout << "GET_PAGE "
          << page_num
          << " -> page_no="
          << node->page_no
          << "\n";
    return pager->cache[page_num];
}


void flush_page(Pager* pager, int page_num){
    if(pager->cache[page_num] == nullptr) return;
    if(page_num >= pager->total_pages) pager->total_pages = page_num + 1;
    BNode* node  = pager->cache[page_num];

    fseek(pager->file, page_num*sizeof(BNode), SEEK_SET);
    fwrite(node, sizeof(BNode), 1, pager->file);

    fflush(pager->file);
}

void pager_close(Pager* pager){
    for(int i=0; i<MAX_PAGES; i++){
        if(pager->cache[i] != nullptr){
            flush_page(pager,i);
            delete pager->cache[i];
        }
    }

    fclose(pager->file);
    delete pager;
}