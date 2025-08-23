// 节点树序列化器 - 将BaklavaJS节点树转换为后端可理解的格式

import type { IBaklavaViewModel } from '@baklavajs/renderer-vue'
import type { Graph, AbstractNode, Connection, NodeInterface, IGraphNode, GraphInputNode, GraphOutputNode } from '@baklavajs/core'
// 导入统一的调试工具
import { logTag } from './logFormatter'

// 类型定义
export interface SerializedNode {
    id: string
    type: string
    title: string
    input_values: Record<string, any>
}

export interface SerializedLink {
    id: string
    from_node: string
    from_socket: string
    to_node: string
    to_socket: string
}

export interface SerializedNodeTree {
    nodes: SerializedNode[]
    links: SerializedLink[]
    metadata: {
        version: string
        createdAt: string
        nodeCount: number
        connectionCount: number
        serializationMethod: string
    }
}

export interface ApiDataValidationResult {
    valid: boolean
    error: string
}


export interface NodeTreeStats {
    totalNodes: number
    totalConnections: number
    nodeTypes: Record<string, number>
    inputValueTypes: Record<string, number>
    complexity: number
}

// 内部映射类型
interface InputMapping {
    fromNodeId: string
    fromSocket: string
}

interface OutputMapping {
    toNodeId: string
    toSocket: string
}

// 声明子图节点类型
// From: @baklavajs/core/dist/graphNode.d.ts:13
type GraphNode = AbstractNode & IGraphNode

/**
 * 将BaklavaJS节点树序列化为后端格式（支持子图）
 * @param baklavaEditor - BaklavaJS编辑器实例
 * @returns 序列化的节点树
 */
