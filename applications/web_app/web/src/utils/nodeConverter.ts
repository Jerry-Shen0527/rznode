// 节点类型转换器 - 将后端节点定义转换为Baklava节点类型

import { defineNode, NodeInterface, } from '@baklavajs/core'
import { NumberInterface, IntegerInterface, CheckboxInterface, TextInputInterface, TextInterface } from '@baklavajs/renderer-vue'
import { BaklavaEditor, type IBaklavaViewModel } from '@baklavajs/renderer-vue'

// 类型定义
export interface NodeSocketInfo {
    name: string
    identifier: string
    type: string
    optional?: boolean
    default_value?: string  // C++后端发送的是字符串格式的值
    min_value?: string      // C++后端发送的是字符串格式的值
    max_value?: string      // C++后端发送的是字符串格式的值
}

// C++后端可能返回两种情况：
// 1. 错误情况：只有 error 字段
// 2. 成功情况：有完整的节点类型信息
export type NodeTypeData =
    | {
        // 错误情况
        error: string
        id_name?: never
        ui_name?: never
        color?: never
        inputs?: never
        outputs?: never
        groups?: never
    }
    | {
        // 成功情况
        error?: never
        id_name: string
        ui_name: string
        color: [number, number, number, number] // RGBA数组
        inputs: NodeSocketInfo[]
        outputs: NodeSocketInfo[]
        groups: any[]
    }

// export interface BaklavaNodeDefinition {
//     type: string
//     title: string
//     inputs: Record<string, () => any>
//     outputs: Record<string, () => any>
//     displayInSidebar: boolean
//     // calculate: () => Record<string, any>
// }

export interface NodeTypeDisplayInfo {
    name: string
    id: string
    inputCount: number
    outputCount: number
    hasGroups: boolean
    description: string
    supportedTypes: {
        inputs: string[]
        outputs: string[]
    }
}

/**
 * 解析C++后端发送的字符串值
 * @param valueStr - C++后端发送的字符串值
 * @param dataType - 数据类型 ('int', 'float', 'double', 'bool', 'string')
 * @returns 解析后的JavaScript值
 */
function parseBackendValue(valueStr: string | undefined, dataType: string): any {
    if (!valueStr || valueStr.trim() === '') {
        return undefined
    }

    try {
        switch (dataType) {
            case 'int':
                const intValue = parseInt(valueStr, 10)
                return isNaN(intValue) ? undefined : intValue

            case 'float':
            case 'double':
                const floatValue = parseFloat(valueStr)
                return isNaN(floatValue) ? undefined : floatValue

            case 'bool':
                if (valueStr.toLowerCase() === 'true') return true
                if (valueStr.toLowerCase() === 'false') return false
                return undefined

            case 'string':
                // C++发送的字符串值是带引号的JSON格式，如 "\"hello\""
                // 需要先解析JSON，然后取出字符串值
                try {
                    return JSON.parse(valueStr)
                } catch {
                    // 如果JSON解析失败，直接返回原字符串（去掉首尾引号）
                    return valueStr.replace(/^"(.*)"$/, '$1')
                }

            default:
                // 对于未知类型，尝试JSON解析
                try {
                    return JSON.parse(valueStr)
                } catch {
                    return valueStr
                }
        }
    } catch (error) {
        console.warn(`解析后端值失败: "${valueStr}" (类型: ${dataType})`, error)
        return undefined
    }
}

/**
 * 将后端节点类型转换为Baklava节点类
 * @param nodeTypeData - 后端返回的节点类型数据
 * @returns 创建的Baklava节点定义类，如果有错误则返回null
 */
export function createBaklavaNodeClass(nodeTypeData: NodeTypeData) {
    // 检查是否是错误情况
    if ('error' in nodeTypeData && nodeTypeData.error) {
        console.error('节点类型包含错误:', nodeTypeData.error)
        return null
    }

    // 类型守卫：确保是成功情况的数据
    if (!nodeTypeData.id_name || !nodeTypeData.ui_name) {
        console.error('节点类型数据不完整:', nodeTypeData)
        return null
    }

    const nodeDefinition = {
        type: nodeTypeData.id_name,
        title: nodeTypeData.ui_name,
        inputs: {} as Record<string, () => NodeInterface>,
        outputs: {} as Record<string, () => NodeInterface>,
        // 启用在BaklavaJS侧边栏中显示
        displayInSidebar: true,
        // 无计算逻辑（C++后端处理输出值）
        // calculate: () => {
        //     // 基础计算逻辑，后续可以扩展
        //     const outputs: Record<string, any> = {}
        //     // 为每个输出设置默认值
        //     if (nodeTypeData.outputs) {
        //         nodeTypeData.outputs.forEach((output: NodeSocketInfo) => {
        //             outputs[output.identifier] = null
        //         })
        //     }
        //     return outputs
        // }
    }

    // 添加输入接口
    if (nodeTypeData.inputs && nodeTypeData.inputs.length > 0) {
        nodeTypeData.inputs.forEach((input: NodeSocketInfo) => {
            // 使用新的解析函数来处理C++后端发送的字符串值
            const defaultValue = parseBackendValue(input.default_value, input.type)
            const minValue = parseBackendValue(input.min_value, input.type)
            const maxValue = parseBackendValue(input.max_value, input.type)

            const inputName = input.name || input.identifier

            if (input.type === 'int') {
                // 对于整数类型使用IntegerInterface
                if (minValue !== undefined && maxValue !== undefined) {
                    nodeDefinition.inputs[input.identifier] = () => new IntegerInterface(
                        inputName,
                        Math.round(defaultValue || 0),
                        Math.round(minValue),
                        Math.round(maxValue)

                    )
                } else {
                    nodeDefinition.inputs[input.identifier] = () => new IntegerInterface(
                        inputName,
                        Math.round(defaultValue || 0)
                    )
                }
            } else if (input.type === 'float' || input.type === 'double') {
                // 对于浮点数类型使用NumberInterface（BaklavaJS中float和double都用NumberInterface）
                if (minValue !== undefined && maxValue !== undefined) {
                    nodeDefinition.inputs[input.identifier] = () => new NumberInterface(
                        inputName,
                        defaultValue || 0.0,
                        minValue,
                        maxValue
                    )
                } else {
                    nodeDefinition.inputs[input.identifier] = () => new NumberInterface(
                        inputName,
                        defaultValue || 0.0
                    )
                }
            } else if (input.type === 'bool') {
                // 对于布尔类型使用CheckboxInterface
                nodeDefinition.inputs[input.identifier] = () => new CheckboxInterface(
                    inputName,
                    defaultValue || false
                )
            } else if (input.type === 'string') {
                // 对于字符串类型使用TextInputInterface
                nodeDefinition.inputs[input.identifier] = () => new TextInputInterface(
                    inputName,
                    defaultValue || ''
                )
            } else {
                // 对于其他类型（包括未知类型）使用TextInterface
                nodeDefinition.inputs[input.identifier] = () => new TextInterface(
                    inputName,
                    defaultValue || ''
                )
            }
        })
    }

    // 添加输出接口
    if (nodeTypeData.outputs && nodeTypeData.outputs.length > 0) {
        nodeTypeData.outputs.forEach((output: NodeSocketInfo) => {
            const outputName = output.name || output.identifier

            // 所有输出接口都使用基础NodeInterface，因为输出值不应由用户编辑
            // 输出值由节点的calculate()函数内部逻辑产生
            nodeDefinition.outputs[output.identifier] = () => new NodeInterface(outputName, undefined)
        })
    }

    return defineNode(nodeDefinition)
}

