<template>
  <div class="app">
    <!-- 标题栏 -->
    <header class="header">
      <h1 class="title">RzNode - 节点编程系统</h1>
      <div class="toolbar">
        <button class="btn btn-primary" @click="testConnection">测试连接</button>
        <button class="btn btn-secondary" @click="loadNodeTypes" :disabled="!isConnected">加载节点类型</button>
        <span class="status" :class="{ 'connected': isConnected, 'disconnected': !isConnected }">
          {{ connectionStatus }}
        </span>
      </div>
    </header>
    
    <!-- 主编辑区域 -->
    <div class="editor-container">
      <BaklavaEditor :view-model="baklava" />
    </div>
    
    <!-- 调试信息面板 -->
    <div class="debug-panel" v-if="debugInfo.length > 0">
      <h3>调试信息</h3>
      <div class="debug-messages">
        <div v-for="(msg, index) in debugInfo" :key="index" class="debug-message">
          {{ msg }}
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { BaklavaEditor, useBaklava } from '@baklavajs/renderer-vue'
// 导入Baklava主题样式
import '@baklavajs/themes/dist/syrup-dark.css'
// 导入API客户端
import RzNodeAPI from './api/rznode-api.js'

// 初始化Baklava编辑器
const baklava = useBaklava()

// 状态管理
const isConnected = ref(false)
const connectionStatus = ref('未连接')
const debugInfo = ref([])
const nodeTypes = ref([])

// 初始化API客户端
const api = new RzNodeAPI()

// 添加调试信息
const addDebugInfo = (message) => {
  const timestamp = new Date().toLocaleTimeString()
  debugInfo.value.unshift(`[${timestamp}] ${message}`)
  // 限制调试信息数量
  if (debugInfo.value.length > 10) {
    debugInfo.value.pop()
  }
  console.log(message)
}

// 测试与后端连接
const testConnection = async () => {
  try {
    addDebugInfo('正在测试连接...')
    const status = await api.getStatus()
    isConnected.value = true
    connectionStatus.value = '已连接'
    addDebugInfo(`连接成功: ${JSON.stringify(status)}`)
  } catch (error) {
    isConnected.value = false
    connectionStatus.value = '连接失败'
    addDebugInfo(`连接失败: ${error.message}`)
  }
}

// 加载节点类型
const loadNodeTypes = async () => {
  try {
    addDebugInfo('正在加载节点类型...')
    const types = await api.getNodeTypes()
    nodeTypes.value = types
    addDebugInfo(`成功加载 ${types.length} 个节点类型`)
    console.log('节点类型详情:', types)
    
    // TODO: 将节点类型注册到Baklava编辑器
    // registerNodeTypesToBaklava(types)
  } catch (error) {
    addDebugInfo(`加载节点类型失败: ${error.message}`)
  }
}

// 组件挂载后的初始化
onMounted(() => {
  console.log('BaklavaJS节点编辑器已初始化')
  addDebugInfo('前端应用已启动')
  // 自动测试连接
  testConnection()
})
</script>

<style scoped>
.app {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: #1a1a1a;
}

.header {
  background-color: #2a2a2a;
  color: white;
  padding: 12px 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid #404040;
  z-index: 1000;
}

.title {
  margin: 0;
  font-size: 1.5rem;
  font-weight: 600;
}

.toolbar {
  display: flex;
  gap: 12px;
  align-items: center;
}

.btn {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.2s ease;
  font-size: 0.9rem;
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-primary {
  background: #3498db;
  color: white;
}

.btn-primary:hover:not(:disabled) {
  background: #2980b9;
  transform: translateY(-1px);
}

.btn-secondary {
  background: #95a5a6;
  color: white;
}

.btn-secondary:hover:not(:disabled) {
  background: #7f8c8d;
  transform: translateY(-1px);
}

.status {
  padding: 6px 12px;
  border-radius: 4px;
  font-weight: 500;
  font-size: 0.85rem;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.connected {
  background: #27ae60;
  color: white;
}

.disconnected {
  background: #e74c3c;
  color: white;
}

.editor-container {
  flex: 1;
  position: relative;
  overflow: hidden;
}

.debug-panel {
  background: #f8f9fa;
  border-top: 2px solid #dee2e6;
  padding: 1rem;
  max-height: 200px;
  overflow-y: auto;
}

.debug-panel h3 {
  margin: 0 0 0.5rem 0;
  font-size: 1rem;
  color: #495057;
}

.debug-messages {
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 0.85rem;
}

.debug-message {
  padding: 0.25rem 0;
  color: #6c757d;
  border-bottom: 1px solid #e9ecef;
}

.debug-message:last-child {
  border-bottom: none;
}

/* 覆盖Baklava的默认样式 */
:deep(.baklava-editor) {
  width: 100%;
  height: 100%;
  background-color: #1a1a1a;
}
</style>
  flex-direction: column;
  background-color: #1a1a1a;
}

.header {
  background-color: #2a2a2a;
  color: white;
  padding: 12px 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid #404040;
  z-index: 1000;
}

.title {
  margin: 0;
  font-size: 1.5rem;
  font-weight: 600;
}

.toolbar {
  display: flex;
  gap: 12px;
}

.btn {
  background-color: #404040;
  color: white;
  border: none;
  padding: 8px 16px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.2s;
}

.btn:hover {
  background-color: #505050;
}

.btn-primary {
  background-color: #007acc;
}

.btn-primary:hover {
  background-color: #005a9e;
}

.editor-container {
  flex: 1;
  width: 100%;
  height: calc(100vh - 80px);
  position: relative;
}

/* 覆盖Baklava的默认样式 */
:deep(.baklava-editor) {
  width: 100%;
  height: 100%;
  background-color: #1a1a1a;
}
</style>
