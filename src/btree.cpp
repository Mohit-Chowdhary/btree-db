#include "btree.h"
#include "meta.h"

BTree* btree_open(const char* filename, const char* meta_filename){
    Pager* pager = pager_open(filename);
    BTree* tree = new BTree();
    tree->page = pager;
    tree->root_page = 0; // root always page0
    tree->meta_filename=meta_filename;  

    if(pager->total_pages==0){//if its a new db 
        PageHandle root(pager,0);
        root->is_leaf = true;
        root.mark_dirty();
        root.flush();
        pager->total_pages = 1;
        tree->root_page = 0;
        write_meta(meta_filename,{0});
    }
    else{
        Meta meta = read_meta(meta_filename);
        tree->root_page = meta.root_page;
        //tree->is_leaf 
        std::cout<<"reopened the db\n";
        std::cout<<"root page: "<<meta.root_page<<", total pages: "<<pager->total_pages<<std::endl;
    }
    return tree;
}

void delete_key(BTree* tree, int key){
    PageHandle node(tree->page, tree->root_page);
    // get root node
    int j;
    while(!node->is_leaf){
        j = 0;
        while(j<node->num_keys && key>=node->keys[j]) j++;
        // check which index to lookinto
        node = PageHandle(tree->page, node->children[j]);
        // get into that, do this untill a leaf is found
    }
    for(int i=0; i<node->num_keys; i++){
        if(node->keys[i] == key){
            PageHandle parent = find_parent(tree, tree->root_page, node->page_no);

            for(int j = i; j < node->num_keys-1; j++){
                node->keys[j] = node->keys[j+1];
                node->values[j] = node->values[j+1];
            }
            node->num_keys--;

            if(parent.valid() && i == 0 && node->num_keys >= (ORDER-1)/2){
                int search_page = node->page_no;

                while(parent.valid()){
                    int idx = 0;
                    while(idx<= parent->num_keys && parent->children[idx] != search_page    ) idx++;

                    if(idx>0 && idx<=parent->num_keys){
                        parent->keys[idx-1] = node->keys[0];
                        parent.mark_dirty();
                        parent.flush();
                        break;
                    }
                    search_page = parent->page_no;
                    parent = find_parent(tree,tree->root_page, parent->page_no);
                }
            }
            node.mark_dirty();
            node.flush();

            //check underflow
            int min_keys = (ORDER-1)/2;
            if(node->num_keys < min_keys && node->page_no != tree->root_page){
                handle_underflow(tree, node.get());
            }
            return;
        }
    }

    std::cout<<"Key not found\n";
}