export function serializeNodeTree(baklavaEditor: IBaklavaViewModel): SerializedNodeTree {
    const nodes: SerializedNode[] = []
    const links: SerializedLink[] = []

    /**
     * 递归处理图，支持子图展开
     * @param graph - 当前处理的图
     * @param inputs - 输入接口映射 {interfaceId: {fromNodeId, fromSocket}}
     * @param outputs - 输出接口映射 {interfaceId: {toNodeId, toSocket}}
     */
    function processGraphRecursively(
        graph: Graph,
        inputs: Record<string, InputMapping> = {},
        outputs: Record<string, OutputMapping> = {}
    ): void {
        // 处理当前图中的所有节点
        graph.nodes.forEach((node: AbstractNode) => {
            if (node.type.startsWith('__baklava_GraphNode')) {
                // 子图节点：需要递归处理其内部子图
                console.log(logTag('INFO'), `处理子图节点: ${node.id}`)

                // 建立子图的输入输出映射，组合当前层的映射
                const subgraphInputs: Record<string, InputMapping> = {}
                const subgraphOutputs: Record<string, OutputMapping> = {}

                // 遍历子图节点的输入接口，建立组合映射关系
                Object.entries(node.inputs).forEach(([inputKey, intf]: [string, NodeInterface]) => {
                    // 查找连接到这个接口的内部连接
                    graph.connections.forEach((conn: Connection) => {
                        if (conn.to.nodeId === node.id && conn.to.id === intf.id) {
                            const fromNode = graph.findNodeById(conn.from.nodeId)

                            if (fromNode && fromNode.type.startsWith('__baklava_SubgraphInputNode')) {
                                // 情况4：SubgraphInput → 子图节点
                                // 将当前层的inputs映射组合到子图层
                                // 使用 inputKey 作为映射键，因为 graphInterfaceId 对应的是子图节点的输入键名
                                const externalInput = inputs[inputKey]
                                if (externalInput) {
                                    subgraphInputs[inputKey] = externalInput
                                }
                            } else if (fromNode && !fromNode.type.startsWith('__baklava_')) {
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
                Object.entries(node.outputs).forEach(([outputKey, intf]: [string, NodeInterface]) => {
                    // 查找从这个接口连出的内部连接
                    graph.connections.forEach((conn: Connection) => {
                        if (conn.from.nodeId === node.id && conn.from.id === intf.id) {
                            const toNode = graph.findNodeById(conn.to.nodeId)

                            if (toNode && toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                                // 情况5：子图节点 → SubgraphOutput
                                // 将子图层的映射组合到当前层的outputs
                                // 使用 outputKey 作为映射键，因为 graphInterfaceId 对应的是子图节点的输出键名
                                const externalOutput = outputs[outputKey]
                                if (externalOutput) {
                                    subgraphOutputs[outputKey] = externalOutput
                                }
                            } else if (toNode && !toNode.type.startsWith('__baklava_')) {
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
                const subgraphNode = node as GraphNode
                if (subgraphNode.subgraph) {
                    processGraphRecursively(subgraphNode.subgraph, subgraphInputs, subgraphOutputs)
                }

            } else if (!node.type.startsWith('__baklava_')) {
                // 普通节点：直接序列化
                console.log(logTag('INFO'), `处理普通节点: ${node.id}, type: ${node.type}`)

                // 序列化节点
                const serializedNode: SerializedNode = {
                    id: node.id,
                    type: node.type,
                    title: node.title,
                    input_values: {}
                }

                // 序列化输入值
                Object.entries(node.inputs).forEach(([key, intf]: [string, NodeInterface]) => {
                    if (intf.value !== undefined) {
                        serializedNode.input_values[key] = intf.value
                    }
                })

                nodes.push(serializedNode)
            }
        })

        // 处理当前图中普通节点之间的连接
        graph.connections.forEach((connection: Connection) => {
            const fromNode = graph.findNodeById(connection.from.nodeId)
            const toNode = graph.findNodeById(connection.to.nodeId)

            // 只处理普通节点之间的连接
            if (fromNode && toNode &&
                !fromNode.type.startsWith('__baklava_') &&
                !toNode.type.startsWith('__baklava_')) {

                links.push({
                    id: connection.id,
                    from_node: connection.from.nodeId,
                    from_socket: connection.from.name,
                    to_node: connection.to.nodeId,
                    to_socket: connection.to.name
                })
            } else if (fromNode && toNode &&
                fromNode.type.startsWith('__baklava_SubgraphInputNode') &&
                !toNode.type.startsWith('__baklava_')) {
                // 情况1：SubgraphInput → 普通节点
                // 映射后创建连接
                const subgraphFromNode = fromNode as GraphInputNode
                const externalInput = inputs[subgraphFromNode.graphInterfaceId!]
                if (externalInput) {
                    links.push({
                        id: connection.id,
                        from_node: externalInput.fromNodeId,
                        from_socket: externalInput.fromSocket,
                        to_node: connection.to.nodeId,
                        to_socket: connection.to.name
                    })
                }
            } else if (fromNode && toNode &&
                !fromNode.type.startsWith('__baklava_') &&
                toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                // 情况2：普通节点 → SubgraphOutput
                // 映射后创建连接
                const subgraphToNode = toNode as GraphOutputNode
                const externalOutput = outputs[subgraphToNode.graphInterfaceId!]
                if (externalOutput) {
                    links.push({
                        id: connection.id,
                        from_node: connection.from.nodeId,
                        from_socket: connection.from.name,
                        to_node: externalOutput.toNodeId,
                        to_socket: externalOutput.toSocket
                    })
                }
            } else if (fromNode && toNode &&
                fromNode.type.startsWith('__baklava_SubgraphInputNode') &&
                toNode.type.startsWith('__baklava_SubgraphOutputNode')) {
                // 情况3：SubgraphInput → SubgraphOutput
                // 两端映射后创建连接
                const subgraphFromNode = fromNode as GraphInputNode
                const subgraphToNode = toNode as GraphOutputNode
                const externalInput = inputs[subgraphFromNode.graphInterfaceId!]
                const externalOutput = outputs[subgraphToNode.graphInterfaceId!]
                if (externalInput && externalOutput) {
                    links.push({
                        id: connection.id,
                        from_node: externalInput.fromNodeId,
                        from_socket: externalInput.fromSocket,
                        to_node: externalOutput.toNodeId,
                        to_socket: externalOutput.toSocket
                    })
                }
            }
        })
    }

    // 从主图开始递归处理
    processGraphRecursively(baklavaEditor.editor.graph)

    console.log(logTag('INFO'), `序列化完成: ${nodes.length} 个节点, ${links.length} 个连接`)

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
 * @param nodeTree - 序列化的节点树
 * @returns 验证结果
 */
export function validateNodeTree(nodeTree: SerializedNodeTree): ApiDataValidationResult {
    const errors: string[] = []
    const warnings: string[] = []

    // 检查基本结构
    if (!nodeTree.nodes || !Array.isArray(nodeTree.nodes)) {
        errors.push('节点树缺少有效的节点数组')
    }

    if (!nodeTree.links || !Array.isArray(nodeTree.links)) {
        errors.push('节点树缺少有效的连接数组')
    }

    if (errors.length > 0) {
        return {
            valid: false,
            error: `结构错误: ${errors.join('; ')}`,
        }
    }

    // 收集所有节点ID（现在是整型）
    const nodeIds = new Set(nodeTree.nodes.map((node: SerializedNode) => node.id))

    // 验证连接的有效性
    nodeTree.links.forEach((link: SerializedLink, index: number) => {
        if (!nodeIds.has(link.from_node)) {
            errors.push(`连接 ${index}: 源节点 ${link.from_node} 不存在`)
        }

        if (!nodeIds.has(link.to_node)) {
            errors.push(`连接 ${index}: 目标节点 ${link.to_node} 不存在`)
        }
    })

    // 检查孤立节点
    const connectedNodes = new Set<string>()
    nodeTree.links.forEach((link: SerializedLink) => {
        connectedNodes.add(link.from_node)
        connectedNodes.add(link.to_node)
    })

    nodeTree.nodes.forEach((node: SerializedNode) => {
        if (!connectedNodes.has(node.id) && nodeTree.nodes.length > 1) {
            warnings.push(`节点 ${node.title} (${node.id}) 没有连接`)
        }
    })

    const messages: string[] = []
    if (errors.length > 0) {
        messages.push(`错误: ${errors.join('; ')}`)
    }
    if (warnings.length > 0) {
        messages.push(`警告: ${warnings.join('; ')}`)
    }

    return {
        valid: errors.length === 0,
        error: messages.join(' | ')
    }
}

/**
 * 获取节点树的统计信息
 * @param nodeTree - 序列化的节点树
 * @returns 统计信息
 */
export function getNodeTreeStats(nodeTree: SerializedNodeTree): NodeTreeStats {
    const nodeTypeCount: Record<string, number> = {}
    const inputValueCount: Record<string, number> = {}

    nodeTree.nodes.forEach((node: SerializedNode) => {
        // 统计节点类型
        nodeTypeCount[node.type] = (nodeTypeCount[node.type] || 0) + 1

        // 统计输入值数量
        if (node.input_values) {
            Object.entries(node.input_values).forEach(([key, value]: [string, any]) => {
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
 * @param nodeTree - 序列化的节点树
 * @returns 复杂度分数
 */
function calculateComplexity(nodeTree: SerializedNodeTree): number {
    const nodeCount = nodeTree.nodes.length
    const connectionCount = nodeTree.links.length

    // 简单的复杂度计算：节点数 + 连接数 * 0.5
    return nodeCount + connectionCount * 0.5
}

/**
 * 安全验证节点树，提供统一的错误处理
 * @param nodeTree - 序列化的节点树
 * @returns 验证结果或错误信息
 */
export function safeValidateNodeTree(nodeTree: SerializedNodeTree): ApiDataValidationResult {
    try {
        return validateNodeTree(nodeTree)
    } catch (error) {
        return {
            valid: false,
            error: `验证过程中发生错误: ${error instanceof Error ? error.message : String(error)}`
        }
    }
}

/**
 * 检查验证结果是否为错误
 * @param result - 验证结果
 * @returns 是否为错误
 */
export function isValidationError(result: ApiDataValidationResult): boolean {
    return result.error !== undefined && result.error.length > 0
}

/**
 * 安全获取节点树统计信息
 * @param nodeTree - 序列化的节点树
 * @returns 统计信息，如果出错则返回默认值
 */
export function safeGetNodeTreeStats(nodeTree: SerializedNodeTree): NodeTreeStats {
    try {
        return getNodeTreeStats(nodeTree)
    } catch (error) {
        console.warn(logTag('WARNING'), '获取节点树统计信息失败:', error)
        return {
            totalNodes: 0,
            totalConnections: 0,
            nodeTypes: {},
            inputValueTypes: {},
            complexity: 0
        }
    }
}
