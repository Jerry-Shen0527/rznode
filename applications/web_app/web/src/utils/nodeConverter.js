// 节点类型转换器 - 将后端节点定义转换为Baklava节点类型
import { defineNode, NodeInterface, NumberInterface, IntegerInterface, TextInputInterface } from 'baklavajs'

/**
 * 将后端节点类型转换为Baklava节点定义
 * @param {Object} nodeTypeData - 后端返回的节点类型数据
 * @returns {Object} Baklava节点定义
 */
export function createBaklavaNodeDefinition(nodeTypeData) {
    const nodeDefinition = {
        type: nodeTypeData.id_name,
        title: nodeTypeData.ui_name || nodeTypeData.id_name,
        inputs: {},
        outputs: {},
        // 启用在BaklavaJS侧边栏中显示
        displayInSidebar: true,
        calculate: () => {
            // 基础计算逻辑，后续可以扩展
            const outputs = {}

            // 为每个输出设置默认值
            if (nodeTypeData.outputs) {
                nodeTypeData.outputs.forEach(output => {
                    outputs[output.identifier] = null
                })
            }

            return outputs
        }
    }

    // 添加输入接口
    if (nodeTypeData.inputs && nodeTypeData.inputs.length > 0) {
        nodeTypeData.inputs.forEach(input => {
            const defaultValue = parseFloat(input.default_value) || 0
            const minValue = input.min_value ? parseFloat(input.min_value) : undefined
            const maxValue = input.max_value ? parseFloat(input.max_value) : undefined
            const inputName = input.name || input.identifier

            if (input.type === 'int') {
                // 对于整数类型使用IntegerInterface
                if (minValue !== undefined && maxValue !== undefined) {
                    nodeDefinition.inputs[input.identifier] = () => new IntegerInterface(
                        inputName,
                        Math.round(defaultValue),
                        Math.round(minValue),
                        Math.round(maxValue)
                    )
                } else {
                    nodeDefinition.inputs[input.identifier] = () => new IntegerInterface(
                        inputName,
                        Math.round(defaultValue)
                    )
                }
            } else if (input.type === 'float' || input.type === 'double') {
                // 对于浮点数类型使用NumberInterface（BaklavaJS中float和double都用NumberInterface）
                if (minValue !== undefined && maxValue !== undefined) {
                    nodeDefinition.inputs[input.identifier] = () => new NumberInterface(
                        inputName,
                        defaultValue,
                        minValue,
                        maxValue
                    )
                } else {
                    nodeDefinition.inputs[input.identifier] = () => new NumberInterface(
                        inputName,
                        defaultValue
                    )
                }
            } else {
                // 对于其他类型使用TextInputInterface
                nodeDefinition.inputs[input.identifier] = () => new TextInputInterface(
                    inputName,
                    input.default_value || ''
                )
            }
        })
    }

    // 添加输出接口
    if (nodeTypeData.outputs && nodeTypeData.outputs.length > 0) {
        nodeTypeData.outputs.forEach(output => {
            const outputName = output.name || output.identifier

            if (output.type === 'int') {
                // 整数输出使用带有默认值0的IntegerInterface
                nodeDefinition.outputs[output.identifier] = () => new IntegerInterface(outputName, 0)
            } else if (output.type === 'float' || output.type === 'double') {
                // 浮点数输出使用带有默认值0.0的NumberInterface
                nodeDefinition.outputs[output.identifier] = () => new NumberInterface(outputName, 0.0)
            } else {
                // 其他类型使用基础NodeInterface
                nodeDefinition.outputs[output.identifier] = () => new NodeInterface(outputName)
            }
        })
    }

    return nodeDefinition
}

/**
 * 注册节点类型到Baklava编辑器
 * @param {Object} baklava - Baklava编辑器实例
 * @param {Array} nodeTypesData - 后端返回的节点类型数组
 */
export function registerNodeTypesToBaklava(baklava, nodeTypesData) {
    console.log('开始注册节点类型到Baklava编辑器...')

    let registeredCount = 0

    nodeTypesData.forEach(nodeTypeData => {
        try {
            const nodeDefinition = createBaklavaNodeDefinition(nodeTypeData)
            const NodeClass = defineNode(nodeDefinition)

            // 注册到编辑器 - 节点将自动显示在BaklavaJS侧边栏中
            baklava.editor.registerNodeType(NodeClass)

            registeredCount++
            console.log(`已注册节点类型: ${nodeTypeData.ui_name} (${nodeTypeData.id_name}) - 侧边栏显示: 启用`)
        } catch (error) {
            console.error(`注册节点类型失败: ${nodeTypeData.ui_name}`, error)
        }
    })

    console.log(`节点类型注册完成，共注册 ${registeredCount} 个节点，将显示在BaklavaJS侧边栏中`)
    return registeredCount
}

/**
 * 获取节点类型的友好显示信息
 * @param {Object} nodeTypeData - 节点类型数据
 * @returns {Object} 显示信息
 */
export function getNodeTypeDisplayInfo(nodeTypeData) {
    // 统计不同类型的输入
    const inputTypeCount = {}
    if (nodeTypeData.inputs) {
        nodeTypeData.inputs.forEach(input => {
            inputTypeCount[input.type] = (inputTypeCount[input.type] || 0) + 1
        })
    }

    // 统计不同类型的输出
    const outputTypeCount = {}
    if (nodeTypeData.outputs) {
        nodeTypeData.outputs.forEach(output => {
            outputTypeCount[output.type] = (outputTypeCount[output.type] || 0) + 1
        })
    }

    const typeDescription = []
    if (Object.keys(inputTypeCount).length > 0) {
        const inputDesc = Object.entries(inputTypeCount)
            .map(([type, count]) => `${count}×${type}`)
            .join(', ')
        typeDescription.push(`输入: ${inputDesc}`)
    }
    if (Object.keys(outputTypeCount).length > 0) {
        const outputDesc = Object.entries(outputTypeCount)
            .map(([type, count]) => `${count}×${type}`)
            .join(', ')
        typeDescription.push(`输出: ${outputDesc}`)
    }

    return {
        name: nodeTypeData.ui_name || nodeTypeData.id_name,
        id: nodeTypeData.id_name,
        inputCount: nodeTypeData.inputs?.length || 0,
        outputCount: nodeTypeData.outputs?.length || 0,
        hasGroups: nodeTypeData.groups?.length > 0,
        description: typeDescription.join('; ') || '无输入输出',
        supportedTypes: {
            inputs: Object.keys(inputTypeCount),
            outputs: Object.keys(outputTypeCount)
        }
    }
}