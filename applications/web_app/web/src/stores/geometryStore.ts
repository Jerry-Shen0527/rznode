import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { GeometryMessage, GeometryWebSocketClient, type GeometryData } from '../api/geometry-websocket'
import { logTag } from '../utils/logFormatter'

export const useGeometryStore = defineStore('geometry', () => {
    // WebSocket连接状态
    const wsClient = ref<GeometryWebSocketClient>(new GeometryWebSocketClient())

    // 几何数据
    const geometries = ref<GeometryData[]>([])
    const selectedGeometryIds = ref<Set<string>>(new Set())
    const sceneId = ref('default')

    // 相机和视图状态
    const cameraPosition = ref({ x: 0, y: 0, z: 10 })
    const cameraTarget = ref({ x: 0, y: 0, z: 0 })
    const viewMode = ref<'perspective' | 'orthographic'>('perspective')

    // 显示选项
    const showWireframe = ref(false)
    const showNormals = ref(false)
    const backgroundColor = ref('#2a2a2a')

    // 计算属性  
    const isConnected = computed(() => wsClient.value.connectionState === 'connected')
    const geometryCount = computed(() => geometries.value.length)
    const selectedCount = computed(() => selectedGeometryIds.value.size)
    const hasGeometry = computed(() => geometries.value.length > 0)

    // WebSocket连接方法
    const connectWebSocket = async (url?: string) => {
        if (wsClient.value.connectionState === 'connected') {
            console.warn(logTag('WARNING'), 'WebSocket已连接，无需重复连接')
            return
        }

        try {
            if (url) {
                wsClient.value.disconnect()
                wsClient.value = new GeometryWebSocketClient(url)
            }

            // 设置消息处理器
            wsClient.value.onMessage('geometry_update', handleGeometryUpdate)
            wsClient.value.onMessage('geometry_clear', handleGeometryClear)

            await wsClient.value.connect()
        } catch (error) {
            console.error(logTag('ERROR'), 'WebSocket连接失败:', error)
            throw error
        }
    }

    const disconnectWebSocket = () => {
        if (wsClient.value) {
            wsClient.value.disconnect()
        }
    }

    // 几何数据处理方法
    const handleGeometryUpdate = (message: GeometryMessage) => {
        if (message.scene_id !== sceneId.value) {
            console.debug(
                logTag('DEBUG'), `收到的几何数据场景ID (${message.scene_id}) 与当前场景ID (${sceneId.value}) 不匹配，已忽略该消息。`
            )
            return
        }

        // 合并新数据
        const newGeometries = message.geometries || []
        const existingIds = new Set(geometries.value.map(g => g.id))
        newGeometries.forEach(geom => {
            if (!existingIds.has(geom.id)) {
                geometries.value.push(geom)
            } else {
                // 更新已有几何体
                const index = geometries.value.findIndex(g => g.id === geom.id)
                if (index !== -1) {
                    geometries.value[index] = geom
                }
            }
        })
        console.log(logTag('INFO'), `几何数据已更新: ${geometries.value.length} 个对象`)
    }

    const handleGeometryClear = (message: GeometryMessage) => {
        if (message.scene_id !== sceneId.value) {
            console.warn(
                logTag('DEBUG'), `收到的几何数据场景ID (${message.scene_id}) 与当前场景ID (${sceneId.value}) 不匹配，已忽略该消息。`
            )
            return
        }

        geometries.value = []
        selectedGeometryIds.value.clear()
        console.log(logTag('INFO'), '几何数据已清空')
    }

    // 几何选择方法
    const selectGeometry = (id: string) => {
        selectedGeometryIds.value.add(id)
    }

    const deselectGeometry = (id: string) => {
        selectedGeometryIds.value.delete(id)
    }

    const clearSelection = () => {
        selectedGeometryIds.value.clear()
    }

    const toggleGeometrySelection = (id: string) => {
        if (selectedGeometryIds.value.has(id)) {
            deselectGeometry(id)
        } else {
            selectGeometry(id)
        }
    }

    // 用户交互方法
    const sendUserInteraction = (type: string, data: any) => {
        if (wsClient.value) {
            wsClient.value.sendMessage({
                type: 'user_interaction',
                data: { interactionType: type, ...data }
            })
        }
    }

    const sendGeometrySelection = (selectedIds: string[]) => {
        if (wsClient.value) {
            wsClient.value.sendMessage({
                type: 'geometry_selection',
                data: { selectedIds }
            })
        }
    }

    const sendCameraChange = (position: any, target: any) => {
        cameraPosition.value = position
        cameraTarget.value = target

        if (wsClient.value) {
            wsClient.value.sendMessage({
                type: 'camera_change',
                data: { position, target }
            })
        }
    }

    // 视图控制方法
    const setViewMode = (mode: 'perspective' | 'orthographic') => {
        viewMode.value = mode
    }

    const toggleWireframe = () => {
        showWireframe.value = !showWireframe.value
    }

    const toggleNormals = () => {
        showNormals.value = !showNormals.value
    }

    const setBackgroundColor = (color: string) => {
        backgroundColor.value = color
    }

    // 重置方法
    const resetCamera = () => {
        cameraPosition.value = { x: 0, y: 0, z: 10 }
        cameraTarget.value = { x: 0, y: 0, z: 0 }
    }

    const resetView = () => {
        resetCamera()
        clearSelection()
        showWireframe.value = false
        showNormals.value = false
        viewMode.value = 'perspective'
    }

    return {
        // 状态
        geometries,
        selectedGeometryIds,
        sceneId,
        cameraPosition,
        cameraTarget,
        viewMode,
        showWireframe,
        showNormals,
        backgroundColor,

        // 计算属性
        isConnected,
        geometryCount,
        selectedCount,
        hasGeometry,

        // WebSocket方法
        connectWebSocket,
        disconnectWebSocket,

        // 几何数据方法
        selectGeometry,
        deselectGeometry,
        clearSelection,
        toggleGeometrySelection,

        // 用户交互方法
        sendUserInteraction,
        sendGeometrySelection,
        sendCameraChange,

        // 视图控制方法
        setViewMode,
        toggleWireframe,
        toggleNormals,
        setBackgroundColor,
        resetCamera,
        resetView
    }
})
