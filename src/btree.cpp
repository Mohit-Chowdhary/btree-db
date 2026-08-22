#include "btree.h"
#include "meta.h"
#include <iostream>
#include <string>

void set_debug_mode(bool enabled) {
    g_debug_mode = enabled;
}

BTree::BTree(const char* filename, const char* meta_filename) {
    std::string wal_filename = std::string(filename) + ".wal";
    pager = pager_open(filename, wal_filename.c_str());
    root_page = 0;
    this->meta_filename = meta_filename;

    if (pager->total_pages == 0) { // new database
        PageHandle root(pager, 0);
        root->is_leaf = true;
        root.mark_dirty();
        root.flush();
        pager->total_pages = 1;
        root_page = 0;
        write_meta(meta_filename, {0});
    } else {
        Meta meta = read_meta(meta_filename);
        root_page = meta.root_page;
        DEBUG_COUT << "reopened the db\n";
        DEBUG_COUT << "root page: " << meta.root_page << ", total pages: " << pager->total_pages << std::endl;
    }
}

BTree::~BTree() {
    pager_close(pager); // now just closes the file handle; nothing left dirty to flush
}

void BTree::delete_key(int key) {
    PageHandle node(pager, root_page);
    int j;
    while (!node->is_leaf) {
        j = 0;
        while (j < node->num_keys && key >= node->keys[j]) j++;
        node = PageHandle(pager, node->children[j]);
    }
    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) {
            PageHandle parent = find_parent(root_page, node->page_no);

            for (int j = i; j < node->num_keys - 1; j++) {
                node->keys[j] = node->keys[j + 1];
                node->values[j] = node->values[j + 1];
            }
            node->num_keys--;

            if (parent.valid() && i == 0 && node->num_keys >= (ORDER - 1) / 2) {
                int search_page = node->page_no;

                while (parent.valid()) {
                    int idx = 0;
                    while (idx <= parent->num_keys && parent->children[idx] != search_page) idx++;

                    if (idx > 0 && idx <= parent->num_keys) {
                        parent->keys[idx - 1] = node->keys[0];
                        parent.mark_dirty();
                        parent.flush();
                        break;
                    }
                    search_page = parent->page_no;
                    parent = find_parent(root_page, parent->page_no);
                }
            }
            node.mark_dirty();
            node.flush();

            // check underflow
            int min_keys = (ORDER - 1) / 2;
            if (node->num_keys < min_keys && node->page_no != root_page) {
                handle_underflow(node.get());
            }
            return;
        }
    }

    DEBUG_COUT << "Key not found\n";
}

