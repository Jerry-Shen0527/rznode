#include "nodes/web_server_oatpp/web_server_oatpp.hpp"

#include <memory>
#include <mutex>

#include "oatpp/Environment.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

WebServerOatpp::WebServerOatpp() : is_running_(false), port_(8080)
{
    spdlog::info("WebServerOatpp: Initializing web server");
    oatpp::Environment::init();
}

WebServerOatpp::~WebServerOatpp()
{
    stop();
    oatpp::Environment::destroy();
    spdlog::info("WebServerOatpp: Web server stopped");
}

bool WebServerOatpp::initialize(int port)
{
    port_ = port;

    // Create Router
    router_ = oatpp::web::server::HttpRouter::createShared();

    // Create ApiContantMappers
    api_content_mappers_ =
        std::shared_ptr<oatpp::web::mime::ContentMappers>([] {
            auto json = std::make_shared<oatpp::json::ObjectMapper>();
            json->serializerConfig().json.useBeautifier = true;

            auto mappers = std::make_shared<oatpp::web::mime::ContentMappers>();
            mappers->putMapper(json);

            return mappers;
        }());

    // Add ApiController
    api_controller_ = std::make_shared<ApiController>(api_content_mappers_);
    router_->addController(api_controller_);

    // Create ConnectionHandler
    http_connection_handler_ =
        oatpp::web::server::HttpConnectionHandler::createShared(router_);

    // Create ConnectionProvider
    server_connection_provider_ =
        oatpp::network::tcp::server::ConnectionProvider::createShared(
            { "localhost",
              static_cast<v_uint16>(port_),
              oatpp::network::Address::IP_4 });

    // Create Server
    server_ = std::make_shared<oatpp::network::Server>(
        server_connection_provider_, http_connection_handler_);

    spdlog::info("WebServerOatpp: Web server initialized on port {}", port_);
    return true;
}

void WebServerOatpp::set_node_system(std::shared_ptr<NodeSystem> node_system)
{
    api_controller_->set_node_system(node_system);
    spdlog::info("WebServerOatpp: Node system attached");
}

void WebServerOatpp::start()
{
    if (is_running_) {
        spdlog::warn("WebServerOatpp: Server is already running");
        return;
    }

    if (!api_controller_->is_node_system_attached()) {
        spdlog::error(
            "WebServerOatpp: Cannot start server without node system");
        return;
    }

    spdlog::info(
        "WebServerOatpp: Starting server on http://localhost:{}", port_);
    is_running_ = true;

    // 阻塞调用启动服务器
    server_->run();

    is_running_ = false;
    spdlog::info("WebServerOatpp: Server stopped");
}

void WebServerOatpp::stop()
{
    if (!is_running_) {
        return;
    }

    server_->stop();
    is_running_ = false;
    spdlog::info("WebServerOatpp: Server stopped");
}

bool WebServerOatpp::is_running() const
{
    return is_running_;
}

std::unique_ptr<WebServerOatpp> create_web_server_oatpp()
{
    return std::make_unique<WebServerOatpp>();
}

USTC_CG_NAMESPACE_CLOSE_SCOPE