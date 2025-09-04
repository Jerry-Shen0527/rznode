<template>
    <div class="geometry-visualizer">
        <!-- 导航栏 -->
        <nav class="navbar">
            <div class="nav-left">
                <h1 class="title">几何可视化器</h1>
                <router-link to="/node-editor" class="nav-link">
                    <button class="btn btn-secondary">返回节点编辑器</button>
                </router-link>
            </div>

            <div class="nav-right">
                <div class="connection-status" :class="{ 'connected': geometryStore.isConnected }">
                    {{ geometryStore.connectionStatus }}
                </div>
                <button class="btn btn-primary" @click="toggleConnection" :disabled="isConnecting">
                    {{ isConnecting ? '连接中...' : (geometryStore.isConnected ? '断开连接' : '连接WebSocket') }}
                </button>
            </div>
        </nav>

        <!-- 工具栏 -->
        <div class="toolbar">
            <div class="toolbar-group">
                <span class="group-label">视图:</span>
                <button class="btn btn-sm" :class="{ 'active': geometryStore.viewMode === 'perspective' }"
                    @click="geometryStore.setViewMode('perspective')">
                    透视
                </button>
                <button class="btn btn-sm" :class="{ 'active': geometryStore.viewMode === 'orthographic' }"
                    @click="geometryStore.setViewMode('orthographic')">
                    正交
                </button>
            </div>

            <div class="toolbar-group">
                <span class="group-label">显示:</span>
                <button class="btn btn-sm" :class="{ 'active': geometryStore.showWireframe }" @click="toggleWireframe">
                    线框
                </button>
                <button class="btn btn-sm" :class="{ 'active': geometryStore.showNormals }"
                    @click="geometryStore.toggleNormals()">
                    法线
                </button>
            </div>

            <div class="toolbar-group">
                <span class="group-label">背景:</span>
                <input type="color" :value="geometryStore.backgroundColor" @input="onBackgroundColorChange"
                    class="color-picker">
            </div>

            <div class="toolbar-group">
                <button class="btn btn-sm" @click="resetView">重置视图</button>
                <button class="btn btn-sm" @click="clearSelection">清除选择</button>
            </div>

            <div class="toolbar-group">
                <span class="group-label">统计:</span>
                <span class="stats">
                    几何体: {{ geometryStore.geometryCount }} |
                    已选择: {{ geometryStore.selectedCount }}
                </span>
            </div>
        </div>

        <!-- 主容器 -->
        <div class="main-container">
            <!-- 3D视口 -->
            <div class="viewport-container">
                <div ref="viewportRef" class="viewport" @click="onViewportClick"></div>

                <!-- 视口覆盖层信息 -->
                <div class="viewport-overlay">
                    <div v-if="!geometryStore.hasGeometry" class="no-geometry-message">
                        <p>暂无几何数据</p>
                        <p class="hint">请在节点编辑器中执行包含几何输出的节点树</p>
                    </div>

                    <div v-if="geometryStore.isConnected && geometryStore.hasGeometry" class="viewport-info">
                        <div class="scene-info">
                            场景ID: {{ geometryStore.sceneId || '未知' }}
                        </div>
                    </div>
                </div>
            </div>

            <!-- 侧边栏 -->
            <div class="sidebar">
                <div class="panel">
                    <h3 class="panel-title">几何对象列表</h3>
                    <div class="geometry-list">
                        <div v-for="geometry in geometryStore.geometries" :key="geometry.id" class="geometry-item"
                            :class="{ 'selected': geometryStore.selectedGeometryIds.has(geometry.id) }"
                            @click="toggleGeometrySelection(geometry.id)">
                            <div class="geometry-info">
                                <span class="geometry-type">{{ geometry.type }}</span>
                                <span class="geometry-id">{{ geometry.id }}</span>
                            </div>
                            <div class="geometry-stats">
                                <span v-if="geometry.mesh_data">
                                    顶点: {{ Math.floor(geometry.mesh_data.vertices.length / 3) }}
                                    | 面: {{ geometry.mesh_data.face_vertex_counts.length }}
                                </span>
                                <!-- TODO: 补充更多信息 -->
                            </div>
                        </div>
                    </div>
                </div>

                <div class="panel">
                    <h3 class="panel-title">相机控制</h3>
                    <div class="camera-controls">
                        <div class="control-row">
                            <label>位置 X:</label>
                            <span>{{ geometryStore.cameraPosition.x.toFixed(2) }}</span>
                        </div>
                        <div class="control-row">
                            <label>位置 Y:</label>
                            <span>{{ geometryStore.cameraPosition.y.toFixed(2) }}</span>
                        </div>
                        <div class="control-row">
                            <label>位置 Z:</label>
                            <span>{{ geometryStore.cameraPosition.z.toFixed(2) }}</span>
                        </div>
                        <div class="control-row">
                            <label>目标 X:</label>
                            <span>{{ geometryStore.cameraTarget.x.toFixed(2) }}</span>
                        </div>
                        <div class="control-row">
                            <label>目标 Y:</label>
                            <span>{{ geometryStore.cameraTarget.y.toFixed(2) }}</span>
                        </div>
                        <div class="control-row">
                            <label>目标 Z:</label>
                            <span>{{ geometryStore.cameraTarget.z.toFixed(2) }}</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, nextTick, watch } from 'vue'
