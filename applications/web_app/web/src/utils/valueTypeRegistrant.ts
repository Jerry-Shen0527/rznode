// 接口值类型注册器 - 将后端的cpp值类型注册到前端，以便使得只有同类型的接口可以连接

import { NodeInterfaceType } from '@baklavajs/interface-types'
import { BaklavaInterfaceTypes } from '@baklavajs/interface-types'
import type { IBaklavaViewModel } from '@baklavajs/renderer-vue'
// 导入统一的调试工具
import { logTag } from './logFormatter'

// 类型定义
export interface ValueTypeInfo {
    type_name: string // 类型名称，对应后端的type_name字段
}

export interface ValueTypeError {
    error: string
}

export type ValueTypesApiResponse = ValueTypeInfo[] | ValueTypeError

// 简单缓存 - 和 nodeTypes 类似
let cachedValueTypes: ValueTypeInfo[] = []

// BaklavaJS 类型管理
let baklavaInterfaceTypes: BaklavaInterfaceTypes | null = null
let interfaceTypeMap: Map<string, NodeInterfaceType<any>> = new Map()

/**
 * 初始化 BaklavaJS 接口类型系统
 * @param baklava - BaklavaJS 编辑器实例
 */
export function initializeBaklavaInterfaceTypes(baklava: IBaklavaViewModel): void {
    if (baklavaInterfaceTypes) {
        console.warn(logTag('WARNING'), 'BaklavaJS 接口类型系统已经初始化过了')
        return
    }

    baklavaInterfaceTypes = new BaklavaInterfaceTypes(baklava.editor)
    console.log(logTag('INFO'), 'BaklavaJS 接口类型系统已初始化')
}

/**
 * 为值类型创建 BaklavaJS 接口类型
 * @param valueTypes - 值类型数组
 */
export function createBaklavaInterfaceTypesFromValueTypes(valueTypes: ValueTypeInfo[]): void {
    if (!baklavaInterfaceTypes) {
        throw new Error('必须先调用 initializeBaklavaInterfaceTypes 初始化类型系统')
    }

    // 清空现有的类型映射
    interfaceTypeMap.clear()

    // 为每个值类型创建 NodeInterfaceType
    const typesToAdd: NodeInterfaceType<any>[] = []

    valueTypes.forEach(valueType => {
        const typeName = valueType.type_name

        // 创建 NodeInterfaceType 实例
        const interfaceType = new NodeInterfaceType(typeName)

        // 存储映射关系
        interfaceTypeMap.set(typeName, interfaceType)
        typesToAdd.push(interfaceType)

        console.log(logTag('INFO'), `创建接口类型: ${typeName}`)
    })

    // 批量注册所有类型到 BaklavaJS
    baklavaInterfaceTypes.addTypes(...typesToAdd)

    console.log(logTag('INFO'), `已注册 ${typesToAdd.length} 个接口类型到 BaklavaJS`)
}

/**
 * 根据类型名称获取 BaklavaJS 接口类型
 * @param typeName - 类型名称
 * @returns BaklavaJS 接口类型实例或 null
 */
export function getBaklavaInterfaceType(typeName: string): NodeInterfaceType<any> | null {
    return interfaceTypeMap.get(typeName) || null
}

/**
 * 检查API响应是否为错误
 * @param response - API响应
 * @returns 是否为错误响应
 */
export function isValueTypeError(response: ValueTypesApiResponse): response is ValueTypeError {
    return !Array.isArray(response) && 'error' in response && typeof response.error === 'string'
}

/**
 * 处理API响应并更新缓存，同时注册 BaklavaJS 类型
 * @param apiResponse - API响应数据
 * @param baklava - BaklavaJS 编辑器实例（可选，如果提供则会注册类型）
 * @returns 成功加载的值类型数量，失败则抛出异常
 */
export function handleValueTypesApiResponse(
    apiResponse: ValueTypesApiResponse,
    baklava?: IBaklavaViewModel
): number {
    // 检查是否是错误响应
    if (isValueTypeError(apiResponse)) {
        throw new Error(`API错误: ${apiResponse.error}`)
    }

    // 确保是数组类型
    if (!Array.isArray(apiResponse)) {
        throw new Error('API响应格式错误：期望数组或错误对象')
    }

    // 更新缓存
    cachedValueTypes = apiResponse

    // 如果提供了 BaklavaJS 实例，则注册类型
    if (baklava) {
        try {
            // 初始化类型系统（如果还没初始化）
            if (!baklavaInterfaceTypes) {
                initializeBaklavaInterfaceTypes(baklava)
            }

            // 创建并注册 BaklavaJS 接口类型
            createBaklavaInterfaceTypesFromValueTypes(apiResponse)
        } catch (error) {
            console.error(logTag('ERROR'), '注册 BaklavaJS 接口类型失败:', error)
            // 不抛出异常，因为缓存更新成功了
        }
    }

    return apiResponse.length
}

/**
 * 获取缓存的值类型
 * @param typeName - 类型名称（可选）
 * @returns 指定类型或所有类型
 */
export function getCachedValueTypes(typeName?: string): ValueTypeInfo[] | ValueTypeInfo | null {
    if (typeName) {
        const found = cachedValueTypes.find(type => type.type_name === typeName)
        return found || null
    }

    return cachedValueTypes
}