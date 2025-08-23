#include "nodes/web_server/web_server.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include "entt/core/type_info.hpp"
#include "nodes/core/api.hpp"
#include "nodes/core/io/json.hpp"
#include "nodes/core/io/json_fwd.hpp"

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
        "/api/value-types",
        [this](const httplib::Request& req, httplib::Response& res) {
            handle_get_value_types(req, res);
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
}

void WebServer::handle_get_status(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    nlohmann::json json_response;
    json_response["code"] = 0;
    json_response["message"] = "success";
    json_response["data"]["status"] = "running";
    json_response["data"]["message"] = (node_system_ != nullptr)
                                           ? "RzNode Web服务器运行正常"
                                           : "尚未初始化节点系统";
    json_response["data"]["port"] = port_;
    json_response["data"]["has_node_system"] = (node_system_ != nullptr);
    res.set_content(json_response.dump(), "application/json");
    spdlog::debug("WebServer: Status request handled");
}
void WebServer::handle_get_value_types(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        // Node System未初始化 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 1;
        json_error["message"] = "Node system not available";
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        return;
    }

    try {
        refresh_value_types_cache();
    }
    catch (const std::exception& e) {
        // 后端获取值类型失败 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 2;
        json_error["message"] =
            std::string("Failed to get value types: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Error getting value types: {}", e.what());
        return;
    }

    nlohmann::json json_value_types;
    try {
        json_value_types = serialize_value_types(cached_value_types_);
    }
    catch (const std::exception& e) {
        // 序列化值类型失败 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 3;
        json_error["message"] =
            std::string("Failed to serialize value types: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Error serializing value types: {}", e.what());
        return;
    }

    nlohmann::json json_response;
    json_response["code"] = 0;
    json_response["message"] = "success";
    json_response["data"] = json_value_types;
    res.set_content(json_response.dump(), "application/json");
    spdlog::debug(
        "WebServer: Value types request handled, {} types",
        cached_value_types_.size());
}

void WebServer::handle_get_node_types(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        // Node System未初始化 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 1;
        json_error["message"] = "Node system not available";
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        return;
    }

    try {
        refresh_node_types_cache();
    }
    catch (const std::exception& e) {
        // 后端获取节点类型失败 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 2;
        json_error["message"] =
            std::string("Failed to get node types: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Error getting node types: {}", e.what());
        return;
    }

    nlohmann::json json_node_types;
    try {
        json_node_types = serialize_node_types(cached_node_types_);
    }
    catch (const std::exception& e) {
        // 序列化节点类型失败 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 3;
        json_error["message"] =
            std::string("Failed to serialize node types: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Error serializing node types: {}", e.what());
        return;
    }

    nlohmann::json json_response;
    json_response["code"] = 0;
    json_response["message"] = "success";
    json_response["data"] = json_node_types;
    res.set_content(json_response.dump(), "application/json");
    spdlog::debug(
        "WebServer: Node types request handled, {} types",
        cached_node_types_.size());
}

void WebServer::handle_execute_tree(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        // Node System未初始化 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 1;
        json_error["message"] = "Node system not available";
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        return;
    }

    NodeTreeDto tree_dto;
    try {
        tree_dto = deserialize_node_tree(req.body);
    }
    catch (const std::exception& e) {
        // 解析请求体失败 -- 400错误
        nlohmann::json json_error;
        json_error["code"] = 2;
        json_error["message"] =
            std::string("Invalid request body: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 400;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Failed to parse node tree: {}", e.what());
        return;
    }

    nlohmann::json json_execution_result;
    try {
        // 尝试执行节点树
        auto result = execute_node_tree_internal(tree_dto);

        // 转换结果为JSON
        json_execution_result["success"] = result.success;
        json_execution_result["error"] = result.error;
        json_execution_result["execution_time"] = result.execution_time;

        spdlog::info(
            "WebServer: Tree execution completed, time: {} ms",
            result.execution_time);
    }
    catch (const std::exception& e) {
        // 捕捉异常，执行失败
        json_execution_result["success"] = false;
        json_execution_result["error"] = e.what();
        json_execution_result["execution_time"] = 0.0;
        spdlog::error("WebServer: Node tree execution failed: {}", e.what());
    }

    // API 响应成功 -- HTTP 200
    nlohmann::json json_response;
    json_response["code"] = 0;
    json_response["message"] =
        json_execution_result["success"] ? "success" : "execution failed";
    json_response["data"] = json_execution_result;
    res.set_content(json_response.dump(), "application/json");
}

