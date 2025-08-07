#include "nodes/web_server/web_server.hpp"

#include <spdlog/spdlog.h>

#include <chrono>

#include "nodes/core/api.hpp"
#include "nodes/core/io/json.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

WebServer::WebServer()
    : server_(std::make_unique<httplib::Server>()),
      port_(8080),
      is_running_(false),
      node_types_cache_dirty_(true)
{
    spdlog::info("WebServer: Initializing web server");
    setup_routes();
}

WebServer::~WebServer()
{
    if (is_running_) {
        stop();
    }
    spdlog::info("WebServer: Web server destroyed");
}

bool WebServer::initialize(int port)
{
    port_ = port;

    // 设置服务器配置
    server_->set_mount_point("/", "../web_server/web/dist");

    spdlog::info("WebServer: Initialized on port {}", port_);
    return true;
}

void WebServer::set_node_system(std::shared_ptr<NodeSystem> node_system)
{
    node_system_ = node_system;
    node_types_cache_dirty_ = true;
    spdlog::info("WebServer: Node system attached");
}

void WebServer::start()
{
    if (is_running_) {
        spdlog::warn("WebServer: Server is already running");
        return;
    }

    if (!node_system_) {
        spdlog::error("WebServer: Cannot start server without node system");
        return;
    }

    spdlog::info("WebServer: Starting server on http://localhost:{}", port_);
    is_running_ = true;

    // 阻塞调用启动服务器
    if (!server_->listen("0.0.0.0", port_)) {
        spdlog::error("WebServer: Failed to start server on port {}", port_);
        is_running_ = false;
    }
}

void WebServer::stop()
{
    if (!is_running_) {
        return;
    }

    server_->stop();
    is_running_ = false;
    spdlog::info("WebServer: Server stopped");
}

bool WebServer::is_running() const
{
    return is_running_;
}

int WebServer::get_port() const
{
    return port_;
}

void WebServer::setup_routes()
{
    // 设置CORS预检请求处理
    server_->Options(
        ".*", [this](const httplib::Request& req, httplib::Response& res) {
            setup_cors_headers(res);
        });

    // API路由
    server_->Get(
        "/api/status",
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_status(req, res);
        });

    server_->Get(
        "/api/node-types",
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_node_types(req, res);
        });

    server_->Post(
        "/api/execute",
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_execute_tree(req, res);
        });

    server_->Post(
        "/api/validate",
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_validate_tree(req, res);
        });

    // 默认路由
    server_->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(
            R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>RzNode Web Server</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 50px; }
        .container { max-width: 600px; margin: 0 auto; }
        .status { color: green; }
    </style>
</head>
<body>
    <div class="container">
        <h1>RzNode 节点编程系统</h1>
        <p class="status">✓ Web服务器运行正常</p>
        <p>前端节点编辑器界面</p>
        <hr>
        <p><a href="/api/status">API状态检查</a></p>
        <p><a href="/api/node-types">节点类型列表</a></p>
    </div>
</body>
</html>
        )",
            "text/html; charset=utf-8");
    });
}

void WebServer::handle_get_status(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    nlohmann::json status_json;
    status_json["status"] = "running";
    status_json["message"] = "RzNode Web服务器运行正常";
    status_json["port"] = port_;
    status_json["has_node_system"] = (node_system_ != nullptr);

    if (node_system_) {
        status_json["node_system_info"] = "Node system attached";
    }

    res.set_content(status_json.dump(2), "application/json");
    spdlog::debug("WebServer: Status request handled");
}

void WebServer::handle_get_node_types(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        nlohmann::json error;
        error["error"] = "Node system not available";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
        return;
    }

    try {
        refresh_node_types_cache();
        std::string json_response = serialize_node_types(cached_node_types_);
        res.set_content(json_response, "application/json");
        spdlog::debug(
            "WebServer: Node types request handled, {} types",
            cached_node_types_.size());
    }
    catch (const std::exception& e) {
        nlohmann::json error;
        error["error"] = std::string("Failed to get node types: ") + e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
        spdlog::error("WebServer: Error getting node types: {}", e.what());
    }
}

