cmake_minimum_required(VERSION 3.16)
project(Database)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# auto-detect pybind11's cmake dir from whatever python/pip the user has active
execute_process(
    COMMAND python -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
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