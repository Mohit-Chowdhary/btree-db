#include "btree.h"
#include <iostream>
#include <string>

int main(){
    BTree* tree = btree_open("db.dat");

    std::string command;
    while(true){
        std::cout<< "> ";
        std::cin>>command;
        if(command == "INSERT"){
            int key,value;
            std::cin>>key>>value;
            insert(tree,key,value);
        }
        else if(command == "GET"){
            int key;
            std::cin>>key;
            int val = search(tree,key);
            if(val == -1) std::cout<<"Not found\n";
            else std::cout<<val<<"\n";
        }
        else if(command == "EXIT" || command == "END" || command == "X"){
            pager_close(tree->page);
            break;
        } 
        else if(command == "PRINT"){
            print_tree(tree, tree->root_page, 0);
        }
        else if(command == "DELETE"){
            int key; std::cin>>key;
            delete_key(tree, key);
        }
    }

    return 0;
}