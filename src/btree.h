#ifndef BTREE_H
#define BTREE_H

#include "pager.h"
#include <vector>
#include <utility>
#include <string>

void set_debug_mode(bool enabled);

class BTree {
private:
    Pager* pager;
    int root_page;
    std::string meta_filename;

    // Internal helper methods
    PageHandle find_parent(int curr_page, int target_page);
    PageHandle find_parent_helper(int curr_page, int target_page, int target_key);
    void insert_into_parent(BNode* left, int key, BNode* right);
    void split_leaf(BNode* node, int key, int value);
    void split_internal(BNode* node, int key, BNode* right);
    void handle_underflow(BNode* node);

public:
    BTree(const char* filename, const char* meta_filename);
    ~BTree();

    void insert(int key, int value);
    int search(int key);
    std::vector<std::pair<int, int>> range_query(int left, int right);
    void delete_key(int key);
    
    void print_tree(int page_num, int depth) const;
    void print_tree() const; // Print from root

    // Getter methods
    int get_root_page() const { return root_page; }
    Pager* get_pager() const { return pager; }
    std::string get_meta_filename() const { return meta_filename; }
};



#endif