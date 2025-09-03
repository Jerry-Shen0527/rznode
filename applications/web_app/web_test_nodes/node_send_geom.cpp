#include <memory>

#include "GCore/GOP.h"
#include "nodes/core/def/node_def.hpp"
#include "nodes/web_server_oatpp/geom_dto.hpp"
#include "nodes/web_server_oatpp/geom_utils.hpp"
#include "nodes/web_server_oatpp/web_server_oatpp.hpp"
#include "oatpp-websocket/WebSocket.hpp"
#include "oatpp/Types.hpp"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(send_geom)
{
    return "Send Geometry";
}

NODE_DECLARATION_FUNCTION(send_geom)
{
    b.add_input<Geometry>("geometry");
}

NODE_EXECUTION_FUNCTION(send_geom)
{
    auto web_server_params = params.get_global_payload<WebServerPrams>();
    if (!web_server_params.web_server) {
        spdlog::error("send_geom node: Web server instance is null");
        return false;
    }

    auto geom = params.get_input<Geometry>("geometry");

    // 将几何数据转换为DTO
    auto mesh = geom.get_component<MeshComponent>();
    auto curve = geom.get_component<CurveComponent>();
    auto points = geom.get_component<PointsComponent>();

    if (!mesh && !curve && !points) {
        spdlog::warn("send_geom node: Geometry has no supported component");
        return false;
    }

    // auto mesh_dto = GeometryUtils::convertMeshToDto(mesh);
    // web_server_params.web_server->send_message_via_ws(mesh_dto);

    // auto geom_dto = GeometryDataDto::createShared();
    // geom_dto->id = "geom_1";
    // geom_dto->type = "mesh";
    // geom_dto->transform = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    //                         0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // web_server_params.web_server->send_message_via_ws(geom_dto);

    // auto mesh_data_dto = MeshDataDto::createShared();
    // mesh_data_dto->id = "mesh_1";
    // mesh_data_dto->type = "mesh";
    // mesh_data_dto->mesh_data = mesh_dto;
    // web_server_params.web_server->send_message_via_ws(mesh_data_dto);

    // auto geom_dto_2 = GeometryUtils::convertGeometryToDto<MeshDataDto>(
    //     std::make_shared<Geometry>(geom), "mesh_2");
    // web_server_params.web_server->send_message_via_ws(geom_dto_2);

    if (mesh) {
        auto geom_dto = GeometryUtils::convertGeometryToDto<MeshDataDto>(
            std::make_shared<Geometry>(geom), "mesh_1");
        if (geom_dto) {
            web_server_params.web_server->send_message_via_ws(geom_dto);
        }
    }
    if (curve) {
        auto geom_dto = GeometryUtils::convertGeometryToDto<CurveDataDto>(
            std::make_shared<Geometry>(geom), "curve_1");
        if (geom_dto) {
            web_server_params.web_server->send_message_via_ws(geom_dto);
        }
    }
    if (points) {
        auto geom_dto = GeometryUtils::convertGeometryToDto<PointsDataDto>(
            std::make_shared<Geometry>(geom), "points_1");
        if (geom_dto) {
            web_server_params.web_server->send_message_via_ws(geom_dto);
        }
    }

    return true;
}

NODE_DECLARATION_REQUIRED(send_geom)

NODE_DEF_CLOSE_SCOPE