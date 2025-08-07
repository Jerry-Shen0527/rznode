# RzNode Web Server API Documentation

## 概述

本文档描述了RzNode节点编程系统的前后端通信API。该API允许前端（基于Baklava.js的Vue应用）与C++内核进行通信，实现节点类型同步和节点树执行。

## 基础信息

- **Base URL**: `http://localhost:8080`
- **协议**: HTTP/HTTPS
- **数据格式**: JSON
- **编码**: UTF-8

## CORS支持

所有API端点都支持跨域请求（CORS），设置了以下响应头：
```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization
```

## API端点

### 1. 服务器状态检查

获取服务器运行状态和基本信息。

**端点**: `GET /api/status`

**请求**: 无需参数

**响应**:
```json
{
  "status": "running",
  "message": "RzNode Web服务器运行正常",
  "port": 8080,
  "has_node_system": true,
  "node_system_info": "Node system attached"
}
```

**字段说明**:
- `status`: 服务器状态（"running" 或 "stopped"）
- `message`: 状态描述消息
- `port`: 服务器监听端口
- `has_node_system`: 是否已连接节点系统
- `node_system_info`: 节点系统信息

---

### 2. 获取节点类型列表

获取所有可用的节点类型定义，用于在前端动态创建节点。

**端点**: `GET /api/node-types`

**请求**: 无需参数

**响应**:
```json
[
  {
    "id_name": "add",
    "ui_name": "Add",
    "color": [0.3, 0.5, 0.7, 1.0],
    "inputs": [
      {
        "name": "Value A",
        "identifier": "value_a",
        "type": "int",
        "optional": false,
        "default_value": "1",
        "min_value": "0",
        "max_value": "10"
      },
      {
        "name": "Value B", 
        "identifier": "value_b",
        "type": "int",
        "optional": false,
        "default_value": "1",
        "min_value": "0", 
        "max_value": "10"
      }
    ],
    "outputs": [
      {
        "name": "Result",
        "identifier": "result",
        "type": "int",
        "optional": false
      }
    ],
    "groups": [
      {
        "identifier": "input_group",
        "type": "input",
        "element_type": "int",
        "runtime_dynamic": true
      }
    ]
  }
]
```

**字段说明**:

**NodeType对象**:
- `id_name`: 节点类型标识符（用于创建节点）
- `ui_name`: 显示名称
- `color`: RGBA颜色数组 [r, g, b, a]
- `inputs`: 输入socket列表
- `outputs`: 输出socket列表  
- `groups`: socket组列表

**Socket对象**:
- `name`: socket显示名称
- `identifier`: socket标识符（用于连接）
- `type`: 数据类型（如 "int", "float", "string"等）
- `optional`: 是否可选
- `default_value`: 默认值（JSON字符串）
- `min_value`: 最小值（可选）
- `max_value`: 最大值（可选）

**SocketGroup对象**:
- `identifier`: 组标识符
- `type`: 组类型（"input" 或 "output"）
- `element_type`: 组元素类型
- `runtime_dynamic`: 是否支持运行时动态添加

---

### 3. 执行节点树

执行前端构建的节点树并返回结果。

**端点**: `POST /api/execute`

**请求**:
```json
{
  "nodes": [
    {
      "id": 1,
      "type": "NumberNode",
      "input_values": {
        "value": "5"
      },
      "position_x": 100.0,
      "position_y": 100.0
    },
    {
      "id": 2,
      "type": "NumberNode", 
      "input_values": {
        "value": "3"
      },
      "position_x": 100.0,
      "position_y": 200.0
    },
    {
      "id": 3,
      "type": "AddNode",
      "input_values": {},
      "position_x": 400.0,
      "position_y": 150.0
    }
  ],
  "links": [
    {
      "from_node": 1,
      "from_socket": "value",
      "to_node": 3,
      "to_socket": "value_a"
    },
    {
      "from_node": 2,
      "from_socket": "value", 
      "to_node": 3,
      "to_socket": "value_b"
    }
  ]
}
```

