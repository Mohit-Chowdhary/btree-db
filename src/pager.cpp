#include "pager.h";

Pager* pager_open(const char* filename){

}



BNode* get_page(Pager* pager, int page_num){
    if(pager->cache[page_num] != nullptr){
        return pager->cache[page_num];
    }
    else{
        BNode* node = new BNode(page_num, false);
        fseek(pager->file, page_num *sizeof(BNode), SEEK_SET);
        fread(node, sizeof(BNode), 1, pager->file);
        pager->cache[page_num] = node;
        return pager->cache[page_num];
    }
}


void flush_page(Pager* pager, int page_num);