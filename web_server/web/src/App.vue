<template>
  <div class="app">
    <!-- 标题栏 -->
    <header class="header">
      <h1 class="title">RzNode - 节点编程系统</h1>
      <div class="toolbar">
        <button class="btn btn-primary" @click="logHello">测试按钮</button>
      </div>
    </header>
    
    <!-- 主编辑区域 -->
    <div class="editor-container">
      <BaklavaEditor :view-model="baklava" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { BaklavaEditor, useBaklava } from '@baklavajs/renderer-vue'
// 导入Baklava主题样式
import '@baklavajs/themes/dist/syrup-dark.css'

// 导入示例节点定义
// import AddNode from './nodes/AddNode.js'
// import NumberNode from './nodes/NumberNode.js'

// 初始化Baklava编辑器
const baklava = useBaklava()

// 组件挂载后注册节点类型
onMounted(() => {
  // 注册示例节点
  // baklava.editor.registerNodeType(AddNode)
  // baklava.editor.registerNodeType(NumberNode)
  
  console.log('BaklavaJS节点编辑器已初始化')
})

// 测试按钮点击事件
const logHello = () => {
  console.log('Hello, RzNode!')
}
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