void WebServer::handle_validate_tree(
    const httplib::Request& req,
    httplib::Response& res)
{
    setup_cors_headers(res);

    if (!node_system_) {
        // Node System未初始化 -- 500错误
        nlohmann::json json_error;
        json_error["code"] = 1;
        json_error["message"] = "Node system not available";
        json_error["data"] = nullptr;
        res.status = 500;
        res.set_content(json_error.dump(), "application/json");
        return;
    }

    NodeTreeDto tree_dto;
    try {
        tree_dto = deserialize_node_tree(req.body);
    }
    catch (const std::exception& e) {
        // 解析请求体失败 -- 400错误
        nlohmann::json json_error;
        json_error["code"] = 2;
        json_error["message"] =
            std::string("Invalid request body: ") + e.what();
        json_error["data"] = nullptr;
        res.status = 400;
        res.set_content(json_error.dump(), "application/json");
        spdlog::error("WebServer: Failed to parse node tree: {}", e.what());
        return;
    }

    nlohmann::json json_validation_result;
    try {
        // 尝试构建节点树但不执行
        // auto tree = convert_dto_to_node_tree(tree_dto);
        update_node_tree_from_dto(node_system_->get_node_tree(), tree_dto);

        // 没有异常，验证成功
        json_validation_result["valid"] = true;
        json_validation_result["error"] = "";
        spdlog::info("WebServer: Node tree validation succeeded");
    }
    catch (const std::exception& e) {
        // 捕捉异常，验证失败
        json_validation_result["valid"] = false;
        json_validation_result["error"] = e.what();
        spdlog::warn("WebServer: Node tree validation failed: {}", e.what());
    }

    // API 响应成功 -- HTTP 200
    nlohmann::json json_response;
    json_response["code"] = 0;
    json_response["message"] =
        json_validation_result["valid"] ? "success" : "validation failed";
    json_response["data"] = json_validation_result;
    res.set_content(json_response.dump(), "application/json");
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
        switch (input->type.id()) {
            case entt::type_hash<int>().value(): socket_dto.type = "int"; break;
            case entt::type_hash<float>().value():
                socket_dto.type = "float";
                break;
            case entt::type_hash<double>().value():
                socket_dto.type = "double";
                break;
            case entt::type_hash<bool>().value():
                socket_dto.type = "bool";
                break;
            case entt::type_hash<std::string>().value():
                socket_dto.type = "string";
                break;
            default:
                socket_dto.type = std::string(input->type.info().name());
                break;
        }

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
    cached_dto_id_to_node_.clear();
    cached_dto_node_ids_.clear();
    for (const auto& node_dto : dto.nodes) {
        Node* node = tree->add_node(node_dto.type.c_str());
        if (!node) {
            throw std::runtime_error(
                "Failed to create node of type: " + node_dto.type);
        }
        cached_dto_id_to_node_[node_dto.id] = node;
        cached_dto_node_ids_.push_back(node_dto.id);

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
                        socket_identifier + "' on node " + node_dto.id + ": " +
                        e.what());
                }
            }
        }
    }

    // 创建连接
    cached_dto_id_to_link_.clear();
    cached_dto_link_ids_.clear();
    for (const auto& link_dto : dto.links) {
        auto from_node_it = cached_dto_id_to_node_.find(link_dto.from_node);
        auto to_node_it = cached_dto_id_to_node_.find(link_dto.to_node);

        if (from_node_it == cached_dto_id_to_node_.end() ||
            to_node_it == cached_dto_id_to_node_.end()) {
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

        auto link = tree->add_link(from_socket, to_socket);

        cached_dto_id_to_link_[link_dto.id] = link;
        cached_dto_link_ids_.push_back(link_dto.id);
    }

    return tree;
}

