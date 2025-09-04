#include <memory>
#include <string>

#include "GCore/GOP.h"
#include "nodes/core/def/node_def.hpp"
#include "nodes/web_server_oatpp/geom_dto.hpp"
#include "nodes/web_server_oatpp/geom_utils.hpp"
#include "nodes/web_server_oatpp/web_server_oatpp.hpp"
#include "spdlog/spdlog.h"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(send_geom)
{
    return "Send Geometry";
}

NODE_DECLARATION_FUNCTION(send_geom)
{
    b.add_input<Geometry>("geometry");
    b.add_input<std::string>("geom_id");
}

NODE_EXECUTION_FUNCTION(send_geom)
{
    auto web_server_params = params.get_global_payload<WebServerPrams>();
    if (!web_server_params.web_server) {
        spdlog::error("send_geom node: Web server instance is null");
        return false;
    }

    auto geom = params.get_input<Geometry>("geometry");
    auto geom_id = params.get_input<std::string>("geom_id");

    if (geom_id.length() == 0) {
        geom_id = "default_geom_id";
    }

    // 将几何数据转换为DTO
    auto mesh = geom.get_component<MeshComponent>();
    auto curve = geom.get_component<CurveComponent>();
    auto points = geom.get_component<PointsComponent>();

    if (!mesh && !curve && !points) {
        spdlog::warn("send_geom node: Geometry has no supported component");
        return false;
    }

    // 新建 GeometryMessage
    auto geom_message_dto = GeometryMessageDto::createShared();
    geom_message_dto->type = "geometry_update";
    geom_message_dto->scene_id = "default";
    geom_message_dto->timestamp = static_cast<v_int64>(std::time(nullptr));

    if (mesh) {
        for (int count : mesh->get_face_vertex_counts()) {
            if (count != 3) {
                spdlog::error(
                    "send_geom node: Mesh contains non-triangular faces, which "
                    "is not supported yet");
                return false;
            }
        }

        auto geom_dto = GeometryUtils::convertGeometryToDto<MeshDataDto>(
            std::make_shared<Geometry>(geom), geom_id);
        if (geom_dto) {
            geom_message_dto->geometries->push_back(geom_dto);
        }
    }
    if (curve) {
        auto geom_dto = GeometryUtils::convertGeometryToDto<CurveDataDto>(
            std::make_shared<Geometry>(geom), geom_id);
        if (geom_dto) {
            geom_message_dto->geometries->push_back(geom_dto);
        }
    }
    if (points) {
        auto geom_dto = GeometryUtils::convertGeometryToDto<PointsDataDto>(
            std::make_shared<Geometry>(geom), geom_id);
        if (geom_dto) {
            geom_message_dto->geometries->push_back(geom_dto);
        }
    }

    // 发送消息
    if (geom_message_dto->geometries->size() > 0) {
        web_server_params.web_server->send_message_via_ws(geom_message_dto);
    }
    else {
        spdlog::warn("send_geom node: No valid geometry DTO to send");
        return false;
    }

    return true;
}

NODE_DECLARATION_REQUIRED(send_geom)

NODE_DEF_CLOSE_SCOPE