void WebServer::handle_execute_tree(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        nlohmann::json error;
        error["error"] = "Node system not available";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
        return;
    }

    try {
        NodeTreeDto tree_dto = deserialize_node_tree(req.body);
        ExecutionResultDto result = execute_node_tree_internal(tree_dto);

        std::string json_response = serialize_execution_result(result);
        res.set_content(json_response, "application/json");

        spdlog::info(
            "WebServer: Tree execution completed, success: {}", result.success);
    }
    catch (const std::exception& e) {
        nlohmann::json error;
        error["error"] = std::string("Execution failed: ") + e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
        spdlog::error("WebServer: Execution error: {}", e.what());
    }
}

void WebServer::handle_validate_tree(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        nlohmann::json error;
        error["error"] = "Node system not available";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
        return;
    }

    try {
        NodeTreeDto tree_dto = deserialize_node_tree(req.body);

        // 验证逻辑：尝试构建节点树但不执行
        auto tree = convert_dto_to_node_tree(tree_dto);

        nlohmann::json result;
        result["valid"] = (tree != nullptr);
        result["message"] =
            tree ? "Node tree is valid" : "Node tree validation failed";

        res.set_content(result.dump(2), "application/json");
        spdlog::debug("WebServer: Tree validation completed");
    }
    catch (const std::exception& e) {
        nlohmann::json error;
        error["valid"] = false;
        error["message"] = std::string("Validation failed: ") + e.what();
        res.set_content(error.dump(), "application/json");
        spdlog::warn("WebServer: Validation error: {}", e.what());
    }
}

NodeTypeDto WebServer::convert_node_type_to_dto(
    const NodeTypeInfo& type_info) const
{
    NodeTypeDto dto;
    dto.id_name = type_info.id_name;
    dto.ui_name = type_info.ui_name;

    // 复制颜色信息
    for (int i = 0; i < 4; ++i) {
        dto.color[i] = type_info.color[i];
    }

    // 转换输入输出socket信息
    const auto& declaration = type_info.static_declaration;

    for (const auto& input : declaration.inputs) {
        NodeTypeDto::SocketDto socket_dto;
        socket_dto.name = input->name;
        socket_dto.identifier = input->identifier;
        socket_dto.type = get_type_name(input->type);
        // TODO: 添加默认值、最小值、最大值的提取逻辑
        dto.inputs.push_back(socket_dto);
    }

    for (const auto& output : declaration.outputs) {
        NodeTypeDto::SocketDto socket_dto;
        socket_dto.name = output->name;
        socket_dto.identifier = output->identifier;
        socket_dto.type = get_type_name(output->type);
        dto.outputs.push_back(socket_dto);
    }

    // 转换socket groups
    for (const auto& group : declaration.socket_group_decls) {
        NodeTypeDto::SocketGroupDto group_dto;
        group_dto.identifier = group->identifier;
        group_dto.type = (group->in_out == PinKind::Input) ? "input" : "output";
        group_dto.element_type = get_type_name(group->type);
        group_dto.runtime_dynamic = group->runtime_dynamic;
        dto.groups.push_back(group_dto);
    }

    return dto;
}

std::unique_ptr<NodeTree> WebServer::convert_dto_to_node_tree(
    const NodeTreeDto& dto) const
{
    if (!node_system_) {
        throw std::runtime_error("Node system not available");
    }

    auto descriptor = node_system_->node_tree_descriptor();
    auto tree = create_node_tree(descriptor);

    // 创建节点
    std::map<int, Node*> node_map;
    for (const auto& node_dto : dto.nodes) {
        Node* node = tree->add_node(node_dto.type.c_str());
        if (!node) {
            throw std::runtime_error(
                "Failed to create node of type: " + node_dto.type);
        }
        node_map[node_dto.id] = node;

        // TODO: 设置输入值
        // 这里需要根据input_values设置节点的输入socket值
    }

    // 创建连接
    for (const auto& link_dto : dto.links) {
        auto from_node_it = node_map.find(link_dto.from_node);
        auto to_node_it = node_map.find(link_dto.to_node);

        if (from_node_it == node_map.end() || to_node_it == node_map.end()) {
            throw std::runtime_error("Invalid node ID in link");
        }

        Node* from_node = from_node_it->second;
        Node* to_node = to_node_it->second;

        NodeSocket* from_socket =
            from_node->get_output_socket(link_dto.from_socket.c_str());
        NodeSocket* to_socket =
            to_node->get_input_socket(link_dto.to_socket.c_str());

        if (!from_socket || !to_socket) {
            throw std::runtime_error("Invalid socket identifier in link");
        }

        tree->add_link(from_socket, to_socket);
    }

    return tree;
}