import { useGeometryStore } from '../stores/geometryStore'
import { ThreeJSRenderer } from '../utils/threeJSRenderer'
import { logTag } from '../utils/logFormatter'

// 状态管理
const geometryStore = useGeometryStore()

// 组件状态
const viewportRef = ref<HTMLElement>()
const isConnecting = ref(false)
const renderer = ref<ThreeJSRenderer>()

// 生命周期
onMounted(async () => {
    await nextTick()
    await initializeViewer()
})

onUnmounted(() => {
    cleanup()
})

// 初始化3D查看器
const initializeViewer = async () => {
    if (!viewportRef.value) {
        console.error(logTag('ERROR'), '视口容器未找到')
        return
    }

    try {
        // 初始化Three.js渲染器
        renderer.value = new ThreeJSRenderer(viewportRef.value)

        // 连接WebSocket
        await connectWebSocket()

        console.log(logTag('INFO'), '几何可视化器初始化完成')
    } catch (error) {
        console.error(logTag('ERROR'), '初始化几何可视化器失败:', error)
    }
}

// WebSocket连接管理
const connectWebSocket = async () => {
    isConnecting.value = true
    try {
        await geometryStore.connectWebSocket()
        console.log(logTag('INFO'), 'WebSocket连接成功')
    } catch (error) {
        console.error(logTag('ERROR'), 'WebSocket连接失败:', error)
    } finally {
        isConnecting.value = false
    }
}

const toggleConnection = async () => {
    if (geometryStore.isConnected) {
        geometryStore.disconnectWebSocket()
    } else {
        await connectWebSocket()
    }
}

// 视图控制
const toggleWireframe = () => {
    geometryStore.toggleWireframe()
    if (renderer.value) {
        renderer.value.setWireframe(geometryStore.showWireframe)
    }
}

const onBackgroundColorChange = (event: Event) => {
    const target = event.target as HTMLInputElement
    geometryStore.setBackgroundColor(target.value)
    if (renderer.value) {
        renderer.value.setBackgroundColor(target.value)
    }
}

const resetView = () => {
    geometryStore.resetView()
    if (renderer.value) {
        renderer.value.resetCamera()
    }
}

const clearSelection = () => {
    geometryStore.clearSelection()
    if (renderer.value) {
        renderer.value.clearSelection()
    }
}

// 几何对象交互
const toggleGeometrySelection = (id: string) => {
    geometryStore.toggleGeometrySelection(id)

    // 向后端发送选择状态
    const selectedIds = Array.from(geometryStore.selectedGeometryIds)
    geometryStore.sendGeometrySelection(selectedIds)

    // 更新3D场景选择状态
    if (renderer.value) {
        if (geometryStore.selectedGeometryIds.has(id)) {
            renderer.value.selectObject(id)
        } else {
            renderer.value.deselectObject(id)
        }
    }
}

const onViewportClick = (event: MouseEvent) => {
    // 处理3D场景中的点击事件
    // 可以实现射线投射来选择3D对象
    console.log(logTag('INFO'), '视口点击:', event)
}

// 清理资源
const cleanup = () => {
    geometryStore.disconnectWebSocket()
    if (renderer.value) {
        renderer.value.dispose()
    }
}

// 监听几何数据变化
watch(() => geometryStore.geometries, (newGeometries) => {
    if (renderer.value) {
        renderer.value.updateGeometries(newGeometries)
    }
}, { deep: true })

// 监听视图模式变化
watch(() => geometryStore.viewMode, (newMode) => {
    if (renderer.value) {
        renderer.value.setViewMode(newMode)
    }
})
</script>