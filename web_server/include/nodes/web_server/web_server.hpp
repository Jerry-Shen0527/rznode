#pragma once

#include <memory>
#include <string>

#include "nodes/system/node_system.hpp"
#include "nodes/web_server/api.h"
#include "nodes/web_server/api_controller.hpp"
#include "oatpp/Types.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/web/mime/ContentMappers.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

// Web服务器类
class WEB_SERVER_API WebServerOatpp {
   public:
    WebServerOatpp();
    virtual ~WebServerOatpp();

    // 初始化服务器
    bool initialize(int port = 8080);

    // 设置节点系统
    void set_node_system(std::shared_ptr<NodeSystem> node_system);

    // 启动服务器（阻塞调用）
    void start();

    // 停止服务器
    void stop();

    // 检查服务器是否运行中
    bool is_running() const;

    // 获取服务器端口
    int get_port() const;

    // 通过webserver发送数据
    bool send_message_via_ws(const oatpp::Void& dto) const;
    bool send_message_via_ws(const std::string& message) const;

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

// 全局参数
struct WebServerPrams {
    std::shared_ptr<WebServerOatpp> web_server;
};

// 工厂函数
WEB_SERVER_API
std::unique_ptr<WebServerOatpp> create_web_server_oatpp();

USTC_CG_NAMESPACE_CLOSE_SCOPE
