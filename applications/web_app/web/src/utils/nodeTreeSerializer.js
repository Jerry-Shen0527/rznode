// 节点树序列化器 - 将BaklavaJS节点树转换为后端可理解的格式

/**
 * 将BaklavaJS节点树序列化为后端格式（支持子图）
 * @param {Object} baklavaEditor - BaklavaJS编辑器实例
 * @returns {Object} 序列化的节点树
 */
export function serializeNodeTree(baklavaEditor) {
    const nodes = []
    const links = []

    // 创建字符串ID到整型ID的映射
    const nodeIdMap = new Map()
    let nodeCounter = 1

    // 先遍历所有图中的普通节点，建立ID映射
    baklavaEditor.editor.graphs.forEach(graph => {
        graph.nodes.forEach(node => {
            if (!node.type.startsWith('__baklava_')) {
                // 只处理普通节点，跳过BaklavaJS内部节点
                if (!nodeIdMap.has(node.id)) {
                    nodeIdMap.set(node.id, nodeCounter++)
                }
            }
        })
    })

    /**
     * 递归处理图，支持子图展开
     * @param {Object} graph - 当前处理的图
     * @param {Object} inputs - 输入接口映射 {interfaceId: {fromNodeId, fromSocket}}
     * @param {Object} outputs - 输出接口映射 {interfaceId: {toNodeId, toSocket}}
     */
    function processGraphRecursively(graph, inputs = {}, outputs = {}) {
        // 处理当前图中的所有节点
        graph.nodes.forEach(node => {
            if (node.type.startsWith('__baklava_GraphNode')) {
                // 子图节点：需要递归处理其内部子图
                console.log(`处理子图节点: ${node.id}`)

                // 建立子图的输入输出映射，组合当前层的映射
                const subgraphInputs = {}
                const subgraphOutputs = {}

                // 遍历子图节点的输入接口，建立组合映射关系
                Object.entries(node.inputs).forEach(([inputKey, intf]) => {
                    // 查找连接到这个接口的内部连接
                    graph.connections.forEach(conn => {
                        if (conn.to.nodeId === node.id && conn.to.id === intf.id) {
                            const fromNode = graph.findNodeById(conn.from.nodeId)

                            if (fromNode.type.startsWith('__baklava_SubgraphInputNode')) {
                                // 情况4：SubgraphInput → 子图节点
                                // 将当前层的inputs映射组合到子图层
                                // 使用 inputKey 作为映射键，因为 graphInterfaceId 对应的是子图节点的输入键名
                                const externalInput = inputs[inputKey]
                                if (externalInput) {
                                    subgraphInputs[inputKey] = externalInput
                                }
                            } else if (!fromNode.type.startsWith('__baklava_')) {
                                // 普通节点 → 子图节点：直接建立映射
                                subgraphInputs[inputKey] = {
                                    fromNodeId: conn.from.nodeId,
                                    fromSocket: conn.from.name
                                }
                            }
                        }
                    })
                })

                // 遍历子图节点的输出接口，建立组合映射关系
                Object.entries(node.outputs).forEach(([outputKey, intf]) => {
                    // 查找从这个接口连出的内部连接
                    graph.connections.forEach(conn => {
                        if (conn.from.nodeId === node.id && conn.from.id === intf.id) {
                            const toNode = graph.findNodeById(conn.to.nodeId)

                            if (toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                                // 情况5：子图节点 → SubgraphOutput
                                // 将子图层的映射组合到当前层的outputs
                                // 使用 outputKey 作为映射键，因为 graphInterfaceId 对应的是子图节点的输出键名
                                const externalOutput = outputs[outputKey]
                                if (externalOutput) {
                                    subgraphOutputs[outputKey] = externalOutput
                                }
                            } else if (!toNode.type.startsWith('__baklava_')) {
                                // 子图节点 → 普通节点：直接建立映射
                                subgraphOutputs[outputKey] = {
                                    toNodeId: conn.to.nodeId,
                                    toSocket: conn.to.name
                                }
                            }
                        }
                    })
                })

                // 递归处理子图，传递组合后的映射
                processGraphRecursively(node.subgraph, subgraphInputs, subgraphOutputs)

            } else if (!node.type.startsWith('__baklava_')) {
                // 普通节点：直接序列化
                console.log(`处理普通节点: ${node.id}, type: ${node.type}`)

                // 序列化节点
                const serializedNode = {
                    id: nodeIdMap.get(node.id),
                    type: node.type,
                    title: node.title,
                    position_x: node.position.x,
                    position_y: node.position.y,
                    input_values: {}
                }

                // 序列化输入值
                Object.entries(node.inputs).forEach(([key, intf]) => {
                    if (intf.value !== undefined) {
                        serializedNode.input_values[key] = intf.value
                    }
                })

                nodes.push(serializedNode)
            }
        })

        // 处理当前图中普通节点之间的连接
        graph.connections.forEach(connection => {
            const fromNode = graph.findNodeById(connection.from.nodeId)
            const toNode = graph.findNodeById(connection.to.nodeId)

            // 只处理普通节点之间的连接
            if (fromNode && toNode &&
                !fromNode.type.startsWith('__baklava_') &&
                !toNode.type.startsWith('__baklava_')) {

                links.push({
                    from_node: nodeIdMap.get(connection.from.nodeId),
                    from_socket: connection.from.name,
                    to_node: nodeIdMap.get(connection.to.nodeId),
                    to_socket: connection.to.name
                })
            } else if (fromNode && toNode &&
                fromNode.type.startsWith('__baklava_SubgraphInputNode') &&
                !toNode.type.startsWith('__baklava_')) {
                // 情况1：SubgraphInput → 普通节点
                // 映射后创建连接
                const externalInput = inputs[fromNode.graphInterfaceId]
                if (externalInput) {
                    links.push({
                        from_node: nodeIdMap.get(externalInput.fromNodeId),
                        from_socket: externalInput.fromSocket,
                        to_node: nodeIdMap.get(connection.to.nodeId),
                        to_socket: connection.to.name
                    })
                }
            } else if (fromNode && toNode &&
                !fromNode.type.startsWith('__baklava_') &&
                toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                // 情况2：普通节点 → SubgraphOutput
                // 映射后创建连接
                const externalOutput = outputs[toNode.graphInterfaceId]
                if (externalOutput) {
                    links.push({
                        from_node: nodeIdMap.get(connection.from.nodeId),
                        from_socket: connection.from.name,
                        to_node: nodeIdMap.get(externalOutput.toNodeId),
                        to_socket: externalOutput.toSocket
                    })
                }
            } else if (fromNode && toNode &&
                fromNode.type.startsWith('__baklava_SubgraphInputNode') &&
                toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                // 情况3：SubgraphInput → SubgraphOutput
                // 两端映射后创建连接
                const externalInput = inputs[fromNode.graphInterfaceId]
                const externalOutput = outputs[toNode.graphInterfaceId]
                if (externalInput && externalOutput) {
                    links.push({
                        from_node: nodeIdMap.get(externalInput.fromNodeId),
                        from_socket: externalInput.fromSocket,
                        to_node: nodeIdMap.get(externalOutput.toNodeId),
                        to_socket: externalOutput.toSocket
                    })
                }
            }
        })
    }

    // 从主图开始递归处理
    processGraphRecursively(baklavaEditor.editor.graph)

    console.log(`序列化完成: ${nodes.length} 个节点, ${links.length} 个连接`)

    return {
        nodes,
        links,
        metadata: {
            version: "1.0",
            createdAt: new Date().toISOString(),
            nodeCount: nodes.length,
            connectionCount: links.length,
            serializationMethod: "recursive-subgraph-expansion"
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

    if (!nodeTree.links || !Array.isArray(nodeTree.links)) {
        errors.push('节点树缺少有效的连接数组')
    }

    if (errors.length > 0) {
        return { valid: false, errors, warnings }
    }

    // 收集所有节点ID（现在是整型）
    const nodeIds = new Set(nodeTree.nodes.map(node => node.id))

    // 验证连接的有效性
    nodeTree.links.forEach((link, index) => {
        if (!nodeIds.has(link.from_node)) {
            errors.push(`连接 ${index}: 源节点 ${link.from_node} 不存在`)
        }

        if (!nodeIds.has(link.to_node)) {
            errors.push(`连接 ${index}: 目标节点 ${link.to_node} 不存在`)
        }
    })

    // 检查孤立节点
    const connectedNodes = new Set()
    nodeTree.links.forEach(link => {
        connectedNodes.add(link.from_node)
        connectedNodes.add(link.to_node)
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
            connectionCount: nodeTree.links.length,
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
        if (node.input_values) {
            Object.entries(node.input_values).forEach(([key, value]) => {
                const valueType = typeof value
                inputValueCount[valueType] = (inputValueCount[valueType] || 0) + 1
            })
        }
    })

    return {
        totalNodes: nodeTree.nodes.length,
        totalConnections: nodeTree.links.length,
        nodeTypes: nodeTypeCount,
        inputValueTypes: inputValueCount,
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
    const connectionCount = nodeTree.links.length

    // 简单的复杂度计算：节点数 + 连接数 * 0.5
    return nodeCount + connectionCount * 0.5
}