**响应**:
```json
{
  "success": true,
  "error_message": "",
  "execution_time": 0.025
}
```

**请求字段说明**:

**NodeInstance对象**:
- `id`: 节点实例ID（唯一标识）
- `type`: 节点类型（对应NodeType的id_name）
- `input_values`: 输入值映射（socket标识符 -> JSON值字符串）
- `position_x`: X坐标（用于UI显示）
- `position_y`: Y坐标（用于UI显示）

**NodeLink对象**:
- `from_node`: 源节点ID
- `from_socket`: 源socket标识符
- `to_node`: 目标节点ID  
- `to_socket`: 目标socket标识符

**响应字段说明**:
- `success`: 执行是否成功
- `error_message`: 错误消息（success为false时）
- `execution_time`: 执行时间（秒）

---

### 4. 验证节点树

验证节点树的有效性，但不执行。

**端点**: `POST /api/validate`

**请求**: 与`/api/execute`相同的格式

**响应**:
```json
{
  "valid": true,
  "message": "Node tree is valid"
}
```

**字段说明**:
- `valid`: 节点树是否有效
- `message`: 验证结果消息

---

### 5. 静态文件服务

**端点**: `GET /`

服务器根目录提供静态HTML页面，展示服务器状态。

**端点**: `GET /static/*`

提供前端构建文件（从 `../web_server/web/dist` 目录）。

---

## 错误处理

### HTTP状态码

- `200 OK`: 请求成功
- `500 Internal Server Error`: 服务器内部错误

### 错误响应格式

```json
{
  "error": "错误描述信息"
}
```

### 常见错误

1. **节点系统未连接**:
   ```json
   {
     "error": "Node system not available"
   }
   ```

2. **JSON解析失败**:
   ```json
   {
     "error": "Failed to parse node tree JSON: ..."
   }
   ```

3. **节点创建失败**:
   ```json
   {
     "error": "Failed to create node of type: AddNode"
   }
   ```

4. **连接创建失败**:
   ```json
   {
     "error": "Invalid socket identifier in link"
   }
   ```

---

## 使用示例

### JavaScript前端示例

```javascript
// 1. 检查服务器状态
async function checkStatus() {
  const response = await fetch('/api/status');
  const status = await response.json();
  console.log('Server status:', status);
}

// 2. 获取节点类型
async function loadNodeTypes() {
  const response = await fetch('/api/node-types');
  const nodeTypes = await response.json();
  
  nodeTypes.forEach(nodeType => {
    console.log(`Node: ${nodeType.ui_name} (${nodeType.id_name})`);
  });
}

// 3. 执行节点树
async function executeNodeTree(treeData) {
  const response = await fetch('/api/execute', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(treeData)
  });
  
  const result = await response.json();
  
  if (result.success) {
    console.log('Execution successful, time:', result.execution_time);
  } else {
    console.error('Execution failed:', result.error_message);
  }
}
```

---

## 实现状态

### ✅ 已实现
- 基础HTTP服务器框架
- CORS支持
- 状态检查API
- 节点类型序列化框架
- 节点树反序列化
- 基础执行框架

### 🚧 待完善
- 节点类型缓存自动刷新
- 输入值设置到节点socket
- 执行结果输出值提取
- 更完整的错误处理
- 性能优化

### 📋 后续扩展
- 执行进度回调
- 实时执行状态推送（WebSocket）
- 节点树保存/加载
- 用户会话管理
- 权限控制

---

## 开发调试

### 启动服务器
```cpp
#include "nodes/web_server/web_server.hpp"
#include "nodes/system/node_system.hpp"

// 创建节点系统
auto nodeSystem = std::make_shared<MyNodeSystem>();
nodeSystem->init();

// 创建Web服务器
auto webServer = create_web_server();
webServer->set_node_system(nodeSystem);
webServer->initialize(8080);

// 启动服务器（阻塞调用）
webServer->start();
```

