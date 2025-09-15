#include "nodes/web_server/util.hpp"

#include <cassert>
#include <string>

#include "entt/core/type_info.hpp"
#include "nodes/core/io/json.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/web_server/dto.hpp"
#include "oatpp/Types.hpp"
#include "oatpp/data/type/Object.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE

StaticFilesManager::StaticFilesManager()
{
    {
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
        }
#endif

        // 构建web/dist的绝对路径
        std::filesystem::path web_dist_path = executable_path / "web" / "dist";
        web_dist_path = web_dist_path.lexically_normal();

        // 检查路径是否存在
        if (std::filesystem::exists(web_dist_path)) {
            spdlog::info(
                "WebServer: Mounting static files from: {}",
                web_dist_path.string());
        }
        else {
            web_dist_path = "./web/dist";  // 回退到相对路径
            spdlog::warn(
                "WebServer: Web directory not found at: {}, using relative "
                "path",
                web_dist_path.string());
        }
        base_path_ = web_dist_path.string();
    }
}

StaticFilesManager::StaticFilesManager(const oatpp::String& base_path)
    : base_path_(base_path)
{
}

oatpp::String StaticFilesManager::getFile(const oatpp::String& path)
{
    if (!path) {
        return nullptr;
    }
    std::lock_guard<oatpp::concurrency::SpinLock> lock(lock_);
    auto& file = cache_[path];
    if (file) {
        return file;
    }
    oatpp::String filename = base_path_ + "/" + path;
    file = oatpp::String::loadFromFile(filename->c_str());
    return file;
}

oatpp::String StaticFilesManager::getFileMIMEType(const oatpp::String& path)
{
    if (!path) {
        return nullptr;
    }
    auto ext_pos = path->find_last_of('.');
    if (ext_pos == std::string::npos) {
        return "application/octet-stream";  // 默认二进制流
    }
    auto ext = path->substr(ext_pos);
    if (ext == ".html" || ext == ".htm") {
        return "text/html";
    }
    else if (ext == ".css") {
        return "text/css";
    }
    else if (ext == ".js") {
        return "application/javascript";
    }
    else if (ext == ".json") {
        return "application/json";
    }
    else if (ext == ".png") {
        return "image/png";
    }
    else if (ext == ".jpg" || ext == ".jpeg") {
        return "image/jpeg";
    }
    else if (ext == ".gif") {
        return "image/gif";
    }
    else if (ext == ".svg") {
        return "image/svg+xml";
    }
    else if (ext == ".ico") {
        return "image/x-icon";
    }
    else if (ext == ".txt") {
        return "text/plain";
    }
    else if (ext == ".pdf") {
        return "application/pdf";
    }
    // 可根据需要添加更多类型
    return "application/octet-stream";  // 默认二进制流
}

void NodeSystemComponent::set_node_system(
    std::shared_ptr<NodeSystem> node_system)
{
    node_system_ = node_system;
    value_types_cache_dirty_ = true;
    cached_value_types_ = ValueTypesDto::createShared();
    node_types_cache_dirty_ = true;
    cached_node_types_ = NodeTypesDto::createShared();
}

std::shared_ptr<NodeSystem> NodeSystemComponent::get_node_system() const
{
    return node_system_;
}

bool NodeSystemComponent::node_system_attached() const
{
    return node_system_ == nullptr ? false : true;
}

oatpp::Object<ValueTypesDto> NodeSystemComponent::get_value_types() const
{
    refresh_value_types_cache();
    return cached_value_types_;
}

oatpp::Object<NodeTypesDto> NodeSystemComponent::get_node_types() const
{
    refresh_node_types_cache();
    return cached_node_types_;
}

bool compare_oatpp_string(const oatpp::String& a, const oatpp::String& b)
{
    return a->c_str() < b->c_str();
}

