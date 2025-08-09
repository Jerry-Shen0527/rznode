<template>
  <div class="app">
    <!-- 标题栏 -->
    <header class="header">
      <h1 class="title">RzNode - 节点编程系统</h1>
      <div class="toolbar">
        <button class="btn btn-primary" @click="testConnection">测试连接</button>
        <button class="btn btn-secondary" @click="loadNodeTypes" :disabled="!isConnected">加载节点类型</button>
        <button class="btn btn-success" @click="executeNodeTree" :disabled="!isConnected || isExecuting">
          {{ isExecuting ? '执行中...' : '执行节点树' }}
        </button>
        <button class="btn btn-info" @click="validateCurrentTree" :disabled="!isConnected">验证节点树</button>
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

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import { BaklavaEditor, useBaklava } from '@baklavajs/renderer-vue'
// 导入Baklava主题样式
import '@baklavajs/themes/dist/syrup-dark.css'
// 导入API客户端 - 使用TypeScript版本
import { RzNodeAPI } from './api/rznode-api.ts'
// 导入节点转换器 - 使用TypeScript版本
import { registerNodeTypesToBaklava, getNodeTypeDisplayInfo, type NodeTypeData } from './utils/nodeConverter.ts'
// 导入节点树序列化器 - 使用TypeScript版本
import { serializeNodeTree, validateNodeTree, getNodeTreeStats } from './utils/nodeTreeSerializer.ts'

// 初始化Baklava编辑器
const baklava = useBaklava()

// 状态管理
const isConnected = ref<boolean>(false)
const connectionStatus = ref<string>('未连接')
const debugInfo = ref<string[]>([])
const nodeTypes = ref<NodeTypeData[]>([])
const isExecuting = ref<boolean>(false)
const lastExecutionResult = ref<any>(null)

// 计算属性：节点类型显示列表
const nodeTypeDisplayList = computed(() => {
  return nodeTypes.value.map(nodeType => getNodeTypeDisplayInfo(nodeType))
})

// 初始化API客户端
const api = new RzNodeAPI()

// 添加调试信息
const addDebugInfo = (message: string) => {
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
    addDebugInfo(`连接失败: ${error instanceof Error ? error.message : String(error)}`)
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
    
    // 将节点类型注册到Baklava编辑器
    const registeredCount = registerNodeTypesToBaklava(baklava, types)
    addDebugInfo(`已注册 ${registeredCount} 个节点类型到编辑器`)
  } catch (error) {
    addDebugInfo(`加载节点类型失败: ${error instanceof Error ? error.message : String(error)}`)
  }
}

// 执行节点树
const executeNodeTree = async () => {
  try {
    isExecuting.value = true
    addDebugInfo('正在序列化节点树...')
    
    // 序列化当前节点树
    const serializedTree = serializeNodeTree(baklava)
    console.log('序列化的节点树:', serializedTree)
    addDebugInfo(`序列化元数据: ${JSON.stringify(serializedTree.metadata)}`)
    
    // 本地验证
    const validation = validateNodeTree(serializedTree)
    if (!validation.valid) {
      addDebugInfo(`节点树验证失败: ${validation.message}`)
      return
    } else{
      addDebugInfo(`节点树警告: ${validation.message}`)
    }
    
    // 显示统计信息
    const stats = getNodeTreeStats(serializedTree)
    addDebugInfo(`节点树统计: ${stats.totalNodes} 个节点, ${stats.totalConnections} 个连接`)
    
    // 发送到后端执行
    addDebugInfo('正在发送到后端执行...')
    const result = await api.executeNodeTree(serializedTree)
    
    lastExecutionResult.value = result
    addDebugInfo(`执行完成: ${JSON.stringify(result)}`)
    console.log('执行结果:', result)
    
  } catch (error) {
    addDebugInfo(`执行失败: ${error instanceof Error ? error.message : String(error)}`)
    console.error('执行错误:', error)
  } finally {
    isExecuting.value = false
  }
}

// 验证当前节点树
const validateCurrentTree = async () => {
  try {
    addDebugInfo('正在验证节点树...')
    
    // 序列化当前节点树
    const serializedTree = serializeNodeTree(baklava)
    
    // 本地验证
    const localValidation = validateNodeTree(serializedTree)
    if (!localValidation.valid) {
      addDebugInfo(`本地验证失败: ${localValidation.message}`)
    } else {
      addDebugInfo('本地验证通过')
    }
    
    if (localValidation.message) {
      addDebugInfo(`警告: ${localValidation.message}`)
    }
    
    // 发送到后端验证
    const result = await api.validateNodeTree(serializedTree)
    addDebugInfo(`后端验证结果: ${JSON.stringify(result)}`)
    
    // 显示统计信息
    const stats = getNodeTreeStats(serializedTree)
    addDebugInfo(`统计信息: ${stats.totalNodes} 节点, ${stats.totalConnections} 连接, 复杂度: ${stats.complexity}`)
    
  } catch (error) {
    addDebugInfo(`验证失败: ${error instanceof Error ? error.message : String(error)}`)
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

.btn-success {
  background: #27ae60;
  color: white;
}

.btn-success:hover:not(:disabled) {
  background: #229954;
  transform: translateY(-1px);
}

.btn-info {
  background: #3498db;
  color: white;
}

.btn-info:hover:not(:disabled) {
  background: #2980b9;
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
