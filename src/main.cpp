#include "btree.h"
#include <iostream>
#include <string>
#include <chrono>

int main(){
    // Use the object-oriented constructor. We can also enable debug mode here if needed.
    // By default, debug mode is false (disabled).
    set_debug_mode(false);

    BTree tree("benchmark.db", "benchmark_meta.db");

    std::string command;
    while(true){
        std::cout<< "> ";
        std::cin>>command;
        if(command == "INSERT"){
            int key,value;
            std::cin>>key>>value;
            tree.insert(key,value);
        }
        else if(command == "GET"){
            int key;
            std::cin>>key;
            int val = tree.search(key);
            if(val == -1) std::cout<<"Not found\n";
            else std::cout<<val<<"\n";
        }
        else if(command == "EXIT" || command == "END" || command == "X"){
            break;
        } 
        else if(command == "PRINT"){
            tree.print_tree();
        }
        else if(command == "RANGE"){
            int left,right; std::cin>>left>>right;
            std::vector<std::pair<int,int>> p =  tree.range_query(left,right);

            std::cout<<"Range query:\n";
            for(auto &[x,y]: p) std::cout<<"Key: "<<x<<", Val: "<<y<<"\n";
        }
        else if(command == "DELETE"){
            int key; std::cin>>key;
            tree.delete_key(key);
        }
        else if(command == "ADD"){
            auto start = std::chrono::steady_clock::now();

            for(int i = 1; i <= 10000; i++){
                tree.insert(i, i);
            }

            auto end = std::chrono::steady_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            );

            std::cout << "Insert time: " << duration.count() << " ms\n";
        }
    }

    return 0;
}