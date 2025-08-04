#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/system/node_system_dl.hpp"
#include "nodes/core/node_exec_eager.hpp"


namespace nb = nanobind;
using namespace USTC_CG;

NB_MODULE(nodes_system_py, m)
{
    // NodeTreeExecutor
    nb::class_<NodeTreeExecutor>(m, "NodeTreeExecutor")
        .def("execute", 
            static_cast<void (NodeTreeExecutor::*)(NodeTree*, Node*)>(&NodeTreeExecutor::execute),
            nb::arg("tree"), nb::arg("required_node") = nullptr,
            "Execute the node tree with specific tree and optional required node");

    // Base NodeSystem class
    nb::class_<NodeSystem>(m, "NodeSystem")
        .def(
            "init",
            static_cast<void (NodeSystem::*)()>(&NodeSystem::init),
            "Initialize the node system with a default empty tree")
        .def(
            "load_configuration",
            &NodeSystem::load_configuration,
            nb::arg("config"),
            "Load node tree configuration from a file")
        .def(
            "execute",
            &NodeSystem::execute,
            nb::arg("is_ui_execution") = false,
            nb::arg("required_node") = nullptr,
            "Execute the node tree")
        .def(
            "get_node_tree",
            &NodeSystem::get_node_tree,
            nb::rv_policy::reference_internal,
            "Get the node tree associated with this system")
        .def(
            "get_node_tree_executor",
            &NodeSystem::get_node_tree_executor,
            nb::rv_policy::reference_internal,
            "Get the node tree executor")
        .def_rw(
            "allow_ui_execution",
            &NodeSystem::allow_ui_execution,
            "Flag to allow execution triggered by UI interactions")
        .def("finalize", &NodeSystem::finalize, "Finalize the node system");

    // NodeDynamicLoadingSystem - concrete implementation
    nb::class_<NodeDynamicLoadingSystem, NodeSystem>(
        m, "NodeDynamicLoadingSystem")
        .def(
            "load_configuration",
            &NodeDynamicLoadingSystem::load_configuration,
            nb::arg("config"),
            "Load node tree configuration and dynamic libraries from a file");

    // Factory function
    m.def(
        "create_dynamic_loading_system",
        &create_dynamic_loading_system,
        nb::rv_policy::take_ownership,
        "Create a NodeSystem instance that supports dynamic loading of nodes");
}
