<template>
  <div class="app">
    <!-- 标题栏 -->
    <header class="header">
      <h1 class="title">RzNode - 节点编程系统</h1>
      <div class="toolbar">
        <button class="btn btn-primary" @click="testConnection">测试连接</button>
        <button class="btn btn-secondary" @click="loadNodeTypes" :disabled="!globalStore.isConnected">加载节点和值类型</button>
        <button class="btn btn-success" @click="executeNodeTree"
          :disabled="!globalStore.isConnected || globalStore.isExecuting">
          {{ globalStore.isExecuting ? '执行中...' : '执行节点树' }}
        </button>
        <button class="btn btn-info" @click="validateCurrentTree" :disabled="!globalStore.isConnected">验证节点树</button>
        <span class="status"
          :class="{ 'connected': globalStore.isConnected, 'disconnected': !globalStore.isConnected }">
          {{ globalStore.connectionStatus }}
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
import { BaklavaEditor, useBaklava, DependencyEngine, BaklavaInterfaceTypes } from 'baklavajs'
// 导入Baklava主题样式
import '@baklavajs/themes/dist/syrup-dark.css'
// 导入API客户端
import { RzNodeAPI } from './api/rznode-api.ts'
// 导入节点转换器
import { handleApiDataNodeTypes, type NodeTypeData } from './utils/nodeConverter.ts'
// 导入值类型注册器
import { handleApiDataValueTypes, type ValueTypeInfo } from './utils/valueTypeRegistrant.ts'
// 导入节点树序列化器
import {
  serializeNodeTree,
  safeValidateNodeTree,
  safeGetNodeTreeStats,
  isValidationError
} from './utils/nodeTreeSerializer.ts'
// 导入状态管理
import { useGlobalStore } from './stores/globalStores.ts'
import { useValueTypeStore } from './stores/valueTypeStores.ts'
// 导入统一的调试工具
import { logTag } from './utils/logFormatter.ts'

// 初始化Baklava编辑器
const baklava = useBaklava()
// 初始化引擎，因为Baklava.js中有一些特性必须在初始化引擎后才能使用
const baklavaEngine = new DependencyEngine(baklava.editor);
// 初始化接口类型管理器
const baklavaInterfaceTypes = new BaklavaInterfaceTypes(baklava.editor)
// 初始化状态管理
const globalStore = useGlobalStore()
const valueTypeStore = useValueTypeStore()

// 状态管理
// const isConnected = ref<boolean>(false)
// const connectionStatus = ref<string>('未连接')
// const isExecuting = ref<boolean>(false)
// const lastExecutionResult = ref<any>(null)

// 初始化API客户端
const api = new RzNodeAPI()

// 测试与后端连接
const testConnection = async () => {
  try {
    console.log(logTag('INFO'), '正在测试连接...')
    const status = await api.getStatus()
    globalStore.isConnected = true
    globalStore.connectionStatus = '已连接'
    console.log(logTag('INFO'), '连接成功:', status)
  } catch (error) {
    globalStore.isConnected = false
    globalStore.connectionStatus = '连接失败'
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

    // 先注册值类型
    const registeredValueTypeCount = handleApiDataValueTypes(baklavaInterfaceTypes, valueTypesResponse, valueTypeStore.interfaceTypeMap)
    console.log(logTag('INFO'), `已注册 ${registeredValueTypeCount} 个值类型到接口类型系统`)

    // 使用辅助函数处理API响应并注册节点类型（同时处理值类型）
    const registeredNodeTypeCount = handleApiDataNodeTypes(baklava, nodeTypesResponse, valueTypeStore.interfaceTypeMap)

    console.log(logTag('INFO'), `已注册 ${registeredNodeTypeCount} 个节点类型到编辑器`)
  } catch (error) {
    console.error(logTag('ERROR'), '加载类型失败:', error)
  }
}

// 执行节点树
const executeNodeTree = async () => {
  try {
    globalStore.isExecuting = true
    console.log(logTag('INFO'), '正在序列化节点树...')

    // 序列化当前节点树
    const serializedTree = serializeNodeTree(baklava)
    console.log(logTag('INFO'), '序列化的节点树:', serializedTree)

    // 本地验证
    const validation = safeValidateNodeTree(serializedTree)
    if (isValidationError(validation)) {
      console.error(logTag('ERROR'), '验证过程失败:', validation.error)
      return
    }

    if (!validation.valid) {
      console.warn(logTag('ERROR'), '节点树验证失败:', validation.error)
      return
    } else {
      console.log(logTag('INFO'), '节点树验证通过')
    }

    // 显示统计信息
    const stats = safeGetNodeTreeStats(serializedTree)
    console.log(logTag('INFO'), `节点树统计: ${stats.totalNodes} 个节点, ${stats.totalConnections} 个连接`)

    // 发送到后端执行
    console.log(logTag('INFO'), '正在发送到后端执行...')
    const result = await api.executeNodeTree(serializedTree)

    globalStore.lastExecutionResult = result
    console.log(logTag('INFO'), '执行完成:', result)

  } catch (error) {
    console.error(logTag('ERROR'), '执行失败:', error)
  } finally {
    globalStore.isExecuting = false
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
      console.warn(logTag('WARNING'), '本地验证失败:', localValidation.error)
    } else {
      console.log(logTag('INFO'), '本地验证通过')
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