void handle_underflow(BTree* tree, BNode* node_raw){
    PageHandle node(tree->page,node_raw->page_no);
    std::cout << "UNDERFLOW node=" << node->page_no << " num_keys=" << node->num_keys << "\n";

    if(node->page_no == tree->root_page){
        if(node->num_keys == 0 && !node->is_leaf){
            tree->root_page = node->children[0];
            write_meta(tree->meta_filename, {node->children[0]}); 
        }
        return;
    }

    PageHandle parent = find_parent(tree, tree->root_page, node->page_no);
    if(!parent.valid()){   std::cout << "NO PARENT\n";    return;}

    std::cout << "parent=" << parent->page_no << " num_keys=" << parent->num_keys << "\n";

    // find which index node is in parent
    int idx = 0;
    while(idx <= parent->num_keys && parent->children[idx]!=node->page_no) idx++;
    std::cout << "idx=" << idx << "\n";

    // try borrowing from left sibling
    if(idx>0){
        PageHandle left_sib(tree->page, parent->children[idx-1]);
        if(left_sib->num_keys > (ORDER-1)/2){
            if(node->is_leaf){
                // shift node right to make space for new addition
                for(int j = node->num_keys; j>0; j--){
                    node->keys[j] = node->keys[j-1];
                    node->values[j] = node->values[j-1];
                }
                //take last left from sibling
                node->keys[0] = left_sib->keys[left_sib->num_keys -1];
                node->values[0] = left_sib->values[left_sib->num_keys -1];
                //update parent
                parent->keys[idx-1] = node->keys[0];
            }
            else{
                for(int j = node->num_keys; j>0; j--){
                    node->keys[j] = node->keys[j-1];
                    node->children[j+1] = node->children[j];
                }
                node->children[1] = node->children[0];
                node->keys[0] = parent->keys[idx-1];
                node->children[0] = left_sib->children[left_sib->num_keys];
                parent->keys[idx-1] = left_sib->keys[left_sib->num_keys-1];
            }

            left_sib->num_keys--;
            node->num_keys++;
            left_sib.mark_dirty();
            parent.mark_dirty();
            node.mark_dirty();
            node.flush();
            left_sib.flush();
            parent.flush();
            return;
        }
    }

    if(idx<parent->num_keys){
        PageHandle right_sib(tree->page, parent->children[idx+1]);   
        if(right_sib->num_keys > (ORDER-1)/2){
            if(node->is_leaf){
                // borrow form right
                node->keys[node->num_keys] = right_sib->keys[0];         //pull down from parent
                node->values[node->num_keys] = right_sib->values[0];

                //shift
                for(int j=0; j<right_sib->num_keys-1; j++){
                    right_sib->keys[j] = right_sib->keys[j+1];
                    right_sib->values[j] = right_sib->values[j+1];
                }
                parent->keys[idx] = right_sib->keys[0];
            }
            else{
                node->keys[node->num_keys] = parent->keys[idx];
                node->children[node->num_keys+1] = right_sib->children[0];
                parent->keys[idx] = right_sib->keys[0];

                for(int j=0; j<right_sib->num_keys-1; j++){
                    right_sib->keys[j] = right_sib->keys[j+1];
                    right_sib->children[j] = right_sib->children[j+1];
                }
                right_sib->children[right_sib->num_keys-1] = right_sib->children[right_sib->num_keys];
            }
            right_sib->num_keys--;
            node->num_keys++;
            right_sib.mark_dirty();
            parent.mark_dirty();
            node.mark_dirty();
            //update parent
            node.flush();
            right_sib.flush();
            parent.flush();
            return;
        }
    }

    //else merge
    if(idx>0){
        PageHandle left_sib(tree->page, parent-> children[idx-1]);
        if(node->is_leaf){
            for(int j = 0; j<node->num_keys; j++){
                left_sib->keys[left_sib->num_keys + j] = node->keys[j];
                left_sib->values[left_sib->num_keys + j] = node->values[j];
            }
            left_sib->num_keys += node->num_keys;
            left_sib->next_leaf = node->next_leaf;
        }
        else{
            // pull seperator down
            left_sib->keys[left_sib->num_keys] = parent->keys[idx-1];
            left_sib->num_keys++;
            
            for(int j=0; j<node->num_keys; j++){
                left_sib->keys[left_sib->num_keys + j] = node->keys[j];
                left_sib->children[left_sib->num_keys + j] = node->children[j];
            }
            left_sib->children[left_sib->num_keys + node->num_keys] = node->children[node->num_keys];
            left_sib->num_keys += node->num_keys;
        }

        //remove seperator from parents, and remove "node" from parent's children
        // shift all leeft
        for(int j = idx-1; j < parent->num_keys-1; j++){
            parent->keys[j] = parent->keys[j+1];
            parent->children[j+1] = parent->children[j+2];
        }
        parent->num_keys--;
        left_sib.mark_dirty();
        parent.mark_dirty();
        node.mark_dirty();
        node.flush();
        left_sib.flush();
        parent.flush();
        if(parent->page_no == tree->root_page && parent->num_keys==0){
            tree->root_page = left_sib->page_no;
            write_meta(tree->meta_filename, {left_sib.page()}); 
        }
        else if(parent->num_keys < (ORDER-1)/2)
            handle_underflow(tree,parent.get());
        return;
    }
    
    else{
        //right
        PageHandle right_sib(tree->page, parent->children[idx+1]);
        if(node->is_leaf){
            for(int j=0; j<right_sib->num_keys; j++){
                node->keys[node->num_keys + j] = right_sib->keys[j];
                node->values[node->num_keys + j] = right_sib->values[j];
            }
            node->num_keys += right_sib->num_keys;
            node->next_leaf = right_sib->next_leaf;
        }
        else{
            node->keys[node->num_keys] = parent->keys[idx];
            node->num_keys++;
            for(int j = 0; j<right_sib->num_keys; j++){
                node->keys[j+node->num_keys] = right_sib->keys[j];
                node->children[j+node->num_keys] = right_sib->children[j];
            }
            node->children[node->num_keys + right_sib->num_keys] = right_sib->children[right_sib->num_keys];
            node->num_keys += right_sib->num_keys;
        }
        std::cout << "RIGHT MERGE: removing parent key at idx=" << idx << " val=" << parent->keys[idx] << "\n";
        
        for(int j = idx; j < parent->num_keys - 1; j++){
            parent->keys[j] = parent->keys[j+1];
            parent->children[j+1] = parent->children[j+2];
        }
        parent->num_keys--;
        right_sib.mark_dirty();
        parent.mark_dirty();
        node.mark_dirty();

        node.flush();
        right_sib.flush();
        parent.flush();
        if(parent->page_no == tree->root_page && parent->num_keys == 0){
            tree->root_page = node->page_no; // or right_sib for right merge
            write_meta(tree->meta_filename, {node->page_no}); 
        }
        else if(parent->num_keys < (ORDER-1)/2)
            handle_underflow(tree, parent.get());
        return;
    }
}


void insert(BTree* tree, int key, int value){
    std::cout << "INSERT key=" << key << " root_page=" << tree->root_page << "\n";
    PageHandle node(tree->page, tree->root_page);
    while(!node->is_leaf){
        int i = 0;
        while(i<node->num_keys && key>=node->keys[i]) i++;
        node = PageHandle(tree->page, node->children[i]);
    }
    //duplicate check
    for(int i = 0; i < node->num_keys; i++){
        if(node->keys[i] == key){
            std::cout << "INSERT REJECTED: key=" << key << " already exists\n";
            return;
        }
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
        node.mark_dirty();
        node.flush();
    }
    else{
        split_leaf(tree, node.get(), key, value);
    }
}


//start search form root, find where target page is
// curr_page=root/main whatver
PageHandle find_parent(BTree* tree, int curr_page, int target_page){
    PageHandle curr(tree->page, curr_page);

    if(curr->is_leaf) return PageHandle(nullptr,-1);

    //check if child is target
    for(int i=0; i<=curr->num_keys; i++){
        if(curr->children[i] == target_page) return curr;
    }

    //else recurse
    int i=0;
    PageHandle target(tree->page, target_page);
    while(i<curr->num_keys && target->keys[0] >= curr->keys[i]) i++;

    return find_parent(tree, curr->children[i],target_page);
}

void insert_into_parent(BTree* tree, BNode* left, int key, BNode* right){
    std::cout << "insert_into_parent total_pages=" << tree->page->total_pages << "\n";
    // if the split thing was root itslef(happens when not too many elems initlaly when root itslewf stores pages)
    // then the left is the root, check if that is the case first
    if(left->page_no == tree->root_page){
        std::cout << "ROOT SPLIT: old root=" << left->page_no << "\n";
        int new_root_page = tree->page->total_pages;
        PageHandle new_root(tree->page, new_root_page);
        tree->page->total_pages++;
        new_root->is_leaf = false;
        new_root->keys[0] = key;
        new_root->children[0] = left->page_no;
        new_root->children[1] = right->page_no;
        new_root->num_keys = 1;
        std::cout << "allocated root page="
            << tree->page->total_pages-1
            << "\n";

        std::cout << "new_root->page_no="
            << new_root->page_no
            << "\n";
        tree->root_page = new_root->page_no;
        write_meta(tree->meta_filename, {new_root->page_no}); 
        std::cout << "NEW ROOT: " << tree->root_page << "\n";   
        new_root.mark_dirty();
        new_root.flush();
        return;
    }

    PageHandle parent  = find_parent(tree, tree->root_page, left->page_no);

    if(!parent.valid()){
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
        parent.mark_dirty();
        parent.flush();
    }
    else{
        split_internal(tree, parent.get(), key, right);
    }
}

void split_internal(BTree* tree, BNode* node_raw, int key, BNode* right){
    int temp_keys[ORDER+1];
    int temp_children[ORDER+2];
    PageHandle node(tree->page, node_raw->page_no);

    //find insert
    int i = 0;
    while(i < node->num_keys && key >= node->keys[i]) i++;

    for(int j=0; j<i; j++){
        temp_keys[j] = node->keys[j];
        temp_children[j] = node->children[j];
    }
    temp_keys[i] = key;
    temp_children[i] = node->children[i];
    temp_children[i+1] = right->page_no;
    for(int j=i+1; j<= node->num_keys; j++){
        temp_keys[j] = node->keys[j-1];
        temp_children[j+1] = node->children[j];
    }

    //split
    int split = ORDER/2;
    int pushed_key = temp_keys[split];

    // left gets "split" no of keys
    node->num_keys = split;
    for(int j=0; j<split; j++){
        node->keys[j] = temp_keys[j];
        node->children[j] = temp_children[j];
    }
    node->children[split] = temp_children[split];

    // right gets rest
    int new_root_page = tree->page->total_pages;
    PageHandle new_node(tree->page, new_root_page);
    tree->page->total_pages++;
    new_node->is_leaf = false;
    new_node->num_keys = ORDER-split-1;

    for(int j=0; j<new_node->num_keys; j++){
        new_node->keys[j] = temp_keys[split+1+j];
        new_node->children[j] = temp_children[split+1+j];
    }
    new_node->children[new_node->num_keys] = temp_children[ORDER];

    node.mark_dirty();
    new_node.mark_dirty();
    node.flush();
    new_node.flush();

    insert_into_parent(tree, node.get(), pushed_key, new_node.get());
}