### 测试API
可以使用curl、Postman或浏览器测试API端点：

```bash
# 检查状态
curl http://localhost:8080/api/status

# 获取节点类型
curl http://localhost:8080/api/node-types

# 执行节点树
curl -X POST http://localhost:8080/api/execute \
  -H "Content-Type: application/json" \
  -d @test_tree.json
```
- **CORS**: 已启用，支持跨域请求

## API 端点

### 1. 获取服务器状态

```
GET /api/status
```

**响应示例**:
```json
{
  "status": "running",
  "message": "RzNode Web服务器运行正常",
  "port": 8080,
  "has_node_system": true,
  "node_system_info": "Node system attached"
}
```

### 2. 获取节点类型列表

```
GET /api/node-types
```

**响应示例**:
```json
[
  {
    "id_name": "AddNode",
    "ui_name": "Add",
    "color": [0.8, 0.3, 0.3, 1.0],
    "inputs": [
      {
        "name": "Value A",
        "identifier": "a",
        "type": "int",
        "optional": false,
        "default_value": "0"
      },
      {
        "name": "Value B", 
        "identifier": "b",
        "type": "int",
        "optional": false,
        "default_value": "0"
      }
    ],
    "outputs": [
      {
        "name": "Result",
        "identifier": "result",
        "type": "int",
        "optional": false
      }
    ],
    "groups": []
  }
]
```

### 3. 执行节点树

```
POST /api/execute
Content-Type: application/json
```

**请求体示例**:
```json
{
  "nodes": [
    {
      "id": 1,
      "type": "NumberNode",
      "position_x": 100,
      "position_y": 100,
      "input_values": {
        "value": "10"
      }
    },
    {
      "id": 2,
      "type": "NumberNode", 
      "position_x": 100,
      "position_y": 200,
      "input_values": {
        "value": "20"
      }
    },
    {
      "id": 3,
      "type": "AddNode",
      "position_x": 300,
      "position_y": 150,
      "input_values": {}
    }
  ],
  "links": [
    {
      "from_node": 1,
      "from_socket": "value",
      "to_node": 3,
      "to_socket": "a"
    },
    {
      "from_node": 2,
      "from_socket": "value", 
      "to_node": 3,
      "to_socket": "b"
    }
  ]
}
```

**响应示例**:
```json
{
  "success": true,
  "error_message": "",
  "execution_time": 0.002
}
```

### 4. 验证节点树

```
POST /api/validate
Content-Type: application/json
```

**请求体**: 与执行接口相同的格式

**响应示例**:
```json
{
  "valid": true,
  "message": "Node tree is valid"
}
```

## 错误处理

所有 API 在出错时返回适当的 HTTP 状态码和错误信息：

**错误响应示例**:
```json
{
  "error": "Node system not available"
}
```

## 前端集成示例

### JavaScript/TypeScript

```javascript
class RzNodeAPI {
  constructor(baseURL = 'http://localhost:8080') {
    this.baseURL = baseURL;
  }

  async getStatus() {
    const response = await fetch(`${this.baseURL}/api/status`);
    return await response.json();
  }

  async getNodeTypes() {
    const response = await fetch(`${this.baseURL}/api/node-types`);
    return await response.json();
  }

  async executeNodeTree(nodeTree) {
    const response = await fetch(`${this.baseURL}/api/execute`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(nodeTree)
    });
    return await response.json();
  }

  async validateNodeTree(nodeTree) {
    const response = await fetch(`${this.baseURL}/api/validate`, {
      method: 'POST', 
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(nodeTree)
    });
    return await response.json();
  }
}

// 使用示例
const api = new RzNodeAPI();

// 获取节点类型并注册到 BaklavaJS
api.getNodeTypes().then(nodeTypes => {
  nodeTypes.forEach(nodeType => {
    // 在这里将节点类型注册到 BaklavaJS
    console.log('Available node:', nodeType.ui_name);
  });
});

// 执行节点树
const nodeTree = {
  nodes: [/* ... */],
  links: [/* ... */]
};

api.executeNodeTree(nodeTree).then(result => {
  if (result.success) {
    console.log('Execution successful, time:', result.execution_time);
  } else {
    console.error('Execution failed:', result.error_message);
  }
});
```

