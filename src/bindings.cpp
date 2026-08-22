#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "btree.h"

namespace py = pybind11;

PYBIND11_MODULE(btree_engine, m){
    m.doc() = "B+Tree database engine bindings";

    py::class_<BTree>(m, "BTree")
        .def(py::init<const char*, const char*>(), py::arg("filename"), py::arg("meta_filename"))
        .def("insert", &BTree::insert, py::arg("key"), py::arg("value"))
        .def("delete_key", &BTree::delete_key, py::arg("key"))
        .def("search", &BTree::search, py::arg("key"))
        .def("range_query", &BTree::range_query, py::arg("left"), py::arg("right"))
        .def("print_tree", py::overload_cast<>(&BTree::print_tree, py::const_))
        ;

    m.def("set_debug_mode", &set_debug_mode,
          py::arg("enabled"),
          "Enable or disable debug logging");
}