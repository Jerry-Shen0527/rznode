#pragma once

#include <memory>

#include "entt/meta/meta.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/web_server_oatpp/api.h"
#include "nodes/web_server_oatpp/dto.hpp"
#include "oatpp/Types.hpp"
#include "oatpp/concurrency/SpinLock.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>

#include <climits>

#endif

USTC_CG_NAMESPACE_OPEN_SCOPE

/**
 * 参考文档：
 * https://github.com/oatpp/oatpp/issues/569
 */
class WEB_SERVER_OATPP_API StaticFilesManager {
   public:
    StaticFilesManager();

    StaticFilesManager(const oatpp::String& base_path);

    oatpp::String getFile(const oatpp::String& path);

    oatpp::String getFileMIMEType(const oatpp::String& path);

   private:
    oatpp::String base_path_;
    oatpp::concurrency::SpinLock lock_;
    std::unordered_map<oatpp::String, oatpp::String> cache_;
};

class WEB_SERVER_OATPP_API NodeSystemComponent {
   public:
    void set_node_system(std::shared_ptr<NodeSystem> node_system);
    std::shared_ptr<NodeSystem> get_node_system() const;
    bool node_system_attached() const;
    oatpp::Object<ValueTypesDto> get_value_types() const;
    oatpp::Object<NodeTypesDto> get_node_types() const;
    void update_node_tree_from_dto(
        NodeTree* tree,
        const oatpp::Object<NodeTreeDto>& dto) const;
    oatpp::Object<ExecutionResultDto> execute_node_tree(
        const oatpp::Object<NodeTreeDto>& dto) const;

   private:
    std::shared_ptr<NodeSystem> node_system_;

    // 缓存的 Node DTO id 到 Node* 的映射（用于更新节点树）
    mutable std::unordered_map<std::string, Node*> cached_dto_id_to_node_;
    mutable std::vector<std::string> cached_dto_node_ids_;

    // 缓存的 Link DTO id 到 NodeLink* 的映射（用于更新节点树）
    mutable std::unordered_map<std::string, NodeLink*> cached_dto_id_to_link_;
    mutable std::vector<std::string> cached_dto_link_ids_;

    // 缓存的接口类型信息
    void refresh_value_types_cache() const;
    mutable oatpp::Object<ValueTypesDto> cached_value_types_;
    mutable bool value_types_cache_dirty_;

    // 缓存的节点类型信息
    void refresh_node_types_cache() const;
    mutable oatpp::Object<NodeTypesDto> cached_node_types_;
    mutable bool node_types_cache_dirty_;

    // NodeSystem -> DTO 转换辅助函数
    oatpp::Object<ValueTypeDto> convert_value_type_to_dto(
        const entt::meta_type& type) const;
    oatpp::Object<NodeTypeDto> convert_node_type_to_dto(
        const NodeTypeInfo& type_info) const;
};

USTC_CG_NAMESPACE_CLOSE_SCOPE