void BTree::handle_underflow(BNode* node_raw) {
    PageHandle node(pager, node_raw->page_no);
    DEBUG_COUT << "UNDERFLOW node=" << node->page_no << " num_keys=" << node->num_keys << "\n";

    if (node->page_no == root_page) {
        if (node->num_keys == 0 && !node->is_leaf) {
            root_page = node->children[0];
            write_meta(meta_filename.c_str(), {node->children[0]});
        }
        return;
    }

    PageHandle parent = find_parent(root_page, node->page_no);
    if (!parent.valid()) {
        DEBUG_COUT << "NO PARENT\n";
        return;
    }

    DEBUG_COUT << "parent=" << parent->page_no << " num_keys=" << parent->num_keys << "\n";

    // find which index node is in parent
    int idx = 0;
    while (idx <= parent->num_keys && parent->children[idx] != node->page_no) idx++;
    DEBUG_COUT << "idx=" << idx << "\n";

    // try borrowing from left sibling
    if (idx > 0) {
        PageHandle left_sib(pager, parent->children[idx - 1]);
        if (left_sib->num_keys > (ORDER - 1) / 2) {
            if (node->is_leaf) {
                // shift node right to make space for new addition
                for (int j = node->num_keys; j > 0; j--) {
                    node->keys[j] = node->keys[j - 1];
                    node->values[j] = node->values[j - 1];
                }
                // take last left from sibling
                node->keys[0] = left_sib->keys[left_sib->num_keys - 1];
                node->values[0] = left_sib->values[left_sib->num_keys - 1];
                // update parent
                parent->keys[idx - 1] = node->keys[0];
            } else {
                for (int j = node->num_keys; j > 0; j--) {
                    node->keys[j] = node->keys[j - 1];
                    node->children[j + 1] = node->children[j];
                }
                node->children[1] = node->children[0];
                node->keys[0] = parent->keys[idx - 1];
                node->children[0] = left_sib->children[left_sib->num_keys];
                parent->keys[idx - 1] = left_sib->keys[left_sib->num_keys - 1];
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

    if (idx < parent->num_keys) {
        PageHandle right_sib(pager, parent->children[idx + 1]);
        if (right_sib->num_keys > (ORDER - 1) / 2) {
            if (node->is_leaf) {
                // borrow from right
                node->keys[node->num_keys] = right_sib->keys[0]; // pull down from parent
                node->values[node->num_keys] = right_sib->values[0];

                // shift
                for (int j = 0; j < right_sib->num_keys - 1; j++) {
                    right_sib->keys[j] = right_sib->keys[j + 1];
                    right_sib->values[j] = right_sib->values[j + 1];
                }
                parent->keys[idx] = right_sib->keys[0];
            } else {
                node->keys[node->num_keys] = parent->keys[idx];
                node->children[node->num_keys + 1] = right_sib->children[0];
                parent->keys[idx] = right_sib->keys[0];

                for (int j = 0; j < right_sib->num_keys - 1; j++) {
                    right_sib->keys[j] = right_sib->keys[j + 1];
                    right_sib->children[j] = right_sib->children[j + 1];
                }
                right_sib->children[right_sib->num_keys - 1] = right_sib->children[right_sib->num_keys];
            }
            right_sib->num_keys--;
            node->num_keys++;
            right_sib.mark_dirty();
            parent.mark_dirty();
            node.mark_dirty();
            node.flush();
            right_sib.flush();
            parent.flush();
            return;
        }
    }

    // else merge
    if (idx > 0) {
        PageHandle left_sib(pager, parent->children[idx - 1]);
        if (node->is_leaf) {
            for (int j = 0; j < node->num_keys; j++) {
                left_sib->keys[left_sib->num_keys + j] = node->keys[j];
                left_sib->values[left_sib->num_keys + j] = node->values[j];
            }
            left_sib->num_keys += node->num_keys;
            left_sib->next_leaf = node->next_leaf;
        } else {
            // pull separator down
            left_sib->keys[left_sib->num_keys] = parent->keys[idx - 1];
            left_sib->num_keys++;

            for (int j = 0; j < node->num_keys; j++) {
                left_sib->keys[left_sib->num_keys + j] = node->keys[j];
                left_sib->children[left_sib->num_keys + j] = node->children[j];
            }
            left_sib->children[left_sib->num_keys + node->num_keys] = node->children[node->num_keys];
            left_sib->num_keys += node->num_keys;
        }

        for (int j = idx - 1; j < parent->num_keys - 1; j++) {
            parent->keys[j] = parent->keys[j + 1];
            parent->children[j + 1] = parent->children[j + 2];
        }
        parent->num_keys--;
        left_sib.mark_dirty();
        parent.mark_dirty();
        node.mark_dirty();
        node.flush();
        left_sib.flush();
        parent.flush();
        free_page(pager, node.page());
        if (parent->page_no == root_page && parent->num_keys == 0) {
            root_page = left_sib->page_no;
            write_meta(meta_filename.c_str(), {left_sib.page()});
        } else if (parent->num_keys < (ORDER - 1) / 2) {
            handle_underflow(parent.get());
        }
        return;
    } else {
        // right merge
        PageHandle right_sib(pager, parent->children[idx + 1]);
        if (node->is_leaf) {
            for (int j = 0; j < right_sib->num_keys; j++) {
                node->keys[node->num_keys + j] = right_sib->keys[j];
                node->values[node->num_keys + j] = right_sib->values[j];
            }
            node->num_keys += right_sib->num_keys;
            node->next_leaf = right_sib->next_leaf;
        } else {
            node->keys[node->num_keys] = parent->keys[idx];
            node->num_keys++;
            for (int j = 0; j < right_sib->num_keys; j++) {
                node->keys[j + node->num_keys] = right_sib->keys[j];
                node->children[j + node->num_keys] = right_sib->children[j];
            }
            node->children[node->num_keys + right_sib->num_keys] = right_sib->children[right_sib->num_keys];
            node->num_keys += right_sib->num_keys;
        }
        DEBUG_COUT << "RIGHT MERGE: removing parent key at idx=" << idx << " val=" << parent->keys[idx] << "\n";

        for (int j = idx; j < parent->num_keys - 1; j++) {
            parent->keys[j] = parent->keys[j + 1];
            parent->children[j + 1] = parent->children[j + 2];
        }
        parent->num_keys--;
        right_sib.mark_dirty();
        parent.mark_dirty();
        node.mark_dirty();

        node.flush();
        right_sib.flush();
        parent.flush();
        free_page(pager, right_sib.page());
        if (parent->page_no == root_page && parent->num_keys == 0) {
            root_page = node->page_no;
            write_meta(meta_filename.c_str(), {node->page_no});
        } else if (parent->num_keys < (ORDER - 1) / 2) {
            handle_underflow(parent.get());
        }
        return;
    }
}

void BTree::insert(int key, int value) {
    DEBUG_COUT << "INSERT key=" << key << " root_page=" << root_page << "\n";
    PageHandle node(pager, root_page);
    while (!node->is_leaf) {
        int i = 0;
        while (i < node->num_keys && key >= node->keys[i]) i++;
        node = PageHandle(pager, node->children[i]);
    }
    // duplicate check
    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) {
            DEBUG_COUT << "INSERT REJECTED: key=" << key << " already exists\n";
            return;
        }
    }

    if (node->num_keys < ORDER - 1) {
        int i = node->num_keys - 1;
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num_keys++;
        node.mark_dirty();
        node.flush();
    } else {
        split_leaf(node.get(), key, value);
    }
}

PageHandle BTree::find_parent(int curr_page, int target_page) {
    if (target_page == root_page) {
        return PageHandle(nullptr, -1);
    }
    // Load target page once to find its first key
    PageHandle target(pager, target_page);
    if (!target.valid() || target->num_keys == 0) {
        return PageHandle(nullptr, -1);
    }
    int target_key = target->keys[0];
    return find_parent_helper(curr_page, target_page, target_key);
}

PageHandle BTree::find_parent_helper(int curr_page, int target_page, int target_key) {
    PageHandle curr(pager, curr_page);
    if (!curr.valid() || curr->is_leaf) {
        return PageHandle(nullptr, -1);
    }

    // Check if any child is target
    for (int i = 0; i <= curr->num_keys; i++) {
        if (curr->children[i] == target_page) {
            return curr;
        }
    }

    // Otherwise, recurse guided by target_key
    int i = 0;
    while (i < curr->num_keys && target_key >= curr->keys[i]) {
        i++;
    }
    return find_parent_helper(curr->children[i], target_page, target_key);
}

void BTree::insert_into_parent(BNode* left, int key, BNode* right) {
    DEBUG_COUT << "insert_into_parent total_pages=" << pager->total_pages << "\n";
    if (left->page_no == root_page) {
        DEBUG_COUT << "ROOT SPLIT: old root=" << left->page_no << "\n";
        int new_root_page = allocate_page(pager);
        PageHandle new_root(pager, new_root_page);
        new_root->is_leaf = false;
        new_root->keys[0] = key;
        new_root->children[0] = left->page_no;
        new_root->children[1] = right->page_no;
        new_root->num_keys = 1;
        DEBUG_COUT << "allocated root page="
                  << pager->total_pages - 1
                  << "\n";

        DEBUG_COUT << "new_root->page_no="
                  << new_root->page_no
                  << "\n";
        root_page = new_root->page_no;
        write_meta(meta_filename.c_str(), {new_root->page_no});
        DEBUG_COUT << "NEW ROOT: " << root_page << "\n";
        new_root.mark_dirty();
        new_root.flush();
        return;
    }

    PageHandle parent = find_parent(root_page, left->page_no);

    if (!parent.valid()) {
        throw std::runtime_error("Parent was not found at 'find_parent'.");
    }

    int i = 0;
    while (i < parent->num_keys && parent->children[i] != left->page_no) i++;

    if (parent->num_keys < ORDER - 1) {
        for (int j = parent->num_keys; j > i; j--) {
            parent->keys[j] = parent->keys[j - 1];
            parent->children[j + 1] = parent->children[j];
        }
        parent->keys[i] = key;
        parent->children[i + 1] = right->page_no;
        parent->num_keys++;
        parent.mark_dirty();
        parent.flush();
    } else {
        split_internal(parent.get(), key, right);
    }
}

void BTree::split_internal(BNode* node_raw, int key, BNode* right) {
    int temp_keys[ORDER + 1];
    int temp_children[ORDER + 2];
    PageHandle node(pager, node_raw->page_no);

    int i = 0;
    while (i < node->num_keys && key >= node->keys[i]) i++;

    for (int j = 0; j < i; j++) {
        temp_keys[j] = node->keys[j];
        temp_children[j] = node->children[j];
    }
    temp_keys[i] = key;
    temp_children[i] = node->children[i];
    temp_children[i + 1] = right->page_no;
    for (int j = i + 1; j <= node->num_keys; j++) {
        temp_keys[j] = node->keys[j - 1];
        temp_children[j + 1] = node->children[j];
    }

    int split = ORDER / 2;
    int pushed_key = temp_keys[split];

    node->num_keys = split;
    for (int j = 0; j < split; j++) {
        node->keys[j] = temp_keys[j];
        node->children[j] = temp_children[j];
    }
    node->children[split] = temp_children[split];

    int new_page_no = allocate_page(pager);
    PageHandle new_node(pager, new_page_no);
    new_node->is_leaf = false;
    new_node->num_keys = ORDER - split - 1;

    for (int j = 0; j < new_node->num_keys; j++) {
        new_node->keys[j] = temp_keys[split + 1 + j];
        new_node->children[j] = temp_children[split + 1 + j];
    }
    new_node->children[new_node->num_keys] = temp_children[ORDER];

    node.mark_dirty();
    new_node.mark_dirty();
    node.flush();
    new_node.flush();

    insert_into_parent(node.get(), pushed_key, new_node.get());
}

void BTree::split_leaf(BNode* node_raw, int key, int value) {
    PageHandle node(pager, node_raw->page_no);
    DEBUG_COUT << "SPLITTING leaf page=" << node->page_no << " num_keys=" << node->num_keys << "\n";
    DEBUG_COUT << "Keys before split: ";
    for (int i = 0; i < node->num_keys; i++) DEBUG_COUT << node->keys[i] << " ";
    DEBUG_COUT << "\n";

    int new_page_no = allocate_page(pager);
    DEBUG_COUT << "got page no: " << new_page_no << "\n";

    PageHandle new_node(pager, new_page_no);
    DEBUG_COUT << "new_node init\n";

    DEBUG_COUT << "total_pages: " << pager->total_pages << "\n";
    new_node->is_leaf = true;

    int temp_keys[ORDER + 1];
    int temp_vals[ORDER + 1];
    DEBUG_COUT << "About to fill temp array, num_keys=" << node->num_keys << " key=" << key << "\n";
    if (g_debug_mode) std::cout.flush();
    bool inserted = false;
    int j = 0;
    for (int i = 0; i < node->num_keys; i++) {
        if (!inserted && key < node->keys[i]) {
            temp_keys[j] = key;
            temp_vals[j++] = value;
            inserted = true;
        }
        temp_keys[j] = node->keys[i];
        temp_vals[j++] = node->values[i];
    }
    if (!inserted) {
        temp_keys[j] = key;
        temp_vals[j++] = value;
    }
    DEBUG_COUT << "Temp filled: ";
    for (int i = 0; i <= node->num_keys; i++) DEBUG_COUT << temp_keys[i] << " ";
    DEBUG_COUT << "\n";
    if (g_debug_mode) std::cout.flush();

    int split = ORDER / 2;
    node->num_keys = split;
    new_node->num_keys = ORDER - split;

    for (int i = 0; i < split; i++) {
        node->keys[i] = temp_keys[i];
        node->values[i] = temp_vals[i];
    }

    for (int i = 0; i < ORDER - split; i++) {
        new_node->keys[i] = temp_keys[split + i];
        new_node->values[i] = temp_vals[split + i];
    }

    new_node->next_leaf = node->next_leaf;
    node->next_leaf = new_page_no;

    insert_into_parent(node.get(), new_node->keys[0], new_node.get());

    node.mark_dirty();
    new_node.mark_dirty();
    node.flush();
    new_node.flush();
    DEBUG_COUT << "New page=" << new_page_no << " split=" << split << "\n";
    DEBUG_COUT << "Left keys after: ";
    for (int i = 0; i < node->num_keys; i++) DEBUG_COUT << node->keys[i] << " ";
    DEBUG_COUT << "\n";
    DEBUG_COUT << "Right keys after: ";
    for (int i = 0; i < new_node->num_keys; i++) DEBUG_COUT << new_node->keys[i] << " ";
    DEBUG_COUT << "\n";
}

int BTree::search(int key) {
    PageHandle node(pager, root_page);
    while (!node->is_leaf) {
        int i = 0;
        while (i < node->num_keys && key >= node->keys[i]) i++;
        node = PageHandle(pager, node->children[i]);
    }
    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) return node->values[i];
    }
    return -1;
}

