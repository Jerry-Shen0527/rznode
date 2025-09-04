// WebSocket client for geometry visualization
// 使用JSON格式传输几何数据，便于早期开发和调试

import { logTag } from "../utils/logFormatter"

export interface MeshData {
    vertices: number[]
    face_vertex_counts: number[]
    face_vertex_indices: number[]
    normals: number[]
    colors: number[]
    uvs: number[]
}

export interface PointsData {
    vertices: number[]
    normals: number[]
    colors: number[]
    widths: number[]
}

export interface CurveData {
    vertices: number[]
    vert_counts: number[]
    normals: number[]
    colors: number[]
    widths: number[]
    periodic: boolean
}

export interface GeometryData {
    id: string
    type: 'mesh' | 'points' | 'curve'
    transform: number[]
    mesh_data?: MeshData
    points_data?: PointsData
    curve_data?: CurveData
}

export interface GeometryMessage {
    type: 'geometry_update' | 'geometry_clear'
    geometries: GeometryData[]
    scene_id: string
    timestamp: number
}

export interface VisualizerMessage {
    type: 'user_interaction' | 'geometry_selection' | 'camera_change'
    data: any
    target_node?: string
}

export class GeometryWebSocketClient {
    private ws: WebSocket | null = null
    private url: string
    private reconnectAttempts = 0
    private maxReconnectAttempts = 5
    private reconnectInterval = 1000
    private messageHandlers: Map<string, (data: any) => void> = new Map()

    constructor(url: string = 'ws://localhost:8080/geometry/ws') {
        this.url = url
    }

    connect(): Promise<void> {
        return new Promise((resolve, reject) => {
            try {
                this.ws = new WebSocket(this.url)

                this.ws.onopen = () => {
                    console.log(logTag('INFO'), '几何可视化WebSocket连接已建立')
                    this.reconnectAttempts = 0
                    resolve()
                }

                this.ws.onmessage = (event) => {
                    try {
                        // 使用JSON格式处理消息
                        const message: GeometryMessage = JSON.parse(event.data)
                        this.handleMessage(message)
                    } catch (error) {
                        console.error(logTag('ERROR'), '解析WebSocket消息失败:', error)
                    }
                }

                this.ws.onclose = () => {
                    console.log(logTag('INFO'), '几何可视化WebSocket连接已关闭')
                    this.handleReconnect()
                }

                this.ws.onerror = (error) => {
                    console.error(logTag('ERROR'), '几何可视化WebSocket错误:', error)
                    reject(error)
                }
            } catch (error) {
                reject(error)
            }
        })
    }

    disconnect(): void {
        if (this.ws) {
            this.ws.close()
            this.ws = null
        }
    }

    private handleMessage(message: GeometryMessage): void {
        const handler = this.messageHandlers.get(message.type)
        if (handler) {
            handler(message)
        }
    }

    private handleReconnect(): void {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++
            console.log(logTag('INFO'), `尝试重连WebSocket (${this.reconnectAttempts}/${this.maxReconnectAttempts})`)

            setTimeout(() => {
                this.connect().catch(() => {
                    console.error(logTag('ERROR'), 'WebSocket重连失败')
                })
            }, this.reconnectInterval * this.reconnectAttempts)
        }
    }

    onMessage(type: string, handler: (data: any) => void): void {
        this.messageHandlers.set(type, handler)
    }

    sendMessage(message: VisualizerMessage): void {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(message))
        } else {
            console.warn(logTag('WARNING'), 'WebSocket未连接，无法发送消息')
        }
    }

    get isConnected(): boolean {
        return this.ws !== null && this.ws.readyState === WebSocket.OPEN
    }
}