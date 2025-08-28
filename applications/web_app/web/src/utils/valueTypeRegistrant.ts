// 接口值类型注册器 - 将后端的cpp值类型注册到前端，以便使得只有同类型的接口可以连接

import {
    NodeInterfaceType,
    type BaklavaInterfaceTypes
} from 'baklavajs'
import { useValueTypeStore } from '../stores/valueTypeStores'
// 导入统一的调试工具
import { logTag } from './logFormatter'

// 类型定义
export interface ValueTypeInfo {
    type_name: string // 类型名称，对应后端的type_name字段
}

export type ApiDataValueTypes = ValueTypeInfo[]

/**
 * 为值类型创建 BaklavaJS 接口类型
 * @param valueTypes - 值类型数组
 */
export function createBaklavaInterfaceTypesFromValueTypes(
    baklavaInterfaceTypes: BaklavaInterfaceTypes,
    valueTypes: ValueTypeInfo[],
    interfaceTypeMap: Map<string, NodeInterfaceType<any>>
): void {
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
 * 处理API响应并更新缓存，同时注册 BaklavaJS 类型
 * @param apiDataValueTypes - API响应数据
 * @param baklava - BaklavaJS 编辑器实例（可选，如果提供则会注册类型）
 * @returns 成功加载的值类型数量，失败则抛出异常
 */
export function handleApiDataValueTypes(
    baklavaInterfaceTypes: BaklavaInterfaceTypes,
    apiDataValueTypes: ApiDataValueTypes,
    interfaceTypeMap: Map<string, NodeInterfaceType<any>>
): number {
    // 确保是数组类型
    if (!Array.isArray(apiDataValueTypes)) {
        throw new Error('API响应格式错误：期望数组或错误对象')
    }


    try {
        // 创建并注册 BaklavaJS 接口类型
        createBaklavaInterfaceTypesFromValueTypes(baklavaInterfaceTypes, apiDataValueTypes, interfaceTypeMap)
    } catch (error) {
        console.error(logTag('ERROR'), '注册 BaklavaJS 接口类型失败:', error)
    }

    return apiDataValueTypes.length
}