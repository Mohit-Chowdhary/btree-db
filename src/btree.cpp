#include "btree.h"

BTree* btree_open(const char* filename){
    Pager* pager = pager_open(filename);
    BTree* tree = new BTree();
    tree->page = pager;
    tree->root_page = 0; // root always page0

    if(pager->total_pages==0){//if its a new db
        BNode* root = get_page(pager,0);
        root->is_leaf = true;
        flush_page(pager,0);
        pager->total_pages = 1;
    }
    return tree;
}


void insert(BTree* tree, int key, int value){
    BNode* node = get_page(tree->page, tree->root_page);
    while(!node->is_leaf){
        int i = 0;
        while(i<node->num_keys && key>node->keys[i]) i++;
        node = get_page(tree->page, node->children[i]);
    }
    if(node->num_keys < ORDER-1){
        int i = node->num_keys-1;
        while(i>=0 && node->keys[i]> key){
            node->keys[i+1] = node->keys[i];
            node->values[i+1] = node->values[i];
            i--;
        }
        node->keys[i+1] = key;
        node->values[i+1] = value;
        node->num_keys++;
        flush_page(tree->page,node->page_no);
    }
    else{
        split_leaf(tree, node, key, value);
    }
}

BNode* find_parent(BTree* tree, int curr_page, int target_page){
    BNode* curr = get_page(tree->page, curr_page);

    if(curr->is_leaf) return nullptr;

    //check if child is target
    for(int i=0; i<=curr->num_keys; i++){
        if(curr->children[i] == target_page) return curr;
    }

    //else recurse
    int i=0;
    BNode* target = get_page(tree->page, target_page);
    while(i<curr->num_keys && target->keys[0] > curr->keys[i]) i++;

    return find_parent(tree, curr->children[i],target_page);
}

void insert_into_parent(BTree* tree, BNode* left, int key, BNode* right){
    // if the split thing was root itslef(happens when not too many elems initlaly when root itslewf stores pages)
    // then the left is the root, check if that is the case first
    if(left->page_no == tree->root_page){
        BNode* new_root = get_page(tree->page, tree->page->total_pages++);
        new_root->is_leaf = false;
        new_root->keys[0] = key;
        new_root->children[0] = left->page_no;
        new_root->children[1] = right->page_no;
        new_root->num_keys = 1;
        tree->root_page = new_root->page_no;
        flush_page(tree->page, new_root->page_no);
        return;
    }

    BNode* parent  = find_parent(tree, tree->root_page, left->page_no);

    if(parent == nullptr){
        throw std::runtime_error("Parent was not found at 'find_parent'.");
    }

    // find pos in parent to insert
    int i = 0;
    while(i<parent->num_keys && parent->children[i] !=left->page_no) i++;

    if(parent->num_keys < ORDER-1){
        for(int j = parent->num_keys; j>i; j--){
            parent->keys[j] = parent->keys[j-1];
            parent->children[j+1] = parent->children[j];
        }
        parent->keys[i] = key;
        parent->children[i+1] = right->page_no;
        parent->num_keys++;
        flush_page(tree->page, parent->page_no);
    }
    else{
        //skull emoji
    }
}

void split_leaf(BTree* tree, BNode* node, int key, int value){
    // create new leaf node
    int new_page_no = tree->page->total_pages;
    BNode* new_node = get_page(tree->page, new_page_no);
    tree->page->total_pages++;
    new_node->is_leaf = true;

    // temporary array with  all keys+values 
    int temp_keys[ORDER];
    int temp_vals[ORDER];
    int inserted =  false;
    int j = 0;
    for(int i=0; i<node->num_keys; i++){
        if(!inserted && key < node->keys[i]){
            temp_keys[j] = key;
            temp_vals[j++] = value;
            inserted = true;
        }
        temp_keys[j] = node->keys[i];
        temp_vals[j++] = node->values[i];
    }
    if(!inserted){
        temp_keys[j] = key;
        temp_vals[j++] = value;
    }

    // split, left and right half
    int split = ORDER/2;
    node->num_keys = split;
    new_node->num_keys = ORDER-split;

    for(int i =0; i<split; i++){
        node->keys[i] = temp_keys[i];
        node->values[i] = temp_vals[i];
    }

    for(int i=0; i<ORDER-split; i++){
        new_node->keys[i] = temp_keys[split+i];
        new_node->values[i] = temp_vals[split+i];
    }

    // linking leaves
    new_node->next_leaf = node->next_leaf;
    node->next_leaf = new_page_no;

    //push middle key to parent
    insert_into_parent(tree, node, new_node->keys[0], new_node);

    flush_page(tree->page,node->page_no);
    flush_page(tree->page,new_node->page_no);
}


int search(BTree* tree,int key){
    BNode* node = get_page(tree->page, tree->root_page);
    // get root node
    while(!node->is_leaf){
        int i = 0;
        while(i<node->num_keys && key>node->keys[i]) i++;
        // check which index to lookinto
        node = get_page(tree->page, node->children[i]);
        // get into that, do this untill a leaf is found
    }
    for(int i=0; i<node->num_keys; i++){
        if(node->keys[i] == key) return node->values[i];
    }

    return -1;
}