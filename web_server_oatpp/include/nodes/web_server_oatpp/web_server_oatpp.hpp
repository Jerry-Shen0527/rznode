#pragma once

#include <memory>

#include "nodes/system/node_system.hpp"
#include "nodes/web_server_oatpp/api.h"
#include "nodes/web_server_oatpp/api_controller.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/web/mime/ContentMappers.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

// Web服务器类
class WEB_SERVER_OATPP_API WebServerOatpp {
   public:
    WebServerOatpp();
    virtual ~WebServerOatpp();

    // 初始化服务器
    bool initialize(int port = 8080);

    // 设置节点系统
    void set_node_system(std::shared_ptr<NodeSystem> node_system);

    // 启动服务器（阻塞调用）
    void start();  // TODO: 迁移到oatpp

    // 停止服务器
    void stop();  // TODO: 迁移到oatpp

    // 检查服务器是否运行中
    bool is_running() const;  // TODO: 迁移到oatpp

    // 获取服务器端口
    int get_port() const;

   private:
    std::shared_ptr<oatpp::web::server::HttpRouter> router_;
    std::shared_ptr<oatpp::web::mime::ContentMappers> api_content_mappers_;
    std::shared_ptr<oatpp::web::server::HttpConnectionHandler>
        http_connection_handler_;
    std::shared_ptr<oatpp::network::ServerConnectionProvider>
        server_connection_provider_;
    std::shared_ptr<oatpp::network::Server> server_;
    std::shared_ptr<ApiController> api_controller_;
    int port_;
    bool is_running_;
};

// 工厂函数
WEB_SERVER_OATPP_API std::unique_ptr<WebServerOatpp> create_web_server_oatpp();

USTC_CG_NAMESPACE_CLOSE_SCOPE