void BTree::print_tree(int page_num, int depth) const {
    if (page_num < 0 || page_num >= pager->total_pages) {
        std::cout << std::string(depth * 4, ' ')
                  << "INVALID PAGE " << page_num << "\n";
        return;
    }

    PageHandle node(pager, page_num);

    std::cout << std::string(depth * 4, ' ');
    std::cout << (node->is_leaf ? "LEAF " : "INTERNAL ");
    std::cout << "page=" << page_num;

    std::cout << " keys=[";
    for (int i = 0; i < node->num_keys; i++) {
        std::cout << node->keys[i];
        if (i != node->num_keys - 1)
            std::cout << ",";
    }
    std::cout << "]";

    if (node->is_leaf) {
        std::cout << " next=" << node->next_leaf;
    } else {
        std::cout << " children=[";
        for (int i = 0; i <= node->num_keys; i++) {
            std::cout << node->children[i];
            if (i != node->num_keys)
                std::cout << ",";
        }
        std::cout << "]";
    }

    std::cout << "\n";

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            if (node->children[i] != -1)
                print_tree(node->children[i], depth + 1);
        }
    }
}

void BTree::print_tree() const {
    print_tree(root_page, 0);
}

std::vector<std::pair<int, int>> BTree::range_query(int left, int right) {
    std::vector<std::pair<int, int>> results;

    if (left > right) return results;

    PageHandle node(pager, root_page);
    while (!node->is_leaf) {
        int i = 0;
        while (i < node->num_keys && left >= node->keys[i]) i++;
        node = PageHandle(pager, node->children[i]);
    }

    while (true) {
        for (int i = 0; i < node->num_keys; i++) {
            if (node->keys[i] >= left && node->keys[i] <= right) {
                results.push_back({node->keys[i], node->values[i]});
            }
            if (node->keys[i] > right) return results;
        }
        if (node->next_leaf == -1) break;
        node = PageHandle(pager, node->next_leaf);
    }

    return results;
}