ExecutionResultDto WebServer::execute_node_tree_internal(
    const NodeTreeDto& dto) const
{
    ExecutionResultDto result;
    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        // 将 NodeTreeDto 转换为 JSON 字符串
        nlohmann::json tree_json;
        tree_json["nodes"] = nlohmann::json::array();
        tree_json["links"] = nlohmann::json::array();
        
        // 转换节点
        for (const auto& node_dto : dto.nodes) {
            nlohmann::json node_json;
            node_json["id"] = node_dto.id;
            node_json["type"] = node_dto.type;
            node_json["position_x"] = node_dto.position_x;
            node_json["position_y"] = node_dto.position_y;
            
            // 转换输入值
            if (!node_dto.input_values.empty()) {
                nlohmann::json input_values_json;
                for (const auto& [key, value] : node_dto.input_values) {
                    try {
                        input_values_json[key] = nlohmann::json::parse(value);
                    } catch (const std::exception&) {
                        input_values_json[key] = value;  // 如果解析失败，直接使用字符串
                    }
                }
                node_json["input_values"] = input_values_json;
            }
            
            tree_json["nodes"].push_back(node_json);
        }
        
        // 转换连接
        for (const auto& link_dto : dto.links) {
            nlohmann::json link_json;
            link_json["from_node"] = link_dto.from_node;
            link_json["from_socket"] = link_dto.from_socket;
            link_json["to_node"] = link_dto.to_node;
            link_json["to_socket"] = link_dto.to_socket;
            tree_json["links"].push_back(link_json);
        }
        
        // 使用新的 JSON 方法设置节点树
        std::string json_str = tree_json.dump();
        bool set_success = node_system_->set_node_tree_from_json(json_str);
        
        if (!set_success) {
            throw std::runtime_error("Failed to set node tree from JSON");
        }
        
        // 执行节点树
        node_system_->execute(false);  // 非UI执行

        result.success = true;
        result.error_message = "";

        // TODO: 提取输出值
        // 这里需要从执行后的节点树中提取输出值
        // 可以遍历节点树中的所有输出socket并提取它们的值
        NodeTree* tree = node_system_->get_node_tree();
        if (tree) {
            // 收集所有输出值
            for (auto& node : tree->nodes) {
                for (auto& output : node->get_outputs()) {
                    if (output->type_info) {
                        // 构建输出键名：节点ID_socket标识符
                        std::string output_key = std::to_string(node->ID.Get()) + "_" + output->identifier;
                        
                        // TODO: 这里需要根据socket的类型来序列化值
                        // 目前简单地存储一个占位符
                        result.output_values[output_key] = "output_value_placeholder";
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
        spdlog::error("WebServer: Execution error: {}", e.what());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    result.execution_time = duration.count() / 1000.0;

    return result;
}

void WebServer::refresh_node_types_cache() const
{
    if (!node_types_cache_dirty_) {
        return;
    }

    cached_node_types_.clear();

    if (!node_system_) {
        return;
    }

    auto descriptor = node_system_->node_tree_descriptor();
    auto registered_types = descriptor->get_registered_node_types();

    for (const auto& [type_name, type_info] : registered_types) {
        NodeTypeDto dto = convert_node_type_to_dto(type_info);
        cached_node_types_.push_back(dto);
    }

    node_types_cache_dirty_ = false;
    spdlog::debug(
        "WebServer: Node types cache refreshed, {} types",
        cached_node_types_.size());
}

std::string WebServer::serialize_node_types(
    const std::vector<NodeTypeDto>& types) const
{
    nlohmann::json json_array = nlohmann::json::array();

    for (const auto& type : types) {
        nlohmann::json type_json;
        type_json["id_name"] = type.id_name;
        type_json["ui_name"] = type.ui_name;
        type_json["color"] = nlohmann::json::array(
            { type.color[0], type.color[1], type.color[2], type.color[3] });

        type_json["inputs"] = nlohmann::json::array();
        for (const auto& input : type.inputs) {
            nlohmann::json input_json;
            input_json["name"] = input.name;
            input_json["identifier"] = input.identifier;
            input_json["type"] = input.type;
            input_json["optional"] = input.optional;
            if (!input.default_value.empty())
                input_json["default_value"] = input.default_value;
            if (!input.min_value.empty())
                input_json["min_value"] = input.min_value;
            if (!input.max_value.empty())
                input_json["max_value"] = input.max_value;
            type_json["inputs"].push_back(input_json);
        }

        type_json["outputs"] = nlohmann::json::array();
        for (const auto& output : type.outputs) {
            nlohmann::json output_json;
            output_json["name"] = output.name;
            output_json["identifier"] = output.identifier;
            output_json["type"] = output.type;
            output_json["optional"] = output.optional;
            type_json["outputs"].push_back(output_json);
        }

        type_json["groups"] = nlohmann::json::array();
        for (const auto& group : type.groups) {
            nlohmann::json group_json;
            group_json["identifier"] = group.identifier;
            group_json["type"] = group.type;
            group_json["element_type"] = group.element_type;
            group_json["runtime_dynamic"] = group.runtime_dynamic;
            type_json["groups"].push_back(group_json);
        }

        json_array.push_back(type_json);
    }

    return json_array.dump(2);
}

std::string WebServer::serialize_execution_result(
    const ExecutionResultDto& result) const
{
    nlohmann::json json;
    json["success"] = result.success;
    json["error_message"] = result.error_message;
    json["execution_time"] = result.execution_time;

    json["output_values"] = nlohmann::json::object();
    for (const auto& [key, value] : result.output_values) {
        json["output_values"][key] = value;
    }

    return json.dump(2);
}

NodeTreeDto WebServer::deserialize_node_tree(const std::string& json) const
{
    NodeTreeDto dto;

    try {
        nlohmann::json tree_json = nlohmann::json::parse(json);

        // 解析节点
        if (tree_json.contains("nodes")) {
            for (const auto& node_json : tree_json["nodes"]) {
                NodeInstanceDto node_dto;
                node_dto.id = node_json["id"];
                node_dto.type = node_json["type"];

                if (node_json.contains("position_x"))
                    node_dto.position_x = node_json["position_x"];
                if (node_json.contains("position_y"))
                    node_dto.position_y = node_json["position_y"];

                if (node_json.contains("input_values")) {
                    for (const auto& [key, value] :
                         node_json["input_values"].items()) {
                        node_dto.input_values[key] = value.dump();
                    }
                }

                dto.nodes.push_back(node_dto);
            }
        }

        // 解析连接
        if (tree_json.contains("links")) {
            for (const auto& link_json : tree_json["links"]) {
                NodeLinkDto link_dto;
                link_dto.from_node = link_json["from_node"];
                link_dto.from_socket = link_json["from_socket"];
                link_dto.to_node = link_json["to_node"];
                link_dto.to_socket = link_json["to_socket"];
                dto.links.push_back(link_dto);
            }
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to parse node tree JSON: " + std::string(e.what()));
    }

    return dto;
}

void WebServer::setup_cors_headers(httplib::Response& res) const
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header(
        "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header(
        "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

std::unique_ptr<WebServer> create_web_server()
{
    return std::make_unique<WebServer>();
}

USTC_CG_NAMESPACE_CLOSE_SCOPE