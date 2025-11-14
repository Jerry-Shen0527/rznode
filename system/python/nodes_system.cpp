#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/system/node_system_dl.hpp"
#include "nodes/core/node_exec_eager.hpp"

#ifdef GEOM_USD_EXTENSION
#include "GCore/geom_payload.hpp"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/path.h>
#endif


namespace nb = nanobind;
using namespace USTC_CG;

NB_MODULE(nodes_system_py, m)
{
#ifdef GEOM_USD_EXTENSION
    // GeomPayload struct binding - simplified for Python use
    nb::class_<GeomPayload>(m, "GeomPayload")
        .def(nb::init<>())
        // Don't bind USD types - they cause cross-binding issues
        // .def_rw("stage", &GeomPayload::stage, "USD stage pointer")
        // .def_rw("current_time", &GeomPayload::current_time, "Current USD time code")
        // .def_rw("prim_path", &GeomPayload::prim_path, "USD prim path")
        .def_rw("pick", &GeomPayload::pick, "Pick event data")
        .def_rw("delta_time", &GeomPayload::delta_time, "Delta time for simulation")
        .def_rw("has_simulation", &GeomPayload::has_simulation, "Whether simulation is available")
        .def_rw("is_simulating", &GeomPayload::is_simulating, "Whether currently simulating")
        .def_rw("stage_filepath_", &GeomPayload::stage_filepath_, "Stage file path");
    
    // Helper function to create GeomPayload with USD stage from file path
    m.def("create_usd_payload", [](const std::string& filepath, const std::string& prim_path="/geom") {
        GeomPayload payload;
        payload.stage = pxr::UsdStage::CreateNew(filepath);
        payload.prim_path = pxr::SdfPath(prim_path);
        payload.current_time = pxr::UsdTimeCode(0);
        payload.delta_time = 0.0f;
        payload.has_simulation = false;
        payload.is_simulating = false;
        payload.stage_filepath_ = filepath;
        return payload;
    }, nb::arg("filepath"), nb::arg("prim_path")="/geom", 
       "Create GeomPayload with new USD stage");
    
    // Save USD stage to file
    m.def("save_usd_stage", [](GeomPayload& payload) {
        if (payload.stage) {
            payload.stage->GetRootLayer()->Save();
            return true;
        }
        return false;
    }, nb::arg("payload"), "Save USD stage to file");
    
    // Get USD stage pointer as capsule for interop with pxr module
    m.def("get_usd_stage_ptr", [](GeomPayload& payload) -> nb::capsule {
        if (payload.stage) {
            // Return raw pointer wrapped in capsule
            // The capsule name "UsdStageRefPtr" is for identification
            pxr::UsdStageRefPtr* stage_ptr = new pxr::UsdStageRefPtr(payload.stage);
            return nb::capsule(stage_ptr, "UsdStageRefPtr", [](void* p) noexcept {
                delete static_cast<pxr::UsdStageRefPtr*>(p);
            });
        }
        return nb::capsule();
    }, nb::arg("payload"), "Get USD stage pointer as capsule (advanced - for interop with pxr)");
    
    // Create GeomPayload from existing USD stage (via capsule from pxr module)
    m.def("create_payload_from_stage_ptr", [](nb::capsule capsule, const std::string& prim_path="/geom") -> GeomPayload {
        GeomPayload payload;
        const char* name = capsule.name();
        if (name && std::string(name) == "UsdStageRefPtr") {
            auto* stage_ptr = static_cast<pxr::UsdStageRefPtr*>(capsule.data());
            payload.stage = *stage_ptr;
            payload.prim_path = pxr::SdfPath(prim_path);
            payload.current_time = pxr::UsdTimeCode(0);
            payload.delta_time = 0.0f;
            payload.has_simulation = false;
            payload.is_simulating = false;
        }
        return payload;
    }, nb::arg("stage_capsule"), nb::arg("prim_path")="/geom",
       "Create GeomPayload from USD stage capsule (advanced - for interop with pxr)");
#endif

    // NodeTreeExecutor
    nb::class_<NodeTreeExecutor>(m, "NodeTreeExecutor")
        .def("execute", 
            static_cast<void (NodeTreeExecutor::*)(NodeTree*, Node*)>(&NodeTreeExecutor::execute),
            nb::arg("tree"), nb::arg("required_node") = nullptr,
            "Execute the node tree with specific tree and optional required node")
        .def("prepare_tree",
            &NodeTreeExecutor::prepare_tree,
            nb::arg("tree"),
            nb::arg("required_node") = nullptr,
            "Prepare the tree for execution")
        .def("execute_tree",
            &NodeTreeExecutor::execute_tree,
            nb::arg("tree"),
            "Execute the prepared tree")
        .def("sync_node_from_external_storage",
            &NodeTreeExecutor::sync_node_from_external_storage,
            nb::arg("socket"),
            nb::arg("data"),
            "Set socket value from external data")
        .def("sync_node_to_external_storage",
            &NodeTreeExecutor::sync_node_to_external_storage,
            nb::arg("socket"),
            nb::arg("data"),
            "Get socket value to external data")
        .def("sync_batch_from_external", [](NodeTreeExecutor& exec,
                                              const nb::list& data) {
            for (size_t i = 0; i < data.size(); ++i) {
                auto pair = nb::cast<nb::tuple>(data[i]);
                auto* socket = nb::cast<NodeSocket*>(pair[0]);
                auto value = nb::cast<entt::meta_any>(pair[1]);
                exec.sync_node_from_external_storage(socket, value);
            }
        }, nb::arg("data"), "Batch set socket values: [(socket, meta_any), ...]")
        .def("sync_batch_to_external", [](NodeTreeExecutor& exec,
                                            const nb::list& sockets) {
            nb::list results;
            for (size_t i = 0; i < sockets.size(); ++i) {
                auto* socket = nb::cast<NodeSocket*>(sockets[i]);
                entt::meta_any data;
                exec.sync_node_to_external_storage(socket, data);
                results.append(data);
            }
            return results;
        }, nb::arg("sockets"), "Batch get socket values: [socket, ...] -> [meta_any, ...]")
        .def("notify_node_dirty",
            &NodeTreeExecutor::notify_node_dirty,
            nb::arg("node"),
            "Notify executor that a node has been modified")
        .def("notify_socket_dirty",
            &NodeTreeExecutor::notify_socket_dirty,
            nb::arg("socket"),
            "Notify executor that a socket has been modified");

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
        .def("finalize", &NodeSystem::finalize, "Finalize the node system")
#ifdef GEOM_USD_EXTENSION
        .def("set_global_params",
            [](NodeSystem& self, const GeomPayload& payload) {
                self.set_global_params(payload);
            },
            nb::arg("payload"),
            "Set global parameters (GeomPayload) for node execution")
#endif
        ;

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