void split_leaf(BTree* tree, BNode* node_raw, int key, int value){
    // create new leaf node
    PageHandle node(tree->page, node_raw->page_no);
    std::cout << "SPLITTING leaf page=" << node->page_no << " num_keys=" << node->num_keys << "\n";
    std::cout << "Keys before split: ";    
    for(int i=0;i<node->num_keys;i++) std::cout << node->keys[i] << " ";
    std::cout << "\n";

    // add page no
    int new_page_no = tree->page->total_pages;
    std::cout<<"got page no: "<<new_page_no<<"\n";

    // make new node 
    PageHandle new_node(tree->page, new_page_no);
    std::cout<<"new_node init\n";

    // incement total pages
    tree->page->total_pages++;
    std::cout<<"total_pages: "<<tree->page->total_pages<<"\n";
    new_node->is_leaf = true;

    // temporary array with  all keys+values 
    int temp_keys[ORDER+1];
    int temp_vals[ORDER+1];
    std::cout << "About to fill temp array, num_keys=" << node->num_keys << " key=" << key << "\n";
    std::cout.flush();
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
    std::cout << "Temp filled: ";
    for(int i=0;i<=node->num_keys;i++) std::cout << temp_keys[i] << " ";
    std::cout << "\n";
    std::cout.flush();

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
    insert_into_parent(tree, node.get(), new_node->keys[0], new_node.get());

    node.mark_dirty();
    new_node.mark_dirty();
    node.flush();
    new_node.flush();
    std::cout << "New page=" << new_page_no << " split=" << split << "\n";
    std::cout << "Left keys after: ";
    for(int i=0;i<node->num_keys;i++) std::cout << node->keys[i] << " ";
    std::cout << "\n";
    std::cout << "Right keys after: ";
    for(int i=0;i<new_node->num_keys;i++) std::cout << new_node->keys[i] << " ";
    std::cout << "\n";
}


int search(BTree* tree,int key){
    PageHandle node(tree->page, tree->root_page);
    // get root node
    while(!node->is_leaf){
        int i = 0;
        while(i<node->num_keys && key>=node->keys[i]) i++;
        // check which index to lookinto
        node = PageHandle(tree->page, node->children[i]);
        // get into that, do this untill a leaf is found
    }
    for(int i=0; i<node->num_keys; i++){
        if(node->keys[i] == key) return node->values[i];
    }

    return -1;
}

void print_tree(BTree* tree, int page_num, int depth) {
    if(page_num < 0 || page_num >= tree->page->total_pages) {
        std::cout << std::string(depth * 4, ' ')
                  << "INVALID PAGE " << page_num << "\n";
        return;
    }

    PageHandle node(tree->page, page_num);

    std::cout << std::string(depth * 4, ' ');

    std::cout << (node->is_leaf ? "LEAF " : "INTERNAL ");
    std::cout << "page=" << page_num;

    std::cout << " keys=[";
    for(int i = 0; i < node->num_keys; i++) {
        std::cout << node->keys[i];
        if(i != node->num_keys - 1)
            std::cout << ",";
    }
    std::cout << "]";

    if(node->is_leaf) {
        std::cout << " next=" << node->next_leaf;
    } else {
        std::cout << " children=[";
        for(int i = 0; i <= node->num_keys; i++) {
            std::cout << node->children[i];
            if(i != node->num_keys)
                std::cout << ",";
        }
        std::cout << "]";
    }

    std::cout << "\n";

    if(!node->is_leaf) {
        for(int i = 0; i <= node->num_keys; i++) {
            if(node->children[i] != -1)   // assuming unused children are -1
                print_tree(tree, node->children[i], depth + 1);
        }
    }
}

std::vector<std::pair<int,int>> range_query(BTree* tree, int left, int right){
    std::vector<std::pair<int,int>> results;

    if(left>right) return results;

    PageHandle node(tree->page, tree->root_page);
    while(!node->is_leaf){
        int i=0;
        while(i<node->num_keys && left>=node->keys[i]) i++;
        node = PageHandle(tree->page, node->children[i]);
    }

    while(true){
        for(int i=0; i<node->num_keys; i++){
            if(node->keys[i]>=left && node->keys[i] <= right){
                results.push_back({node->keys[i], node->values[i]});
            }
            if(node->keys[i]>right) return results;
        }
        if(node->next_leaf == -1) break;
        node = PageHandle(tree->page, node->next_leaf);
    }

    return results;
}