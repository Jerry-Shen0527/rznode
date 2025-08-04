#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "nodes/core/node_link.hpp"
#include "nodes/core/node_tree.hpp"

namespace nb = nanobind;

using namespace USTC_CG;

NB_MODULE(nodes_core_py, m)
{
    // Basic enums
    nb::enum_<PinKind>(m, "PinKind")
        .value("Input", PinKind::Input)
        .value("Output", PinKind::Output);

    // ID types - simplified as opaque types
    nb::class_<NodeId>(m, "NodeId")
        .def("__bool__", [](const NodeId& id) { return bool(id); })
        .def("__eq__", [](const NodeId& a, const NodeId& b) { return a == b; });

    nb::class_<SocketID>(m, "SocketID")
        .def("__bool__", [](const SocketID& id) { return bool(id); })
        .def("__eq__", [](const SocketID& a, const SocketID& b) {
            return a == b;
        });

    nb::class_<LinkId>(m, "LinkId")
        .def("__bool__", [](const LinkId& id) { return bool(id); })
        .def("__eq__", [](const LinkId& a, const LinkId& b) { return a == b; });

    // NodeSocket - core socket functionality
    nb::class_<NodeSocket>(m, "NodeSocket")
        .def_prop_ro(
            "identifier",
            [](const NodeSocket& s) { return std::string(s.identifier); })
        .def_prop_ro(
            "ui_name",
            [](const NodeSocket& s) { return std::string(s.ui_name); })
        .def_ro("ID", &NodeSocket::ID)
        .def_ro("in_out", &NodeSocket::in_out)
        .def_ro("optional", &NodeSocket::optional)
        .def_prop_ro(
            "node",
            [](const NodeSocket& s) { return s.node; },
            nb::rv_policy::reference)
        .def_prop_ro(
            "connected_sockets",
            [](const NodeSocket& s) { return s.directly_linked_sockets; },
            nb::rv_policy::reference_internal);  // Node - core node
                                                 // functionality
    nb::class_<Node>(m, "Node")
        .def_prop_ro("name", &Node::getName)
        .def_ro("ID", &Node::ID)
        .def_rw("ui_name", &Node::ui_name)
        .def_prop_ro(
            "inputs",
            [](const Node& n) { return n.get_inputs(); },
            nb::rv_policy::reference_internal)
        .def_prop_ro(
            "outputs",
            [](const Node& n) { return n.get_outputs(); },
            nb::rv_policy::reference_internal)
        .def(
            "get_input_socket",
            &Node::get_input_socket,
            nb::rv_policy::reference)
        .def(
            "get_output_socket",
            &Node::get_output_socket,
            nb::rv_policy::reference)
        .def("get_input_connections", &Node::getInputConnections)
        .def("get_output_connections", &Node::getOutputConnections)
        .def("is_valid", &Node::valid)
        .def("is_node_group", &Node::is_node_group)
        .def(
            "group_add_socket",
            &Node::group_add_socket,
            nb::arg("group_identifier"),
            nb::arg("socket_type"),
            nb::arg("identifier"),
            nb::arg("ui_name"),
            nb::arg("in_out"),
            nb::rv_policy::reference)
        .def(
            "group_remove_socket",
            &Node::group_remove_socket,
            nb::arg("group_identifier"),
            nb::arg("identifier"),
            nb::arg("in_out"),
            nb::arg("is_recursive_call") = false);

    // NodeLink - connection between sockets
    nb::class_<NodeLink>(m, "NodeLink")
        .def_ro("ID", &NodeLink::ID)
        .def_prop_ro(
            "from_node",
            [](const NodeLink& l) { return l.from_node; },
            nb::rv_policy::reference)
        .def_prop_ro(
            "to_node",
            [](const NodeLink& l) { return l.to_node; },
            nb::rv_policy::reference)
        .def_prop_ro(
            "from_socket",
            [](const NodeLink& l) { return l.from_sock; },
            nb::rv_policy::reference)
        .def_prop_ro(
            "to_socket",
            [](const NodeLink& l) { return l.to_sock; },
            nb::rv_policy::reference);

    // NodeTreeDescriptor - simplified for basic node registration
    nb::class_<NodeTreeDescriptor>(m, "NodeTreeDescriptor")
        .def(nb::init<>())
        .def(
            "get_node_type",
            &NodeTreeDescriptor::get_node_type,
            nb::rv_policy::reference);

    // NodeTree - main tree management
    nb::class_<NodeTree>(m, "NodeTree")
        .def(nb::init<std::shared_ptr<NodeTreeDescriptor>>())
        .def_prop_ro(
            "nodes",
            [](const NodeTree& tree) {
                std::vector<Node*> result;
                for (const auto& node : tree.nodes) {
                    result.push_back(node.get());
                }
                return result;
            },
            nb::rv_policy::reference_internal)
        .def_prop_ro(
            "links",
            [](const NodeTree& tree) {
                std::vector<NodeLink*> result;
                for (const auto& link : tree.links) {
                    result.push_back(link.get());
                }
                return result;
            },
            nb::rv_policy::reference_internal)
        .def_prop_ro(
            "node_count",
            [](const NodeTree& tree) { return tree.nodes.size(); })
        .def_prop_ro(
            "link_count",
            [](const NodeTree& tree) { return tree.links.size(); })
        // Core operations
        .def("add_node", &NodeTree::add_node, nb::rv_policy::reference)
        .def(
            "find_node",
            static_cast<Node* (NodeTree::*)(NodeId) const>(
                &NodeTree::find_node),
            nb::rv_policy::reference)
        .def(
            "delete_node",
            static_cast<void (NodeTree::*)(Node*, bool)>(
                &NodeTree::delete_node),
            nb::arg("node"),
            nb::arg("allow_repeat_delete") = false)
        .def(
            "delete_node_by_id",
            static_cast<void (NodeTree::*)(NodeId, bool)>(
                &NodeTree::delete_node),
            nb::arg("node_id"),
            nb::arg("allow_repeat_delete") = false)
        // Link operations
        .def(
            "add_link",
            static_cast<NodeLink* (
                NodeTree::*)(NodeSocket*, NodeSocket*, bool, bool)>(
                &NodeTree::add_link),
            nb::arg("from_socket"),
            nb::arg("to_socket"),
            nb::arg("allow_relink_to_output") = false,
            nb::arg("refresh_topology") = true,
            nb::rv_policy::reference)
        .def(
            "add_link_by_name",
            static_cast<NodeLink* (
                NodeTree::*)(Node*, Node*, const char*, const char*, bool)>(
                &NodeTree::add_link),
            nb::arg("from_node"),
            nb::arg("to_node"),
            nb::arg("from_identifier"),
            nb::arg("to_identifier"),
            nb::arg("refresh_topology") = true,
            nb::rv_policy::reference)
        .def(
            "delete_link",
            static_cast<void (NodeTree::*)(NodeLink*, bool, bool)>(
                &NodeTree::delete_link),
            nb::arg("link"),
            nb::arg("refresh_topology") = true,
            nb::arg("remove_from_group") = true)
        .def("can_create_link", &NodeTree::can_create_link)
        .def("clear", &NodeTree::clear)
        // Serialization
        .def("serialize", &NodeTree::serialize)
        .def("deserialize", &NodeTree::deserialize);

    // Helper functions
    m.def("create_descriptor", []() {
        return std::make_shared<NodeTreeDescriptor>();
    });
    m.def("create_tree", [](std::shared_ptr<NodeTreeDescriptor> desc) {
        return std::make_shared<NodeTree>(desc);
    });
}
