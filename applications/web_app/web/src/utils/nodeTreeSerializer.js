// 节点树序列化器 - 将BaklavaJS节点树转换为后端可理解的格式

/**
 * 将BaklavaJS节点树序列化为后端格式
 * @param {Object} baklavaEditor - BaklavaJS编辑器实例
 * @returns {Object} 序列化的节点树
 */
export function serializeNodeTree(baklavaEditor) {
    const nodes = []
    const links = []  // 改为 links 匹配后端期望

    // 创建字符串ID到整型ID的映射
    const nodeIdMap = new Map()
    let nodeCounter = 1

    // 首先遍历所有节点，建立ID映射
    baklavaEditor.editor.graph.nodes.forEach(node => {
        nodeIdMap.set(node.id, nodeCounter++)
    })

    // 序列化节点
    baklavaEditor.editor.graph.nodes.forEach(node => {
        const serializedNode = {
            id: nodeIdMap.get(node.id),  // 使用映射后的整型ID
            type: node.type,
            title: node.title,
            position_x: node.position.x,  // 改为 position_x
            position_y: node.position.y,  // 改为 position_y
            input_values: {}  // 改为 input_values 匹配后端期望
        }

        // 序列化输入值
        Object.entries(node.inputs).forEach(([key, intf]) => {
            serializedNode.input_values[key] = intf.value
        })

        nodes.push(serializedNode)
    })

    // 序列化连接
    baklavaEditor.editor.graph.connections.forEach(connection => {
        links.push({
            from_node: nodeIdMap.get(connection.from.nodeId),    // 使用映射后的整型ID
            from_socket: connection.from.name, // 改为 from_socket  
            to_node: nodeIdMap.get(connection.to.nodeId),       // 使用映射后的整型ID
            to_socket: connection.to.name    // 改为 to_socket
        })
    })

    return {
        nodes,
        links,  // 改为 links
        metadata: {
            version: "1.0",
            createdAt: new Date().toISOString(),
            nodeCount: nodes.length,
            connectionCount: links.length,  // 更新字段名
            nodeIdMapping: Object.fromEntries(nodeIdMap)  // 可选：保存映射关系用于调试
        }
    }
}

/**
 * 验证节点树的完整性
 * @param {Object} nodeTree - 序列化的节点树
 * @returns {Object} 验证结果
 */
export function validateNodeTree(nodeTree) {
    const errors = []
    const warnings = []

    // 检查基本结构
    if (!nodeTree.nodes || !Array.isArray(nodeTree.nodes)) {
        errors.push('节点树缺少有效的节点数组')
    }

    if (!nodeTree.links || !Array.isArray(nodeTree.links)) {  // 改为 links
        errors.push('节点树缺少有效的连接数组')
    }

    if (errors.length > 0) {
        return { valid: false, errors, warnings }
    }

    // 收集所有节点ID（现在是整型）
    const nodeIds = new Set(nodeTree.nodes.map(node => node.id))

    // 验证连接的有效性
    nodeTree.links.forEach((link, index) => {  // 改为 links
        if (!nodeIds.has(link.from_node)) {      // 改为 from_node
            errors.push(`连接 ${index}: 源节点 ${link.from_node} 不存在`)
        }

        if (!nodeIds.has(link.to_node)) {        // 改为 to_node
            errors.push(`连接 ${index}: 目标节点 ${link.to_node} 不存在`)
        }
    })

    // 检查孤立节点
    const connectedNodes = new Set()
    nodeTree.links.forEach(link => {            // 改为 links
        connectedNodes.add(link.from_node)       // 改为 from_node
        connectedNodes.add(link.to_node)         // 改为 to_node
    })

    nodeTree.nodes.forEach(node => {
        if (!connectedNodes.has(node.id) && nodeTree.nodes.length > 1) {
            warnings.push(`节点 ${node.title} (${node.id}) 没有连接`)
        }
    })

    return {
        valid: errors.length === 0,
        errors,
        warnings,
        stats: {
            nodeCount: nodeTree.nodes.length,
            connectionCount: nodeTree.links.length,  // 改为 links
            isolatedNodeCount: nodeTree.nodes.length - connectedNodes.size
        }
    }
}

/**
 * 获取节点树的统计信息
 * @param {Object} nodeTree - 序列化的节点树
 * @returns {Object} 统计信息
 */
export function getNodeTreeStats(nodeTree) {
    const nodeTypeCount = {}
    const inputValueCount = {}

    nodeTree.nodes.forEach(node => {
        // 统计节点类型
        nodeTypeCount[node.type] = (nodeTypeCount[node.type] || 0) + 1

        // 统计输入值数量
        if (node.input_values) {  // 改为 input_values
            Object.entries(node.input_values).forEach(([key, value]) => {
                const valueType = typeof value
                inputValueCount[valueType] = (inputValueCount[valueType] || 0) + 1
            })
        }
    })

    return {
        totalNodes: nodeTree.nodes.length,
        totalConnections: nodeTree.links.length,  // 改为 links
        nodeTypes: nodeTypeCount,
        inputValueTypes: inputValueCount,  // 更新字段名
        complexity: calculateComplexity(nodeTree)
    }
}

/**
 * 计算节点树的复杂度
 * @param {Object} nodeTree - 序列化的节点树
 * @returns {number} 复杂度分数
 */
function calculateComplexity(nodeTree) {
    const nodeCount = nodeTree.nodes.length
    const connectionCount = nodeTree.links.length  // 改为 links

    // 简单的复杂度计算：节点数 + 连接数 * 0.5
    return nodeCount + connectionCount * 0.5
}
