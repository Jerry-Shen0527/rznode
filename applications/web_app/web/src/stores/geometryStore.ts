import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { GeometryWebSocketClient, type GeometryData } from '../api/geometry-websocket'

export const useGeometryStore = defineStore('geometry', () => {
    // WebSocket连接状态
    const wsClient = ref<GeometryWebSocketClient | null>(null)
    const isConnected = ref(false)
    const connectionStatus = ref('未连接')

    // 几何数据
    const geometries = ref<GeometryData[]>([])
    const selectedGeometryIds = ref<Set<string>>(new Set())
    const sceneId = ref('')

    // 相机和视图状态
    const cameraPosition = ref({ x: 0, y: 0, z: 10 })
    const cameraTarget = ref({ x: 0, y: 0, z: 0 })
    const viewMode = ref<'perspective' | 'orthographic'>('perspective')

    // 显示选项
    const showWireframe = ref(false)
    const showNormals = ref(false)
    const backgroundColor = ref('#2a2a2a')

    // 计算属性
    const geometryCount = computed(() => geometries.value.length)
    const selectedCount = computed(() => selectedGeometryIds.value.size)
    const hasGeometry = computed(() => geometries.value.length > 0)

    // WebSocket连接方法
    const connectWebSocket = async (url?: string) => {
        try {
            wsClient.value = new GeometryWebSocketClient(url)

            // 设置消息处理器
            wsClient.value.onMessage('geometry_update', handleGeometryUpdate)
            wsClient.value.onMessage('geometry_clear', handleGeometryClear)
            wsClient.value.onMessage('scene_update', handleSceneUpdate)

            await wsClient.value.connect()
            isConnected.value = true
            connectionStatus.value = '已连接'
        } catch (error) {
            console.error('WebSocket连接失败:', error)
            isConnected.value = false
            connectionStatus.value = '连接失败'
            throw error
        }
    }

    const disconnectWebSocket = () => {
        if (wsClient.value) {
            wsClient.value.disconnect()
            wsClient.value = null
        }
        isConnected.value = false
        connectionStatus.value = '未连接'
    }

    // 几何数据处理方法
    const handleGeometryUpdate = (message: any) => {
        geometries.value = message.geometries || []
        sceneId.value = message.scene_id || ''
        console.log(`几何数据已更新: ${geometries.value.length} 个对象`)
    }

    const handleGeometryClear = () => {
        geometries.value = []
        selectedGeometryIds.value.clear()
        console.log('几何数据已清空')
    }

    const handleSceneUpdate = (message: any) => {
        // 处理场景更新
        sceneId.value = message.scene_id || ''
        if (message.geometries) {
            geometries.value = message.geometries
        }
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
        if (wsClient.value && isConnected.value) {
            wsClient.value.sendMessage({
                type: 'user_interaction',
                data: { interactionType: type, ...data }
            })
        }
    }

    const sendGeometrySelection = (selectedIds: string[]) => {
        if (wsClient.value && isConnected.value) {
            wsClient.value.sendMessage({
                type: 'geometry_selection',
                data: { selectedIds }
            })
        }
    }

    const sendCameraChange = (position: any, target: any) => {
        cameraPosition.value = position
        cameraTarget.value = target

        if (wsClient.value && isConnected.value) {
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
        isConnected,
        connectionStatus,
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
