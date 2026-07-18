#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "btree.h"

namespace py = pybind11;

PYBIND11_MODULE(btree_engine, m){
    m.doc() = "B+Tree database engine bindings";

    py::class_<BTree>(m, "BTree");

    m.def("btree_open", &btree_open,
          py::arg("filename"), py::arg("meta_filename"),
          py::return_value_policy::reference,
          "Open or create a B+Tree database file");

    m.def("insert", &insert,
          py::arg("tree"), py::arg("key"), py::arg("value"),
          "Insert a key-value pair");

    m.def("delete_key", &delete_key,
          py::arg("tree"), py::arg("key"),
          "Delete a key");

    m.def("search", &search,
          py::arg("tree"), py::arg("key"),
          "Search for a key, returns value or -1 if not found");

    m.def("range_query", &range_query,
          py::arg("tree"), py::arg("left"), py::arg("right"),
          "Return all key-value pairs where left <= key <= right");

    m.def("btree_close", &btree_close, py::arg("tree"), "Close the database, flushing all pages");

    m.def("print_tree",
      [](BTree* tree) {
            print_tree(tree, tree->root_page, 0);
      },
      py::arg("tree"),
      "Print the B+Tree");
}