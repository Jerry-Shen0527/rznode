// RzNode API 客户端 - TypeScript版本

import type { SerializedNodeTree, ValidationResult } from '../utils/nodeTreeSerializer'
import type { NodeTypeData, NodeTypesApiResponse } from '../utils/nodeConverter'
import type { ValueTypesApiResponse } from '../utils/valueTypeRegistrant'
// 导入统一的调试工具
import { logTag } from '../utils/logFormatter'

// API响应类型定义
export interface ServerStatus {
    status: string
    message: string
    port: number
    has_node_system: boolean
    node_system_info?: string
}

export type ExecutionResponse =
    | {
        error: string
        success?: never
        error_message?: never
        execution_time?: never
    }
    | {
        error?: never
        success: boolean
        error_message: string
        execution_time: number
    }

export interface ApiRequestOptions {
    timeout?: number
    headers?: Record<string, string>
}

/**
 * RzNode API 客户端类
 */
export class RzNodeAPI {
    private baseURL: string
    private defaultTimeout: number

    constructor(baseURL: string = 'http://localhost:8080') {
        this.baseURL = baseURL
        this.defaultTimeout = 10000 // 10秒默认超时
    }

    /**
     * 获取服务器状态
     * @param options - 请求选项
     * @returns 服务器状态信息
     */
    async getStatus(options: ApiRequestOptions = {}): Promise<ServerStatus> {
        const response = await this.makeRequest('/api/status', 'GET', undefined, options)
        return response as ServerStatus
    }

    /**
     * 获取节点类型列表
     * @param options - 请求选项
     * @returns 节点类型数组或错误对象
     */
    async getNodeTypes(options: ApiRequestOptions = {}): Promise<NodeTypesApiResponse> {
        const response = await this.makeRequest('/api/node-types', 'GET', undefined, options)
        return response as NodeTypesApiResponse
    }

    /**
     * 获取值类型列表
     * @param options - 请求选项
     * @returns 值类型数组或错误对象
     */
    async getValueTypes(options: ApiRequestOptions = {}): Promise<ValueTypesApiResponse> {
        const response = await this.makeRequest('/api/value-types', 'GET', undefined, options)
        return response as ValueTypesApiResponse
    }

    /**
     * 执行节点树
     * @param nodeTree - 序列化的节点树
     * @param options - 请求选项
     * @returns 执行结果
     */
    async executeNodeTree(nodeTree: SerializedNodeTree, options: ApiRequestOptions = {}): Promise<ExecutionResponse> {
        const response = await this.makeRequest('/api/execute', 'POST', nodeTree, options)
        return response as ExecutionResponse
    }

    /**
     * 验证节点树（不执行）
     * @param nodeTree - 序列化的节点树
     * @param options - 请求选项
     * @returns 验证结果
     */
    async validateNodeTree(nodeTree: SerializedNodeTree, options: ApiRequestOptions = {}): Promise<ValidationResult> {
        const response = await this.makeRequest('/api/validate', 'POST', nodeTree, options)
        return response as ValidationResult
    }

    /**
     * 通用请求方法
     * @param endpoint - API端点
     * @param method - HTTP方法
     * @param body - 请求体数据
     * @param options - 请求选项
     * @returns 响应数据
     */
    private async makeRequest(
        endpoint: string,
        method: 'GET' | 'POST' | 'PUT' | 'DELETE' = 'GET',
        body?: any,
        options: ApiRequestOptions = {}
    ): Promise<any> {
        const url = `${this.baseURL}${endpoint}`
        const timeout = options.timeout || this.defaultTimeout

        // 创建AbortController用于超时控制
        const controller = new AbortController()
        const timeoutId = setTimeout(() => controller.abort(), timeout)

        try {
            const requestOptions: RequestInit = {
                method,
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                },
                signal: controller.signal
            }

            if (body && (method === 'POST' || method === 'PUT')) {
                requestOptions.body = JSON.stringify(body)
            }

            const response = await fetch(url, requestOptions)

            // 清除超时定时器
            clearTimeout(timeoutId)

            if (!response.ok) {
                const errorText = await response.text()
                throw new Error(`HTTP ${response.status}: ${errorText}`)
            }

            return await response.json()
        } catch (error) {
            clearTimeout(timeoutId)

            if (error instanceof Error) {
                if (error.name === 'AbortError') {
                    throw new Error(`请求超时 (${timeout}ms): ${endpoint}`)
                }
                throw new Error(`API请求失败: ${error.message}`)
            } else {
                throw new Error(`API请求失败: 未知错误`)
            }
        }
    }

    /**
     * 设置基础URL
     * @param baseURL - 新的基础URL
     */
    setBaseURL(baseURL: string): void {
        this.baseURL = baseURL
    }

    /**
     * 获取当前基础URL
     * @returns 当前基础URL
     */
    getBaseURL(): string {
        return this.baseURL
    }

    /**
     * 设置默认超时时间
     * @param timeout - 超时时间（毫秒）
     */
    setDefaultTimeout(timeout: number): void {
        this.defaultTimeout = timeout
    }

    /**
     * 获取当前默认超时时间
     * @returns 超时时间（毫秒）
     */
    getDefaultTimeout(): number {
        return this.defaultTimeout
    }

    /**
     * 测试API连接
     * @returns 连接是否成功
     */
    async testConnection(): Promise<boolean> {
        try {
            await this.getStatus({ timeout: 5000 })
            return true
        } catch (error) {
            console.warn(logTag('WARNING'), 'API连接测试失败:', error)
            return false
        }
    }
}

// 默认导出类以保持与原JS文件的兼容性
export default RzNodeAPI
