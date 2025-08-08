#include "nodes/web_server/web_server.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>

#include "nodes/core/api.hpp"
#include "nodes/core/io/json.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>

#include <climits>

#endif

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

    // 获取可执行程序所在目录的绝对路径（参考NodeDynamicLoadingSystem的实现）
    std::filesystem::path executable_path;

#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    executable_path = std::filesystem::path(path).parent_path();
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        executable_path = std::filesystem::path(path).parent_path();
    }
    else {
        spdlog::error("WebServer: Failed to get executable path");
        server_->set_mount_point("/", "./web/dist");  // 回退到相对路径
        spdlog::info("WebServer: Initialized on port {}", port_);
        return true;
    }
#endif

    // 构建web/dist的绝对路径
    std::filesystem::path web_dist_path = executable_path / "web" / "dist";
    web_dist_path = web_dist_path.lexically_normal();

    // 检查路径是否存在
    if (std::filesystem::exists(web_dist_path)) {
        server_->set_mount_point("/", web_dist_path.string());
        spdlog::info(
            "WebServer: Mounting static files from: {}",
            web_dist_path.string());
    }
    else {
        spdlog::warn(
            "WebServer: Web directory not found at: {}, using relative path",
            web_dist_path.string());
        server_->set_mount_point("/", "./web/dist");  // 回退到相对路径
    }

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

    // 注意：移除了硬编码的根路由 server_->Get("/", ...)
    // 现在根路径由静态文件服务 set_mount_point("/", "./web/dist") 处理
    // 这样可以使用真正的React/Vue前端节点编辑器界面
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

    // 创建临时节点实例来提取默认值、最小值、最大值等信息
    // 这是必要的，因为这些值只有在节点实例化时才会从模板化的Decl类传递到NodeSocket
    std::unique_ptr<NodeTree> temp_tree;
    Node* temp_node = nullptr;

    try {
        if (node_system_) {
            auto temp_descriptor = node_system_->node_tree_descriptor();
            temp_tree = create_node_tree(temp_descriptor);
            temp_node = temp_tree->add_node(type_info.id_name.c_str());
        }
    }
    catch (const std::exception& e) {
        // 如果创建临时节点失败，记录警告但继续处理基本信息
        spdlog::warn(
            "WebServer: Failed to create temporary node for {}: {}",
            type_info.id_name,
            e.what());
    }

    for (const auto& input : declaration.inputs) {
        NodeTypeDto::SocketDto socket_dto;
        socket_dto.name = input->name;
        socket_dto.identifier = input->identifier;
        socket_dto.type = get_type_name(input->type);

        // 从临时节点实例提取默认值、最小值、最大值等信息
        // 注意：这些值在temp_tree->add_node()调用时已经通过update_default_value()自动设置到dataField中
        if (temp_node) {
            NodeSocket* temp_socket =
                temp_node->get_input_socket(input->identifier.c_str());
            if (temp_socket) {
                // 设置optional属性
                socket_dto.optional = temp_socket->optional;

                // 使用帮助函数来提取值，减少重复代码
                auto extract_value = [](const entt::meta_any& any_value,
                                        entt::id_type type_id) -> std::string {
                    try {
                        if (type_id == entt::type_hash<int>()) {
                            return std::to_string(any_value.cast<int>());
                        }
                        else if (type_id == entt::type_hash<float>()) {
                            return std::to_string(any_value.cast<float>());
                        }
                        else if (type_id == entt::type_hash<double>()) {
                            return std::to_string(any_value.cast<double>());
                        }
                        else if (type_id == entt::type_hash<bool>()) {
                            return any_value.cast<bool>() ? "true" : "false";
                        }
                        else if (type_id == entt::type_hash<std::string>()) {
                            return "\"" + any_value.cast<std::string>() + "\"";
                        }
                        return "";
                    }
                    catch (const std::exception&) {
                        return "";
                    }
                };

                auto type_id = temp_socket->type_info.id();

                // 提取默认值（根据ValueTrait，所有支持的类型都有默认值）
                if (temp_socket->dataField.value) {
                    socket_dto.default_value =
                        extract_value(temp_socket->dataField.value, type_id);
                }

                // 提取最小值（只有支持min值的类型dataField才会有min）
                if (temp_socket->dataField.min) {
                    socket_dto.min_value =
                        extract_value(temp_socket->dataField.min, type_id);
                }

                // 提取最大值（只有支持max值的类型dataField才会有max）
                if (temp_socket->dataField.max) {
                    socket_dto.max_value =
                        extract_value(temp_socket->dataField.max, type_id);
                }
            }
        }

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

        // 设置输入值
        for (const auto& [socket_identifier, json_value_str] :
             node_dto.input_values) {
            NodeSocket* input_socket =
                node->get_input_socket(socket_identifier.c_str());
            if (input_socket && input_socket->dataField.value) {
                try {
                    // 解析JSON字符串
                    nlohmann::json json_value =
                        nlohmann::json::parse(json_value_str);

                    // 构造符合DeserializeValue期望的格式
                    nlohmann::json socket_data;
                    socket_data["value"] = json_value;

                    // 使用已有的反序列化方法设置值
                    input_socket->DeserializeValue(socket_data);
                }
                catch (const std::exception& e) {
                    throw std::runtime_error(
                        "Failed to set input value for socket '" +
                        socket_identifier + "' on node " +
                        std::to_string(node_dto.id) + ": " + e.what());
                }
            }
        }
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
        // 从 DTO 创建节点树
        auto tree = convert_dto_to_node_tree(dto);

        // 将新的节点树设置到 NodeSystem 中
        node_system_->set_node_tree(std::move(tree));

        // 执行节点树
        node_system_->execute(false);  // 非UI执行

        result.success = true;
        result.error_message = "";
    }
    catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
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