void NodeSystemComponent::update_node_tree_from_dto(
    NodeTree* tree,
    const oatpp::Object<NodeTreeDto>& node_tree_dto) const
{
    if (!tree) {
        throw std::runtime_error("Node tree is null");
    }

    // 收集本次DTO中的节点ID
    std::vector<std::string> dto_node_ids;
    for (const auto& node_dto : *node_tree_dto->nodes) {
        dto_node_ids.push_back(node_dto->id);
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
    for (const auto& node_dto : *node_tree_dto->nodes) {
        bool is_new_node =
            (std::find(
                 nodes_to_add.begin(), nodes_to_add.end(), *node_dto->id) !=
             nodes_to_add.end());
        bool is_existing_node =
            (std::find(
                 nodes_to_keep.begin(), nodes_to_keep.end(), *node_dto->id) !=
             nodes_to_keep.end());
        if (!is_new_node && !is_existing_node) {
            spdlog::warn(
                "WebServer: Node ID {} is neither new nor existing, "
                "skipping",
                *node_dto->id);
            continue;  // 应该不存在这种情况
        }
        else if (is_existing_node) {
            // 修改已有节点的输入值
            auto it = cached_dto_id_to_node_.find(*node_dto->id);
            if (it == cached_dto_id_to_node_.end()) {
                throw std::runtime_error(
                    "Inconsistent state: existing node ID not found in "
                    "cache: " +
                    node_dto->id);
            }
            Node* node = it->second;

            // 设置输入值
            for (const auto& [socket_identifier, any_type_value] :
                 *node_dto->input_values) {
                NodeSocket* input_socket =
                    node->get_input_socket(socket_identifier->c_str());
                if (input_socket && input_socket->dataField.value) {
                    try {
                        // 根据接口的类型，将any_type_value转换为对应类型
                        switch (input_socket->dataField.value.type().id()) {
                            case entt::type_hash<int>().value():
                                input_socket->dataField.value = int(
                                    *any_type_value.retrieve<oatpp::Int64>());
                                break;
                            case entt::type_hash<float>().value():
                                if (any_type_value.getStoredType() ==
                                    oatpp::Float64::Class::getType()) {
                                    input_socket->dataField.value =
                                        float(*any_type_value
                                                   .retrieve<oatpp::Float64>());
                                }
                                else if (
                                    any_type_value.getStoredType() ==
                                    oatpp::Int64::Class::getType()) {
                                    input_socket->dataField.value =
                                        float(*any_type_value
                                                   .retrieve<oatpp::Int64>());
                                }
                                break;
                            case entt::type_hash<double>().value():
                                if (any_type_value.getStoredType() ==
                                    oatpp::Float64::Class::getType()) {
                                    input_socket->dataField.value = double(
                                        *any_type_value
                                             .retrieve<oatpp::Float64>());
                                }
                                else if (
                                    any_type_value.getStoredType() ==
                                    oatpp::Int64::Class::getType()) {
                                    input_socket->dataField.value =
                                        double(*any_type_value
                                                    .retrieve<oatpp::Int64>());
                                }
                                break;
                            case entt::type_hash<bool>().value():
                                input_socket->dataField.value = bool(
                                    *any_type_value.retrieve<oatpp::Boolean>());
                                break;
                            case entt::type_hash<std::string>().value():
                                input_socket->dataField.value = std::string(
                                    *any_type_value.retrieve<oatpp::String>());
                                break;
                            default:
                                throw std::runtime_error(
                                    "Unsupported input socket type for socket "
                                    "'" +
                                    socket_identifier + "' on node " +
                                    node_dto->id);
                        }
                    }
                    catch (const std::exception& e) {
                        throw std::runtime_error(
                            "Failed to set input value for socket '" +
                            socket_identifier + "' on node " + node_dto->id +
                            ": " + e.what());
                    }
                }
            }

            continue;  // 已处理完修改，继续下一个节点
        }
        else if (is_new_node) {
            // 添加新节点
            Node* node = tree->add_node(node_dto->type->c_str());
            if (!node) {
                throw std::runtime_error(
                    "Failed to create node of type: " + node_dto->type);
            }
            cached_dto_id_to_node_[node_dto->id] = node;
            cached_dto_node_ids_.push_back(node_dto->id);

            // 设置输入值
            for (const auto& [socket_identifier, any_type_value] :
                 *node_dto->input_values) {
                NodeSocket* input_socket =
                    node->get_input_socket(socket_identifier->c_str());
                if (input_socket && input_socket->dataField.value) {
                    try {
                        // 根据接口的类型，将any_type_value转换为对应类型
                        switch (input_socket->dataField.value.type().id()) {
                            case entt::type_hash<int>().value():
                                input_socket->dataField.value = int(
                                    *any_type_value.retrieve<oatpp::Int64>());
                                break;
                            case entt::type_hash<float>().value():
                                if (any_type_value.getStoredType() ==
                                    oatpp::Float64::Class::getType()) {
                                    input_socket->dataField.value =
                                        float(*any_type_value
                                                   .retrieve<oatpp::Float64>());
                                }
                                else if (
                                    any_type_value.getStoredType() ==
                                    oatpp::Int64::Class::getType()) {
                                    input_socket->dataField.value =
                                        float(*any_type_value
                                                   .retrieve<oatpp::Int64>());
                                }
                                break;
                            case entt::type_hash<double>().value():
                                if (any_type_value.getStoredType() ==
                                    oatpp::Float64::Class::getType()) {
                                    input_socket->dataField.value = double(
                                        *any_type_value
                                             .retrieve<oatpp::Float64>());
                                }
                                else if (
                                    any_type_value.getStoredType() ==
                                    oatpp::Int64::Class::getType()) {
                                    input_socket->dataField.value =
                                        double(*any_type_value
                                                    .retrieve<oatpp::Int64>());
                                }
                                break;
                            case entt::type_hash<bool>().value():
                                input_socket->dataField.value = bool(
                                    *any_type_value.retrieve<oatpp::Boolean>());
                                break;
                            case entt::type_hash<std::string>().value():
                                input_socket->dataField.value = std::string(
                                    *any_type_value.retrieve<oatpp::String>());
                                break;
                            default:
                                throw std::runtime_error(
                                    "Unsupported input socket type for socket "
                                    "'" +
                                    socket_identifier + "' on node " +
                                    node_dto->id);
                        }
                    }
                    catch (const std::exception& e) {
                        throw std::runtime_error(
                            "Failed to set input value for socket '" +
                            socket_identifier + "' on node " + node_dto->id +
                            ": " + e.what());
                    }
                }
            }

            continue;  // 已处理完添加，继续下一个节点
        }
    }

    // 收集本次DTO中的连接ID
    std::vector<std::string> dto_link_ids;
    for (const auto& link_dto : *node_tree_dto->links) {
        dto_link_ids.push_back(link_dto->id);
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
    for (const auto& link_dto : *node_tree_dto->links) {
        auto it =
            std::find(links_to_add.begin(), links_to_add.end(), *link_dto->id);
        if (it == links_to_add.end()) {
            continue;  // 不是新增连接
        }

        auto from_node_it = cached_dto_id_to_node_.find(link_dto->from_node);
        auto to_node_it = cached_dto_id_to_node_.find(link_dto->to_node);

        if (from_node_it == cached_dto_id_to_node_.end() ||
            to_node_it == cached_dto_id_to_node_.end()) {
            throw std::runtime_error("Invalid node ID in link");
        }

        Node* from_node = from_node_it->second;
        Node* to_node = to_node_it->second;

        NodeSocket* from_socket =
            from_node->get_output_socket(link_dto->from_socket->c_str());
        NodeSocket* to_socket =
            to_node->get_input_socket(link_dto->to_socket->c_str());

        if (!from_socket || !to_socket) {
            throw std::runtime_error("Invalid socket identifier in link");
        }

        auto link = tree->add_link(from_socket, to_socket);

        cached_dto_id_to_link_[link_dto->id] = link;
        cached_dto_link_ids_.push_back(link_dto->id);
    }
}

oatpp::Object<ExecutionResultDto> NodeSystemComponent::execute_node_tree(
    const oatpp::Object<NodeTreeDto>& node_tree_dto) const
{
    auto result_dto = ExecutionResultDto::createShared();
    auto start_time = std::chrono::high_resolution_clock::now();

    // 从 DTO 创建节点树
    // auto tree = convert_dto_to_node_tree(dto);
    update_node_tree_from_dto(node_system_->get_node_tree(), node_tree_dto);

    // 执行节点树
    node_system_->execute(false);  // 非UI执行

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    result_dto->success = true;
    result_dto->error = "";
    result_dto->execution_time = duration.count();

    return result_dto;
}

void NodeSystemComponent::refresh_value_types_cache() const
{
    if (!value_types_cache_dirty_) {
        return;
    }

    cached_value_types_->value_types->clear();

    if (!node_system_) {
        return;
    }

    auto descriptor = node_system_->node_tree_descriptor();
    auto value_types = descriptor->get_registered_value_types();
    for (const auto& type : value_types) {
        auto value_type_dto = convert_value_type_to_dto(type);
        cached_value_types_->value_types->push_back(value_type_dto);
    }

    value_types_cache_dirty_ = false;
    spdlog::debug(
        "WebServer: Value types cache refreshed, {} types",
        cached_value_types_->value_types->size());
}

void NodeSystemComponent::refresh_node_types_cache() const
{
    if (!node_types_cache_dirty_) {
        return;
    }

    cached_node_types_->node_types->clear();

    if (!node_system_) {
        return;
    }

    auto descriptor = node_system_->node_tree_descriptor();
    auto registered_types = descriptor->get_registered_node_types();

    for (const auto& [type_name, type_info] : registered_types) {
        auto node_type_dto = convert_node_type_to_dto(type_info);
        cached_node_types_->node_types->push_back(node_type_dto);
    }

    node_types_cache_dirty_ = false;
    spdlog::debug(
        "WebServer: Node types cache refreshed, {} types",
        cached_node_types_->node_types->size());
}

oatpp::Object<ValueTypeDto> NodeSystemComponent::convert_value_type_to_dto(
    const entt::meta_type& type) const
{
    auto value_type_dto = ValueTypeDto::createShared();
    switch (type.id()) {
        case entt::type_hash<int>().value():
            value_type_dto->type_name = "int";
            break;
        case entt::type_hash<float>().value():
            value_type_dto->type_name = "float";
            break;
        case entt::type_hash<double>().value():
            value_type_dto->type_name = "double";
            break;
        case entt::type_hash<bool>().value():
            value_type_dto->type_name = "bool";
            break;
        case entt::type_hash<std::string>().value():
            value_type_dto->type_name = "string";
            break;  // 支持的基本类型
        default:
            // 其他类型使用其名称
            value_type_dto->type_name = std::string(type.info().name());
            break;
    }
    return value_type_dto;
}

oatpp::Object<NodeTypeDto> NodeSystemComponent::convert_node_type_to_dto(
    const NodeTypeInfo& type_info) const
{
    auto node_type_dto = NodeTypeDto::createShared();
    node_type_dto->id_name = type_info.id_name;
    node_type_dto->ui_name = type_info.ui_name;

    // 复制颜色信息
    node_type_dto->color = {
        type_info.color[0],
        type_info.color[1],
        type_info.color[2],
        type_info.color[3],
    };

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
            "WebServer: Failed to create temporary node for "
            "{}: {}",
            type_info.id_name,
            e.what());
    }

    for (const auto& input : declaration.inputs) {
        auto socket_dto = NodeTypeDto::SocketDto::createShared();
        socket_dto->name = input->name;
        socket_dto->identifier = input->identifier;
        switch (input->type.id()) {
            case entt::type_hash<int>().value():
                socket_dto->type = "int";
                break;
            case entt::type_hash<float>().value():
                socket_dto->type = "float";
                break;
            case entt::type_hash<double>().value():
                socket_dto->type = "double";
                break;
            case entt::type_hash<bool>().value():
                socket_dto->type = "bool";
                break;
            case entt::type_hash<std::string>().value():
                socket_dto->type = "string";
                break;
            default:
                socket_dto->type = std::string(input->type.info().name());
                break;
        }

        // 从临时节点实例提取默认值、最小值、最大值等信息
        // 注意：这些值在temp_tree->add_node()调用时已经通过update_default_value()自动设置到dataField中
        if (temp_node) {
            NodeSocket* temp_socket =
                temp_node->get_input_socket(input->identifier.c_str());
            if (temp_socket) {
                // 设置optional属性
                socket_dto->optional = temp_socket->optional;

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
                    socket_dto->default_value =
                        extract_value(temp_socket->dataField.value, type_id);
                }

                // 提取最小值（只有支持min值的类型dataField才会有min）
                if (temp_socket->dataField.min) {
                    socket_dto->min_value =
                        extract_value(temp_socket->dataField.min, type_id);
                }

                // 提取最大值（只有支持max值的类型dataField才会有max）
                if (temp_socket->dataField.max) {
                    socket_dto->max_value =
                        extract_value(temp_socket->dataField.max, type_id);
                }
            }
        }

        node_type_dto->inputs->push_back(socket_dto);
    }

    for (const auto& output : declaration.outputs) {
        auto socket_dto = NodeTypeDto::SocketDto::createShared();
        socket_dto->name = output->name;
        socket_dto->identifier = output->identifier;
        socket_dto->type = get_type_name(output->type);
        node_type_dto->outputs->push_back(socket_dto);
    }

    // 转换socket groups
    for (const auto& group : declaration.socket_group_decls) {
        auto group_dto = NodeTypeDto::SocketGroupDto::createShared();
        group_dto->identifier = group->identifier;
        group_dto->type =
            (group->in_out == PinKind::Input) ? "input" : "output";
        group_dto->element_type = get_type_name(group->type);
        group_dto->runtime_dynamic = group->runtime_dynamic;
        node_type_dto->groups->push_back(group_dto);
    }

    return node_type_dto;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE