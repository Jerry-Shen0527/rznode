#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "httplib.h"
#include "nodes/core/node_tree.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/web_server/api.h"

USTC_CG_NAMESPACE_OPEN_SCOPE

// 前端节点类型定义结构
struct WEB_SERVER_API NodeTypeDto {
    std::string id_name;
    std::string ui_name;

    struct SocketDto {
        std::string name;
        std::string identifier;
        std::string type;
        bool optional = false;
        // 默认值、最小值、最大值等（JSON格式）
        std::string default_value;
        std::string min_value;
        std::string max_value;
    };

    struct SocketGroupDto {
        std::string identifier;
        std::string type;  // "input" or "output"
        std::string element_type;
        bool runtime_dynamic = false;
    };

    std::vector<SocketDto> inputs;
    std::vector<SocketDto> outputs;
    std::vector<SocketGroupDto> groups;

    // 节点颜色信息
    float color[4] = { 0.3f, 0.5f, 0.7f, 1.0f };
};

// 前端节点实例定义
struct WEB_SERVER_API NodeInstanceDto {
    std::string id;
    std::string type;
    std::map<std::string, std::string> input_values;  // JSON格式的输入值
};

// 前端连接定义
struct WEB_SERVER_API NodeLinkDto {
    std::string from_node;
    std::string from_socket;
    std::string to_node;
    std::string to_socket;
};

// 前端节点树定义
struct WEB_SERVER_API NodeTreeDto {
    std::vector<NodeInstanceDto> nodes;
    std::vector<NodeLinkDto> links;
};

// 执行结果定义
struct WEB_SERVER_API ExecutionResultDto {
    bool success = false;         // 执行是否成功
    std::string error = "";       // 错误信息（如果有的话）
    double execution_time = 0.0;  // 执行时间（秒）
    // TODO: 添加执行完成后，各节点输出值、执行状态、颜色等
};

// Web服务器类
class WEB_SERVER_API WebServer {
   public:
    WebServer();
    virtual ~WebServer();

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

   protected:
    // API 处理函数
    void handle_get_status(const httplib::Request& req, httplib::Response& res);
    void handle_get_value_types(
        const httplib::Request& req,
        httplib::Response& res);
    void handle_get_node_types(
        const httplib::Request& req,
        httplib::Response& res);
    void handle_execute_tree(
        const httplib::Request& req,
        httplib::Response& res);
    void handle_validate_tree(
        const httplib::Request& req,
        httplib::Response& res);

    // 辅助函数
    NodeTypeDto convert_node_type_to_dto(const NodeTypeInfo& type_info) const;
    std::unique_ptr<NodeTree> convert_dto_to_node_tree(
        const NodeTreeDto& dto) const;
    ExecutionResultDto execute_node_tree_internal(const NodeTreeDto& dto) const;

    // JSON 序列化/反序列化
    nlohmann::json serialize_value_types(
        const std::vector<std::string>& types) const;
    nlohmann::json serialize_node_types(
        const std::vector<NodeTypeDto>& types) const;
    NodeTreeDto deserialize_node_tree(const std::string& json) const;

    // CORS 设置
    void setup_cors_headers(httplib::Response& res) const;

   private:
    std::unique_ptr<httplib::Server> server_;
    std::shared_ptr<NodeSystem> node_system_;
    int port_;
    bool is_running_;

    // 缓存的接口类型信息
    mutable std::vector<std::string> cached_value_types_;
    mutable bool value_types_cache_dirty_;

    void refresh_value_types_cache() const;

    // 缓存的节点类型信息
    mutable std::vector<NodeTypeDto> cached_node_types_;
    mutable bool node_types_cache_dirty_;

    void refresh_node_types_cache() const;

    // 设置路由
    void setup_routes();
};

// 工厂函数
WEB_SERVER_API std::unique_ptr<WebServer> create_web_server();

USTC_CG_NAMESPACE_CLOSE_SCOPE
