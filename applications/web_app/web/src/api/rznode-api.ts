// RzNode API 客户端 - TypeScript版本

import type { SerializedNodeTree, ApiDataValidationResult } from '../utils/nodeTreeSerializer'
import type { ApiDataNodeTypes } from '../utils/nodeConverter'
import type { ApiDataValueTypes } from '../utils/valueTypeRegistrant'
// 导入统一的调试工具
import { logTag } from '../utils/logFormatter'

export interface ApiResponse<T> {
    code: number
    message: string
    data: T | null
}

// API响应类型定义
export interface ApiDataServerStatus {
    status: string
    message: string
    port: number
    has_node_system: boolean
}

export interface ApiDataExecutionResult {
    success: boolean
    error: string
    execution_time: number
}


export interface ApiRequestOptions {
    timeout?: number
    headers?: Record<string, string>
}

/**
 * 自定义API错误类
 */
export class ApiError extends Error {
    constructor(
        public code: number,
        public apiMessage: string,
        public endpoint: string,
        public originalError?: string
    ) {
        super(`API Error [${code}] at ${endpoint}: ${apiMessage}`)
        this.name = 'ApiError'
    }
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
    async getStatus(options: ApiRequestOptions = {}): Promise<ApiDataServerStatus> {
        const data = await this.makeRequest<ApiDataServerStatus>('/api/status', 'GET', undefined, options)
        return data as ApiDataServerStatus
    }

    /**
     * 获取节点类型列表
     * @param options - 请求选项
     * @returns 节点类型数组或错误对象
     */
    async getNodeTypes(options: ApiRequestOptions = {}): Promise<ApiDataNodeTypes> {
        const data = await this.makeRequest<ApiDataNodeTypes>('/api/node-types', 'GET', undefined, options)
        return data as ApiDataNodeTypes
    }

    /**
     * 获取值类型列表
     * @param options - 请求选项
     * @returns 值类型数组或错误对象
     */
    async getValueTypes(options: ApiRequestOptions = {}): Promise<ApiDataValueTypes> {
        const data = await this.makeRequest<ApiDataValueTypes>('/api/value-types', 'GET', undefined, options)
        return data as ApiDataValueTypes
    }

    /**
     * 执行节点树
     * @param nodeTree - 序列化的节点树
     * @param options - 请求选项
     * @returns 执行结果
     */
    async executeNodeTree(nodeTree: SerializedNodeTree, options: ApiRequestOptions = {}): Promise<ApiDataExecutionResult> {
        const data = await this.makeRequest<ApiDataExecutionResult>('/api/execute', 'POST', nodeTree, options)
        return data as ApiDataExecutionResult
    }

    /**
     * 验证节点树（不执行）
     * @param nodeTree - 序列化的节点树
     * @param options - 请求选项
     * @returns 验证结果
     */
    async validateNodeTree(nodeTree: SerializedNodeTree, options: ApiRequestOptions = {}): Promise<ApiDataValidationResult> {
        const data = await this.makeRequest<ApiDataValidationResult>('/api/validate', 'POST', nodeTree, options)
        return data as ApiDataValidationResult
    }



    /**
     * 通用请求方法
     * @param endpoint - API端点
     * @param method - HTTP方法
     * @param body - 请求体数据
     * @param options - 请求选项
     * @returns 响应数据
     */
    private async makeRequest<T>(
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

            // return await response.json()

            const apiResponse: ApiResponse<T> = await response.json()

            if (apiResponse.code !== 0) {
                throw new ApiError(apiResponse.code, apiResponse.message, endpoint)
            }

            return apiResponse.data as T    // 返回data字段（后端应保证成功时data不为null）

        } catch (error) {
            clearTimeout(timeoutId)

            if (error instanceof ApiError) {
                throw error // 保留业务错误
            }

            if (error instanceof Error) {
                if (error.name === 'AbortError') {
                    throw new ApiError(-1, `请求超时 (${timeout}ms)`, endpoint, error.message)
                }
                throw new ApiError(-2, `网络错误: ${error.message}`, endpoint, error.message)
            }

            throw new ApiError(-3, '未知错误', endpoint)
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