void WebServer::update_node_tree_from_dto(
    NodeTree* tree,
    const NodeTreeDto& dto) const
{
    // TODO: Implement it
    if (!tree) {
        throw std::runtime_error("Node tree is null");
    }

    // 收集本次DTO中的节点ID
    std::vector<std::string> dto_node_ids;
    for (const auto& node_dto : dto.nodes) {
        dto_node_ids.push_back(node_dto.id);
    }

    std::sort(cached_dto_node_ids_.begin(), cached_dto_node_ids_.end());
    std::sort(dto_node_ids.begin(), dto_node_ids.end());

    // 需要保留的节点（需要修改输入值）
    std::vector<std::string> nodes_to_keep;
    std::set_intersection(
        cached_dto_node_ids_.begin(),
        cached_dto_node_ids_.end(),
        dto_node_ids.begin(),
        dto_node_ids.end(),
        std::back_inserter(nodes_to_keep));
    // 需要添加的节点
    std::vector<std::string> nodes_to_add;
    std::set_difference(
        dto_node_ids.begin(),
        dto_node_ids.end(),
        cached_dto_node_ids_.begin(),
        cached_dto_node_ids_.end(),
        std::back_inserter(nodes_to_add));
    // 需要删除的节点
    std::vector<std::string> nodes_to_remove;
    std::set_difference(
        cached_dto_node_ids_.begin(),
        cached_dto_node_ids_.end(),
        dto_node_ids.begin(),
        dto_node_ids.end(),
        std::back_inserter(nodes_to_remove));

    // 1. 删除不在DTO中的节点
    for (const auto& node_id : nodes_to_remove) {
        auto it = cached_dto_id_to_node_.find(node_id);
        if (it != cached_dto_id_to_node_.end()) {
            Node* node = it->second;
            tree->delete_node(node);
            cached_dto_id_to_node_.erase(it);
        }
    }

    // 2. 添加DTO中新增的节点，并修改保留节点的输入值
    for (const auto& node_dto : dto.nodes) {
        bool is_new_node =
            (std::find(nodes_to_add.begin(), nodes_to_add.end(), node_dto.id) !=
             nodes_to_add.end());
        bool is_existing_node =
            (std::find(
                 nodes_to_keep.begin(), nodes_to_keep.end(), node_dto.id) !=
             nodes_to_keep.end());
        if (!is_new_node && !is_existing_node) {
            spdlog::warn(
                "WebServer: Node ID {} is neither new nor existing, skipping",
                node_dto.id);
            continue;  // 应该不存在这种情况
        }
        else if (is_existing_node) {
            // 修改已有节点的输入值
            auto it = cached_dto_id_to_node_.find(node_dto.id);
            if (it == cached_dto_id_to_node_.end()) {
                throw std::runtime_error(
                    "Inconsistent state: existing node ID not found in "
                    "cache: " +
                    node_dto.id);
            }
            Node* node = it->second;

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
                            socket_identifier + "' on node " + node_dto.id +
                            ": " + e.what());
                    }
                }
            }

            continue;  // 已处理完修改，继续下一个节点
        }
        else if (is_new_node) {
            // 添加新节点
            Node* node = tree->add_node(node_dto.type.c_str());
            if (!node) {
                throw std::runtime_error(
                    "Failed to create node of type: " + node_dto.type);
            }
            cached_dto_id_to_node_[node_dto.id] = node;
            cached_dto_node_ids_.push_back(node_dto.id);

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
                            socket_identifier + "' on node " + node_dto.id +
                            ": " + e.what());
                    }
                }
            }

            continue;  // 已处理完添加，继续下一个节点
        }
    }

    // 收集本次DTO中的连接ID
    std::vector<std::string> dto_link_ids;
    for (const auto& link_dto : dto.links) {
        dto_link_ids.push_back(link_dto.id);
    }

    std::sort(cached_dto_link_ids_.begin(), cached_dto_link_ids_.end());
    std::sort(dto_link_ids.begin(), dto_link_ids.end());

    // 需要添加的连接
    std::vector<std::string> links_to_add;
    std::set_difference(
        dto_link_ids.begin(),
        dto_link_ids.end(),
        cached_dto_link_ids_.begin(),
        cached_dto_link_ids_.end(),
        std::back_inserter(links_to_add));
    // 需要删除的连接
    std::vector<std::string> links_to_remove;
    std::set_difference(
        cached_dto_link_ids_.begin(),
        cached_dto_link_ids_.end(),
        dto_link_ids.begin(),
        dto_link_ids.end(),
        std::back_inserter(links_to_remove));

    // 3. 删除不在DTO中的连接
    for (const auto& link_id : links_to_remove) {
        auto it = cached_dto_id_to_link_.find(link_id);
        if (it != cached_dto_id_to_link_.end()) {
            NodeLink* link = it->second;
            tree->delete_link(link);
            cached_dto_id_to_link_.erase(it);
        }
    }

    // 4. 添加DTO中新增的连接
    for (const auto& link_dto : dto.links) {
        auto it =
            std::find(links_to_add.begin(), links_to_add.end(), link_dto.id);
        if (it == links_to_add.end()) {
            continue;  // 不是新增连接
        }

        auto from_node_it = cached_dto_id_to_node_.find(link_dto.from_node);
        auto to_node_it = cached_dto_id_to_node_.find(link_dto.to_node);

        if (from_node_it == cached_dto_id_to_node_.end() ||
            to_node_it == cached_dto_id_to_node_.end()) {
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

        auto link = tree->add_link(from_socket, to_socket);

        cached_dto_id_to_link_[link_dto.id] = link;
        cached_dto_link_ids_.push_back(link_dto.id);
    }
}