### Vue.js + BaklavaJS 集成

```vue
<template>
  <div class="node-editor">
    <baklava-editor :viewModel="baklava" />
    <div class="controls">
      <button @click="executeGraph">执行节点图</button>
      <button @click="validateGraph">验证节点图</button>
    </div>
  </div>
</template>

<script>
import { Editor } from "@baklavajs/core";
import { ViewPlugin } from "@baklavajs/renderer-vue";

export default {
  data() {
    return {
      baklava: new Editor(),
      api: new RzNodeAPI()
    };
  },
  async mounted() {
    this.baklava.use(new ViewPlugin());
    
    // 加载节点类型
    await this.loadNodeTypes();
  },
  methods: {
    async loadNodeTypes() {
      try {
        const nodeTypes = await this.api.getNodeTypes();
        
        // 将 C++ 节点类型转换为 BaklavaJS 节点
        nodeTypes.forEach(nodeType => {
          this.registerBaklavaNode(nodeType);
        });
      } catch (error) {
        console.error('Failed to load node types:', error);
      }
    },
    
    registerBaklavaNode(nodeType) {
      // 创建 BaklavaJS 节点类
      const BaklavaNode = class extends Node {
        constructor() {
          super();
          this.title = nodeType.ui_name;
          this.name = nodeType.id_name;
          
          // 添加输入接口
          nodeType.inputs.forEach(input => {
            this.addInputInterface(input.identifier, input.name, input.type);
          });
          
          // 添加输出接口  
          nodeType.outputs.forEach(output => {
            this.addOutputInterface(output.identifier, output.name, output.type);
          });
        }
      };
      
      this.baklava.registerNodeType(BaklavaNode, {
        category: "RzNode"
      });
    },
    
    async executeGraph() {
      const nodeTree = this.convertBaklavaToRzNode();
      
      try {
        const result = await this.api.executeNodeTree(nodeTree);
        
        if (result.success) {
          this.$message.success('执行成功');
          console.log('Output values:', result.output_values);
        } else {
          this.$message.error(`执行失败: ${result.error_message}`);
        }
      } catch (error) {
        this.$message.error('执行请求失败');
        console.error(error);
      }
    },
    
    convertBaklavaToRzNode() {
      const nodes = [];
      const links = [];
      
      // 转换节点
      this.baklava.nodes.forEach(node => {
        const nodeData = {
          id: parseInt(node.id),
          type: node.name,
          position_x: node.position.x,
          position_y: node.position.y,
          input_values: {}
        };
        
        // 收集输入值
        Object.entries(node.inputs).forEach(([key, input]) => {
          if (input.value !== undefined) {
            nodeData.input_values[key] = String(input.value);
          }
        });
        
        nodes.push(nodeData);
      });
      
      // 转换连接
      this.baklava.connections.forEach(conn => {
        links.push({
          from_node: parseInt(conn.from.node.id),
          from_socket: conn.from.interface.id,
          to_node: parseInt(conn.to.node.id),
          to_socket: conn.to.interface.id
        });
      });
      
      return { nodes, links };
    }
  }
};
</script>
```

## 注意事项

1. **异步执行**: 所有 API 调用都是异步的，请使用 Promise 或 async/await
2. **错误处理**: 始终检查响应的 success 字段和 HTTP 状态码
3. **数据格式**: 输入值需要转换为字符串格式传递给后端
4. **性能**: 大型节点图的执行可能需要较长时间，考虑添加加载状态
5. **实时更新**: 当前 API 是批量同步模式，不支持实时更新

## 扩展功能

将来可能添加的功能：
- WebSocket 支持实时通信
- 节点图的保存和加载
- 调试和单步执行
- 性能分析和统计
