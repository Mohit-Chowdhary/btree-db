cmake_minimum_required(VERSION 3.16)

project(Database)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(pybind11_DIR "C:/Users/mohit/AppData/Roaming/Python/Python313/site-packages/pybind11/share/cmake/pybind11")
find_package(pybind11 REQUIRED)

add_executable(database
    src/main.cpp
    src/btree.cpp
    src/pager.cpp
    src/meta.cpp
)

pybind11_add_module(btree_engine
    src/bindings.cpp
    src/btree.cpp
    src/pager.cpp
    src/meta.cpp
)