// 节点类型转换器 - 将后端节点定义转换为Baklava节点类型

import {
    defineNode, NodeInterface, setType,
    NumberInterface, IntegerInterface, CheckboxInterface, TextInputInterface, TextInterface,
    type IBaklavaViewModel, type NodeInterfaceType
} from 'baklavajs'
// 导入统一的调试工具
import { logTag } from './logFormatter'

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

// 单个节点类型的数据结构（对应后端的 NodeTypeDto）
export interface NodeTypeData {
    id_name: string
    ui_name: string
    color: [number, number, number, number] // RGBA数组
    inputs: NodeSocketInfo[]
    outputs: NodeSocketInfo[]
    groups: any[]
}

// API 响应类型：节点类型列表
export interface ApiDataNodeTypes {
    node_types: NodeTypeData[]
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
        console.warn(logTag('WARNING'), `解析后端值失败: "${valueStr}" (类型: ${dataType})`, error)
        return undefined
    }
}

/**
 * 将后端节点类型转换为Baklava节点类
 * @param nodeTypeData - 后端返回的节点类型数据
 * @returns 创建的Baklava节点定义类
 */
export function createBaklavaNodeClass(
    nodeTypeData: NodeTypeData,
    interfaceTypeMap: Map<string, NodeInterfaceType<any>>
): any | null {
    // 检查数据完整性
    if (!nodeTypeData.id_name || !nodeTypeData.ui_name) {
        console.error(logTag('ERROR'), '节点类型数据不完整:', nodeTypeData)
        return null
    }

    const nodeDefinition = {
        type: nodeTypeData.id_name,
        title: nodeTypeData.ui_name,
        inputs: {} as Record<string, () => NodeInterface>,
        outputs: {} as Record<string, () => NodeInterface>,
        displayInSidebar: true
    }

    // 添加输入接口
    if (nodeTypeData.inputs && nodeTypeData.inputs.length > 0) {
        nodeTypeData.inputs.forEach((input: NodeSocketInfo) => {
            // 使用新的解析函数来处理C++后端发送的字符串值
            const defaultValue = parseBackendValue(input.default_value, input.type)
            const minValue = parseBackendValue(input.min_value, input.type)
            const maxValue = parseBackendValue(input.max_value, input.type)

            const inputName = input.name || input.identifier

            // 获取 BaklavaJS 接口类型
            const baklavaType = interfaceTypeMap.get(input.type)

            if (baklavaType) {
                console.log(logTag('INFO'), `为输入接口 ${input.identifier} 应用类型约束: ${input.type}`)
            } else {
                console.warn(logTag('WARNING'), `未找到输入接口 ${input.identifier} 的类型约束: ${input.type}`)
            }

            if (input.type === 'int') {
                // 对于整数类型使用IntegerInterface
                if (minValue !== undefined && maxValue !== undefined) {
                    const intInterface = () => {
                        const intf = new IntegerInterface(
                            inputName,
                            Math.round(defaultValue || 0),
                            Math.round(minValue),
                            Math.round(maxValue)
                        )
                        // 如果找到了对应的类型，则应用类型约束
                        if (baklavaType) {
                            intf.use(setType, baklavaType)
                        }
                        intf
                        return intf
                    }
                    nodeDefinition.inputs[input.identifier] = intInterface
                } else {
                    const intInterface = () => {
                        const intf = new IntegerInterface(
                            inputName,
                            Math.round(defaultValue || 0)
                        )
                        if (baklavaType) {
                            intf.use(setType, baklavaType)
                        }
                        return intf
                    }
                    nodeDefinition.inputs[input.identifier] = intInterface
                }
            } else if (input.type === 'float' || input.type === 'double') {
                // 对于浮点数类型使用NumberInterface（BaklavaJS中float和double都用NumberInterface）
                if (minValue !== undefined && maxValue !== undefined) {
                    const numInterface = () => {
                        const intf = new NumberInterface(
                            inputName,
                            defaultValue || 0.0,
                            minValue,
                            maxValue
                        )
                        if (baklavaType) {
                            intf.use(setType, baklavaType)
                        }
                        return intf
                    }
                    nodeDefinition.inputs[input.identifier] = numInterface
                } else {
                    const numInterface = () => {
                        const intf = new NumberInterface(
                            inputName,
                            defaultValue || 0.0
                        )
                        if (baklavaType) {
                            intf.use(setType, baklavaType)
                        }
                        return intf
                    }
                    nodeDefinition.inputs[input.identifier] = numInterface
                }
            } else if (input.type === 'bool') {
                // 对于布尔类型使用CheckboxInterface
                const boolInterface = () => {
                    const intf = new CheckboxInterface(
                        inputName,
                        defaultValue || false
                    )
                    if (baklavaType) {
                        intf.use(setType, baklavaType)
                    }
                    return intf
                }
                nodeDefinition.inputs[input.identifier] = boolInterface
            } else if (input.type === 'string') {
                // 对于字符串类型使用TextInputInterface
                const stringInterface = () => {
                    const intf = new TextInputInterface(
                        inputName,
                        defaultValue || ''
                    )
                    if (baklavaType) {
                        intf.use(setType, baklavaType)
                    }
                    return intf
                }
                nodeDefinition.inputs[input.identifier] = stringInterface
            } else {
                // 对于其他类型（包括未知类型）使用TextInterface
                const genericInterface = () => {
                    const intf = new NodeInterface(
                        inputName,
                        undefined
                    )
                    if (baklavaType) {
                        intf.use(setType, baklavaType)
                    }
                    return intf
                }
                nodeDefinition.inputs[input.identifier] = genericInterface
            }
        })
    }

    // 添加输出接口
    if (nodeTypeData.outputs && nodeTypeData.outputs.length > 0) {
        nodeTypeData.outputs.forEach((output: NodeSocketInfo) => {
            const outputName = output.name || output.identifier

            // 获取 BaklavaJS 接口类型
            const baklavaType = interfaceTypeMap.get(output.type)

            if (baklavaType) {
                console.log(logTag('INFO'), `为输出接口 ${output.identifier} 应用类型约束: ${output.type}`)
            } else {
                console.warn(logTag('WARNING'), `未找到输出接口 ${output.identifier} 的类型约束: ${output.type}`)
            }

            // 所有输出接口都使用基础NodeInterface，因为输出值不应由用户编辑
            // 输出值由节点的calculate()函数内部逻辑产生
            const outputInterface = () => {
                const intf = new NodeInterface(outputName, undefined)
                // 如果找到了对应的类型，则应用类型约束
                if (baklavaType) {
                    intf.use(setType, baklavaType)
                }
                return intf
            }
            nodeDefinition.outputs[output.identifier] = outputInterface
        })
    }

    return defineNode(nodeDefinition)
}

