#include "nodes/system/node_system.hpp"

#include "nodes/core/node_exec_eager.hpp"
#include "nodes/system/node_system_dl.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE
void NodeSystem::init()
{
    this->node_tree = create_node_tree(node_tree_descriptor());
    if (!node_tree_executor)
        this->node_tree_executor = create_node_tree_executor({});
}

void NodeSystem::init(std::unique_ptr<NodeTree> tree)
{
    this->node_tree = std::move(tree);
    if (!node_tree_executor)
        this->node_tree_executor = create_node_tree_executor({});
}

void NodeSystem::set_node_tree_executor(
    std::unique_ptr<NodeTreeExecutor> executor)
{
    node_tree_executor = std::move(executor);
}

NodeSystem::~NodeSystem()
{
}

void NodeSystem::finalize()
{
    if (node_tree_executor) {
        node_tree_executor->finalize(node_tree.get());
    }
}

void NodeSystem::execute(bool is_ui_execution, Node* required_node) const
{
    if (is_ui_execution && !allow_ui_execution) {
        return;
    }
    if (node_tree_executor) {
        return node_tree_executor->execute(node_tree.get(), required_node);
    }
}

void NodeSystem::set_node_tree(std::unique_ptr<NodeTree> new_tree)
{
    // 如果有执行器，先清理当前节点树
    if (node_tree_executor && node_tree) {
        node_tree_executor->finalize(node_tree.get());
    }
    
    // 设置新的节点树
    node_tree = std::move(new_tree);
    
    // 如果还没有执行器，创建一个默认的
    if (!node_tree_executor) {
        node_tree_executor = create_node_tree_executor({});
    }
}

NodeTree* NodeSystem::get_node_tree() const
{
    return node_tree.get();
}

NodeTreeExecutor* NodeSystem::get_node_tree_executor() const
{
    return node_tree_executor.get();
}

std::shared_ptr<NodeSystem> create_dynamic_loading_system()
{
    return std::make_shared<NodeDynamicLoadingSystem>();
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