/**
 * 注册节点类型到Baklava编辑器
 * @param baklava - Baklava编辑器实例
 * @param nodeTypesData - 后端返回的节点类型数组
 */
export function registerNodeTypesToBaklava(baklava: IBaklavaViewModel, nodeTypesData: NodeTypeData[]): number {
    console.log('开始注册节点类型到Baklava编辑器...')

    let registeredCount = 0

    for (const nodeTypeData of nodeTypesData) {
        try {
            const nodeClass = createBaklavaNodeClass(nodeTypeData)

            // 检查节点类创建是否成功
            if (nodeClass === null) {
                console.warn(`跳过有问题的节点类型:`, nodeTypeData)
                continue
            }

            baklava.editor.registerNodeType(nodeClass)

            registeredCount++
            console.log(`已注册节点类型: ${nodeTypeData.ui_name} (${nodeTypeData.id_name})`)
        } catch (error) {
            console.error(`注册节点类型失败: ${nodeTypeData.ui_name}`, error)
        }
    }

    console.log(`节点类型注册完成，共注册 ${registeredCount} 个节点`)
    return registeredCount
}

/**
 * 获取节点类型的友好显示信息
 * @param nodeTypeData - 节点类型数据
 * @returns 显示信息，如果数据有误返回错误信息
 */
export function getNodeTypeDisplayInfo(nodeTypeData: NodeTypeData): NodeTypeDisplayInfo {
    // 检查是否是错误情况
    if ('error' in nodeTypeData && nodeTypeData.error) {
        return {
            name: 'Error',
            id: 'error',
            inputCount: 0,
            outputCount: 0,
            hasGroups: false,
            description: `错误: ${nodeTypeData.error}`,
            supportedTypes: {
                inputs: [],
                outputs: []
            }
        }
    }

    // 类型守卫：确保是成功情况的数据
    if (!nodeTypeData.id_name || !nodeTypeData.ui_name) {
        return {
            name: 'Invalid Data',
            id: 'invalid',
            inputCount: 0,
            outputCount: 0,
            hasGroups: false,
            description: '数据不完整',
            supportedTypes: {
                inputs: [],
                outputs: []
            }
        }
    }

    // 统计不同类型的输入
    const inputTypeCount: Record<string, number> = {}
    if (nodeTypeData.inputs) {
        nodeTypeData.inputs.forEach((input: NodeSocketInfo) => {
            inputTypeCount[input.type] = (inputTypeCount[input.type] || 0) + 1
        })
    }

    // 统计不同类型的输出
    const outputTypeCount: Record<string, number> = {}
    if (nodeTypeData.outputs) {
        nodeTypeData.outputs.forEach((output: NodeSocketInfo) => {
            outputTypeCount[output.type] = (outputTypeCount[output.type] || 0) + 1
        })
    }

    const typeDescription: string[] = []
    if (Object.keys(inputTypeCount).length > 0) {
        const inputDesc = Object.entries(inputTypeCount)
            .map(([type, count]: [string, number]) => `${count}×${type}`)
            .join(', ')
        typeDescription.push(`输入: ${inputDesc}`)
    }
    if (Object.keys(outputTypeCount).length > 0) {
        const outputDesc = Object.entries(outputTypeCount)
            .map(([type, count]: [string, number]) => `${count}×${type}`)
            .join(', ')
        typeDescription.push(`输出: ${outputDesc}`)
    }

    return {
        name: nodeTypeData.ui_name,
        id: nodeTypeData.id_name,
        inputCount: nodeTypeData.inputs.length,
        outputCount: nodeTypeData.outputs.length,
        hasGroups: nodeTypeData.groups.length > 0,
        description: typeDescription.join('; ') || '无输入输出',
        supportedTypes: {
            inputs: Object.keys(inputTypeCount),
            outputs: Object.keys(outputTypeCount)
        }
    }
}