/**
 * 注册节点类型到Baklava编辑器
 * @param baklava - Baklava编辑器实例
 * @param nodeTypesData - 后端返回的节点类型数组
 */
export function registerNodeTypesToBaklava(
    baklava: IBaklavaViewModel,
    nodeTypesData: NodeTypeData[],
    interfaceTypeMap: Map<string, NodeInterfaceType<any>>
): number {
    console.log(logTag('INFO'), '开始注册节点类型到Baklava编辑器...')

    let registeredCount = 0

    for (const nodeTypeData of nodeTypesData) {
        try {
            const nodeClass = createBaklavaNodeClass(nodeTypeData, interfaceTypeMap)

            // 检查节点类创建是否成功
            if (nodeClass === null) {
                console.warn(logTag('WARNING'), `跳过有问题的节点类型:`, nodeTypeData)
                continue
            }

            baklava.editor.registerNodeType(nodeClass)

            registeredCount++
            console.log(logTag('INFO'), `已注册节点类型: ${nodeTypeData.ui_name} (${nodeTypeData.id_name})`)
        } catch (error) {
            console.error(logTag('ERROR'), `注册节点类型失败: ${nodeTypeData.ui_name}`, error)
        }
    }

    console.log(logTag('INFO'), `节点类型注册完成，共注册 ${registeredCount} 个节点`)
    return registeredCount
}

/**
 * 处理 API 响应并注册节点类型
 * @param baklava - Baklava编辑器实例
 * @param apiDataNodeTypes - 节点类型 API 响应数据
 * @param apiDataValueTypes - 值类型 API 响应数据
 * @returns 注册的节点数量，如果出错则抛出异常
 */
export function handleApiDataNodeTypes(
    baklava: IBaklavaViewModel,
    apiDataNodeTypes: ApiDataNodeTypes,
    interfaceTypeMap: Map<string, NodeInterfaceType<any>>
): number {
    // 然后处理节点类型
    return registerNodeTypesToBaklava(baklava, apiDataNodeTypes.node_types, interfaceTypeMap)
}
