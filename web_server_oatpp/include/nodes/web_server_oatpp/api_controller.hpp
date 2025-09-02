#pragma once

#ifdef GEOM_EXTENSION
#include "nodes//web_server_oatpp/geom_ws_listener.hpp"
#endif

#include <memory>

#include "nodes/web_server_oatpp/api.h"
#include "nodes/web_server_oatpp/dto.hpp"
#include "nodes/web_server_oatpp/util.hpp"
#include "oatpp-websocket/ConnectionHandler.hpp"
#include "oatpp-websocket/Handshaker.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/web/mime/ContentMappers.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "spdlog/spdlog.h"

#include OATPP_CODEGEN_BEGIN(ApiController)  //<- Begin Codegen

USTC_CG_NAMESPACE_OPEN_SCOPE

/**
 * API Controller for RESTful endpoints
 * 参考文档：
 * https://oatpp.io/docs/components/api-controller/
 * https://github.com/oatpp/oatpp-starter/blob/master/src/controller/MyController.hpp
 * https://github.com/oatpp/example-crud/blob/master/src/controller/UserController.hpp
 */

class WEB_SERVER_OATPP_API ApiController
    : public oatpp::web::server::api::ApiController {
   public:
    ApiController(
        std::shared_ptr<oatpp::web::mime::ContentMappers> api_content_mappers)
        : oatpp::web::server::api::ApiController(api_content_mappers)
    {
        static_files_manager_ = std::make_unique<StaticFilesManager>();
        node_system_component_ = std::make_unique<NodeSystemComponent>();

#ifdef GEOM_EXTENSION
        geometry_ws_connection_handler =
            oatpp::websocket::ConnectionHandler::createShared();
        geometry_ws_connection_handler->setSocketInstanceListener(
            std::make_shared<GeometryWSInstanceListener>());
#endif
    }

    void set_node_system(std::shared_ptr<NodeSystem> node_system)
    {
        node_system_component_->set_node_system(node_system);
    }

    bool is_node_system_attached() const
    {
        return node_system_component_->node_system_attached();
    }

   private:
    // 静态文件管理器，用于提供前端文件
    std::unique_ptr<StaticFilesManager> static_files_manager_ = nullptr;
    // NodeSystem 组件
    std::unique_ptr<NodeSystemComponent> node_system_component_ = nullptr;

#ifdef GEOM_EXTENSION
    // 用于处理 Geometry Visualizer 相关的 WebSocket 连接
    std::shared_ptr<oatpp::websocket::ConnectionHandler>
        geometry_ws_connection_handler = nullptr;
#endif

   public:
#ifdef GEOM_EXTENSION
    // 测试websocket接口
    ENDPOINT(
        "GET",
        "/geometry/ws",
        GeometryWS,
        REQUEST(std::shared_ptr<IncomingRequest>, request))
    {
        return oatpp::websocket::Handshaker::serversideHandshake(
            request->getHeaders(), geometry_ws_connection_handler);
    };
#endif

    ENDPOINT_INFO(GetStatus)
    {
        info->summary = "Get server status";
        info->description = "Returns the current status of the server.";
        info->addResponse<Object<MessageDto>>(
            Status::CODE_200, "application/json");
    }
    ENDPOINT("GET", "/api/status", GetStatus)
    {
        auto status_dto = StatusDto::createShared();
        status_dto->status = "running";
        if (node_system_component_->node_system_attached()) {
            status_dto->has_node_system = true;
            status_dto->message = "Node system is attached.";
        }
        else {
            status_dto->has_node_system = false;
            status_dto->message = "Node system is not attached.";
        }

        auto message_dto = MessageDto::createShared();
        message_dto->code = 0;
        message_dto->message = "success";
        message_dto->data = status_dto;
        spdlog::debug("WebServer: Status request handled");
        return createDtoResponse(Status::CODE_200, message_dto);
    }

    ENDPOINT_INFO(GetValueTypes)
    {
        info->summary = "Get registered value types";
        info->description =
            "Returns a list of all registered value types in the node system.";
        info->addResponse<Object<MessageDto>>(
            Status::CODE_200, "application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_500, "application/json");
    }
    ENDPOINT("GET", "/api/value-types", GetValueTypes)
    {
        // 检查节点系统是否初始化
        if (!node_system_component_->node_system_attached()) {
            auto message_dto = MessageDto::createShared();
            message_dto->code = 1;
            message_dto->message = "Node system not attached";
            message_dto->data = nullptr;
            spdlog::warn(
                "WebServer: Value types request failed - no node system");
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        oatpp::Object<ValueTypesDto> value_types_dto;
        try {
            // 尝试获取值类型
            value_types_dto = node_system_component_->get_value_types();
        }
        catch (const std::exception& e) {
            // 若获取值类型失败，返回500错误
            auto message_dto = MessageDto::createShared();
            message_dto->code = 2;
            message_dto->message =
                std::string("Node system not available: ") + e.what();
            message_dto->data = nullptr;
            spdlog::error(
                "WebServer: Value types request failed - exception: {}",
                e.what());
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        auto message_dto = MessageDto::createShared();
        message_dto->code = 0;
        message_dto->message = "success";
        message_dto->data = value_types_dto;
        spdlog::debug(
            "WebServer: Value types request handled, {} types",
            value_types_dto->value_types->size());
        return createDtoResponse(Status::CODE_200, message_dto);
    }

    ENDPOINT_INFO(GetNodeTypes)
    {
        info->summary = "Get registered node types";
        info->description =
            "Returns a list of all registered node types in the node system.";
        info->addResponse<Object<MessageDto>>(
            Status::CODE_200, "application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_500, "application/json");
    }
    ENDPOINT("GET", "/api/node-types", GetNodeTypes)
    {
        // 检查节点系统是否初始化
        if (!node_system_component_->node_system_attached()) {
            auto message_dto = MessageDto::createShared();
            message_dto->code = 1;
            message_dto->message = "Node system not attached";
            message_dto->data = nullptr;
            spdlog::warn(
                "WebServer: Node types request failed - no node system");
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        oatpp::Object<NodeTypesDto> node_types_dto;
        try {
            // 尝试获取节点类型
            node_types_dto = node_system_component_->get_node_types();
        }
        catch (const std::exception& e) {
            // 若获取节点类型失败，返回500错误
            auto message_dto = MessageDto::createShared();
            message_dto->code = 2;
            message_dto->message =
                std::string("Node system not available: ") + e.what();
            message_dto->data = nullptr;
            spdlog::error(
                "WebServer: Node types request failed - exception: {}",
                e.what());
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        auto message_dto = MessageDto::createShared();
        message_dto->code = 0;
        message_dto->message = "success";
        message_dto->data = node_types_dto;
        spdlog::debug(
            "WebServer: Node types request handled, {} types",
            node_types_dto->node_types->size());
        return createDtoResponse(Status::CODE_200, message_dto);
    }

    ENDPOINT_INFO(ExecuteTree)
    {
        info->summary = "Execute a node tree";
        info->description =
            "Executes the provided node tree and returns the execution result.";
        info->addConsumes<Object<NodeTreeDto>>("application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_200, "application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_500, "application/json");
    }
    ENDPOINT(
        "POST",
        "/api/execute",
        ExecuteTree,
        BODY_DTO(Object<NodeTreeDto>, node_tree_dto))
    {
        // 检查节点系统是否初始化
        if (!node_system_component_->node_system_attached()) {
            auto message_dto = MessageDto::createShared();
            message_dto->code = 1;
            message_dto->message = "Node system not attached";
            message_dto->data = nullptr;
            spdlog::warn(
                "WebServer: Execute tree request failed - no node system");
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        oatpp::Object<ExecutionResultDto> result_dto;
        try {
            // 尝试执行节点树
            result_dto =
                node_system_component_->execute_node_tree(node_tree_dto);
        }
        catch (const std::exception& e) {
            // 若执行节点树失败，在data字段返回错误信息
            result_dto = ExecutionResultDto::createShared();
            result_dto->success = false;
            result_dto->error = e.what();
            result_dto->execution_time = 0.0;
        }

        auto message_dto = MessageDto::createShared();
        message_dto->code = 0;
        message_dto->message =
            result_dto->success ? "success" : "execution failed";
        message_dto->data = result_dto;
        spdlog::debug(
            "WebServer: Execute tree request handled, success: {}, time: {} ms",
            *result_dto->success,
            *result_dto->execution_time);
        return createDtoResponse(Status::CODE_200, message_dto);
    }

    ENDPOINT_INFO(ValidateTree)
    {
        info->summary = "Validate a node tree";
        info->description =
            "Validates the provided node tree without executing it.";
        info->addConsumes<Object<NodeTreeDto>>("application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_200, "application/json");
        info->addResponse<Object<MessageDto>>(
            Status::CODE_500, "application/json");
    }
    ENDPOINT(
        "POST",
        "/api/validate",
        ValidateTree,
        BODY_DTO(Object<NodeTreeDto>, node_tree_dto))
    {
        // 检查节点系统是否初始化
        if (!node_system_component_->node_system_attached()) {
            auto message_dto = MessageDto::createShared();
            message_dto->code = 1;
            message_dto->message = "Node system not attached";
            message_dto->data = nullptr;
            spdlog::warn(
                "WebServer: Validate tree request failed - no node system");
            return createDtoResponse(Status::CODE_500, message_dto);
        }

        oatpp::Object<ValidationResultDto> validation_result_dto;
        try {
            // 尝试验证节点树
            node_system_component_->update_node_tree_from_dto(
                node_system_component_->get_node_system()->get_node_tree(),
                node_tree_dto);
            // 没有异常，验证成功
            validation_result_dto = ValidationResultDto::createShared();
            validation_result_dto->valid = true;
            validation_result_dto->error = "";
        }
        catch (const std::exception& e) {
            // 若验证节点树失败，在data字段返回错误信息
            validation_result_dto = ValidationResultDto::createShared();
            validation_result_dto->valid = false;
            validation_result_dto->error = e.what();
            spdlog::warn(
                "WebServer: Validate tree request failed - exception: {}",
                e.what());
        }

        auto message_dto = MessageDto::createShared();
        message_dto->code = 0;
        message_dto->message =
            validation_result_dto->valid ? "success" : "validation failed";
        message_dto->data = validation_result_dto;
        spdlog::debug(
            "WebServer: Validate tree request handled, valid: {}",
            *validation_result_dto->valid);
        return createDtoResponse(Status::CODE_200, message_dto);
    }

    ENDPOINT("GET", "/", Root)
    {
        // Serve index.html from the web/dist directory
        auto file = static_files_manager_->getFile("index.html");
        auto res = createResponse(Status::CODE_200, file);
        res->putHeader("Content-Type", "text/html");
        return res;
    }

    ENDPOINT(
        "GET",
        "*",
        GetStaticFiles,
        REQUEST(std::shared_ptr<IncomingRequest>, request))
    {
        // Get the target file
        auto tail = request->getPathTail();
        auto file = static_files_manager_->getFile(request->getPathTail());
        auto mime_type =
            static_files_manager_->getFileMIMEType(request->getPathTail());
        OATPP_ASSERT_HTTP(
            file.get() != nullptr, Status::CODE_404, "File not found");
        auto res = createResponse(Status::CODE_200, file);
        res->putHeader("Content-Type", mime_type);
        return res;
    }
};

USTC_CG_NAMESPACE_CLOSE_SCOPE

#include OATPP_CODEGEN_END(ApiController)  //<- End Codegen