ExecutionResultDto WebServer::execute_node_tree_internal(
    const NodeTreeDto& dto) const
{
    // TODO: 不再重新创建NodeTree实例，而是复用已有实例，以保留节点中可能的缓存
    ExecutionResultDto result;
    auto start_time = std::chrono::high_resolution_clock::now();

    // 从 DTO 创建节点树
    // auto tree = convert_dto_to_node_tree(dto);
    update_node_tree_from_dto(node_system_->get_node_tree(), dto);

    // 将新的节点树设置到 NodeSystem 中
    // node_system_->set_node_tree(std::move(tree));

    // 执行节点树
    node_system_->execute(false);  // 非UI执行

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    result.success = true;
    result.error = "";
    result.execution_time = duration.count();

    return result;
}

void WebServer::refresh_value_types_cache() const
{
    if (!value_types_cache_dirty_) {
        return;
    }

    cached_value_types_.clear();

    if (!node_system_) {
        return;
    }

    auto descriptor = node_system_->node_tree_descriptor();
    auto value_types = descriptor->get_registered_value_types();
    for (const auto& type : value_types) {
        switch (type.id()) {
            case entt::type_hash<int>().value():
                cached_value_types_.push_back("int");
                break;
            case entt::type_hash<float>().value():
                cached_value_types_.push_back("float");
                break;
            case entt::type_hash<double>().value():
                cached_value_types_.push_back("double");
                break;
            case entt::type_hash<bool>().value():
                cached_value_types_.push_back("bool");
                break;
            case entt::type_hash<std::string>().value():
                cached_value_types_.push_back("string");
                break;  // 支持的基本类型
            default:
                // 其他类型使用其名称
                cached_value_types_.push_back(std::string(type.info().name()));
                break;
        }
    }

    value_types_cache_dirty_ = false;
    spdlog::debug(
        "WebServer: Value types cache refreshed, {} types",
        cached_value_types_.size());
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

nlohmann::json WebServer::serialize_value_types(
    const std::vector<std::string>& types) const
{
    nlohmann::json json_array = nlohmann::json::array();

    for (const auto& type : types) {
        nlohmann::json type_json;
        type_json["type_name"] = type;
        json_array.push_back(type_json);
    }

    return json_array;
}

nlohmann::json WebServer::serialize_node_types(
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

    return json_array;
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
                link_dto.id = link_json["id"];
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