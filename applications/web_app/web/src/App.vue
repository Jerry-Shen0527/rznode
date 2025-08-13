<template>
  <div class="app">
    <!-- 标题栏 -->
    <header class="header">
      <h1 class="title">RzNode - 节点编程系统</h1>
      <div class="toolbar">
        <button class="btn btn-primary" @click="testConnection">测试连接</button>
        <button class="btn btn-secondary" @click="loadNodeTypes" :disabled="!isConnected">加载节点和值类型</button>
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
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { BaklavaEditor, useBaklava } from '@baklavajs/renderer-vue'
import { DependencyEngine } from '@baklavajs/engine'
// 导入Baklava主题样式
import '@baklavajs/themes/dist/syrup-dark.css'
// 导入API客户端 - 使用TypeScript版本
import { RzNodeAPI } from './api/rznode-api.ts'
// 导入节点转换器 - 使用TypeScript版本
import { handleNodeTypesApiResponse, type NodeTypeData } from './utils/nodeConverter.ts'
// 导入值类型注册器 - 使用TypeScript版本
import { 
    getCachedValueTypes,
    type ValueTypeInfo 
} from './utils/valueTypeRegistrant.ts'
// 导入节点树序列化器 - 使用TypeScript版本
import { 
    serializeNodeTree, 
    safeValidateNodeTree, 
    getValidationDisplayText, 
    safeGetNodeTreeStats,
    isValidationError 
} from './utils/nodeTreeSerializer.ts'
// 导入统一的调试工具
import { logTag } from './utils/logFormatter.ts'

// 初始化Baklava编辑器
const baklava = useBaklava()
// 初始化引擎，因为Baklava.js中有一些特性必须在初始化引擎后才能使用
const engine = new DependencyEngine(baklava.editor); 

// 状态管理
const isConnected = ref<boolean>(false)
const connectionStatus = ref<string>('未连接')
const nodeTypes = ref<NodeTypeData[]>([])
const valueTypes = ref<ValueTypeInfo[]>([])
const isExecuting = ref<boolean>(false)
const lastExecutionResult = ref<any>(null)

// 初始化API客户端
const api = new RzNodeAPI()

// 测试与后端连接
const testConnection = async () => {
  try {
    console.log(logTag('INFO'), '正在测试连接...')
    const status = await api.getStatus()
    isConnected.value = true
    connectionStatus.value = '已连接'
    console.log(logTag('INFO'), '连接成功:', status)
  } catch (error) {
    isConnected.value = false
    connectionStatus.value = '连接失败'
    console.error(logTag('ERROR'), '连接失败:', error)
  }
}

// 加载节点类型
const loadNodeTypes = async () => {
  try {
    console.log(logTag('INFO'), '正在加载节点类型和值类型...')
    
    // 同时获取节点类型和值类型
    const [nodeTypesResponse, valueTypesResponse] = await Promise.all([
      api.getNodeTypes(),
      api.getValueTypes()
    ])
    
    // 使用辅助函数处理API响应并注册节点类型（同时处理值类型）
    const registeredCount = handleNodeTypesApiResponse(baklava, nodeTypesResponse, valueTypesResponse)
    
    // 如果成功注册，则更新本地状态
    if (Array.isArray(nodeTypesResponse)) {
      nodeTypes.value = nodeTypesResponse
      console.log(logTag('INFO'), `成功加载 ${nodeTypesResponse.length} 个节点类型`)
      console.log(logTag('INFO'), '节点类型详情:', nodeTypesResponse)
    }
    
    // 更新值类型状态
    const cachedValueTypes = getCachedValueTypes()
    if (Array.isArray(cachedValueTypes)) {
      valueTypes.value = cachedValueTypes
      console.log(logTag('INFO'), `成功加载 ${cachedValueTypes.length} 个值类型`)
      console.log(logTag('INFO'), '值类型详情:', cachedValueTypes)
    }
    
    console.log(logTag('INFO'), `已注册 ${registeredCount} 个节点类型到编辑器`)
  } catch (error) {
    console.error(logTag('ERROR'), '加载类型失败:', error)
  }
}

// 执行节点树
const executeNodeTree = async () => {
  try {
    isExecuting.value = true
    console.log(logTag('INFO'), '正在序列化节点树...')
    
    // 序列化当前节点树
    const serializedTree = serializeNodeTree(baklava)
    console.log(logTag('INFO'), '序列化的节点树:', serializedTree)
    console.log(logTag('INFO'), '序列化元数据:', serializedTree.metadata)
    
    // 本地验证
    const validation = safeValidateNodeTree(serializedTree)
    if (isValidationError(validation)) {
      console.error(logTag('ERROR'), '验证过程失败:', validation.error)
      return
    }
    
    if (!validation.valid) {
      console.warn(logTag('WARNING'), '节点树验证失败:', getValidationDisplayText(validation))
      return
    } else {
      console.log(logTag('INFO'), '节点树验证通过:', getValidationDisplayText(validation))
    }
    
    // 显示统计信息
    const stats = safeGetNodeTreeStats(serializedTree)
    console.log(logTag('INFO'), `节点树统计: ${stats.totalNodes} 个节点, ${stats.totalConnections} 个连接`)
    
    // 发送到后端执行
    console.log(logTag('INFO'), '正在发送到后端执行...')
    const result = await api.executeNodeTree(serializedTree)
    
    lastExecutionResult.value = result
    console.log(logTag('INFO'), '执行完成:', result)
    
  } catch (error) {
    console.error(logTag('ERROR'), '执行失败:', error)
  } finally {
    isExecuting.value = false
  }
}

// 验证当前节点树
const validateCurrentTree = async () => {
  try {
    console.log(logTag('INFO'), '正在验证节点树...')
    
    // 序列化当前节点树
    const serializedTree = serializeNodeTree(baklava)
    
    // 本地验证
    const localValidation = safeValidateNodeTree(serializedTree)
    if (isValidationError(localValidation)) {
      console.error(logTag('ERROR'), '本地验证过程失败:', localValidation.error)
    } else if (!localValidation.valid) {
      console.warn(logTag('WARNING'), '本地验证失败:', getValidationDisplayText(localValidation))
    } else {
      console.log(logTag('INFO'), '本地验证通过:', getValidationDisplayText(localValidation))
    }
    
    // 发送到后端验证
    const result = await api.validateNodeTree(serializedTree)
    console.log(logTag('INFO'), '后端验证结果:', result)
    
    // 显示统计信息
    const stats = safeGetNodeTreeStats(serializedTree)
    console.log(logTag('INFO'), `统计信息: ${stats.totalNodes} 节点, ${stats.totalConnections} 连接, 复杂度: ${stats.complexity}`)
    
  } catch (error) {
    console.error(logTag('ERROR'), '验证失败:', error)
  }
}

// 组件挂载后的初始化
onMounted(() => {
  console.log(logTag('INFO'), 'BaklavaJS节点编辑器已初始化')
  console.log(logTag('INFO'), '前端应用已启动')
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

/* 覆盖Baklava的默认样式 */
:deep(.baklava-editor) {
  width: 100%;
  height: 100%;
  background-color: #1a1a1a;
}
</style>
