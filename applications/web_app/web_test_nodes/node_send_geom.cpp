#include <memory>
#include <string>

#include "GCore/GOP.h"
#include "nodes/core/def/node_def.hpp"
#include "nodes/web_server/geom_utils.hpp"
#include "nodes/web_server/web_server.hpp"
#include "oatpp/Types.hpp"
#include "spdlog/spdlog.h"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_UI(send_geom)
{
    return "Send Geometry";
}

NODE_DECLARATION_FUNCTION(send_geom)
{
    // auto dummy_json = std::make_shared<oatpp::json::ObjectMapper>();
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

    // 将几何信息转换为几何消息DTO
    auto geometry_message_dto = GeometryUtils::convertGeometryMessageToDto(
        "geometry_update", "default", { geom }, { geom_id });

    // 发送消息
    if (!web_server_params.web_server->send_message_via_ws(
            geometry_message_dto)) {
        spdlog::error(
            "send_geom node: Failed to send geometry message via WebSocket");
        return false;
    }

    return true;
}

NODE_DECLARATION_REQUIRED(send_geom)

NODE_DEF_CLOSE_SCOPE