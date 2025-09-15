# RZNode Web Server API 文档

## 简介

RZNode Web Server 提供了一组 RESTful API 接口，允许用户通过 HTTP 请求与 RZNode 系统进行交互。

## API 列表

### 1. 获取服务器状态

- **URL**: `GET /api/status`
- **描述**: 检查 Web 服务器是否正常运行
- **成功响应（200）**:

    ```json
    {
        "code": 0,
        "message": "success",
        "data": {
            "has_node_system": true,
            "message": "RzNode Web服务器运行正常",
            "port": 8080,
            "status": "running"
        }
    }
    ```

---

### 2. 获取值类型列表

- **URL**: `GET /api/value-types`
- **描述**: 获取所有注册的节点中，端口所使用的值类型列表
- **成功响应（200）**:

    ```json
    {
        "code": 0,
        "message": "success",
        "data": {
            "value_types": [
                {
                    "type_name": "int"
                },
                {
                    "type_name": "string"
                },
                // 其他值类型...
            ]
        }
    }
    ```

- **错误响应（500）**:

    ```json
    {
        "code": 1,
        "message": "Failed to get value types: <error_message>",
        "data": null
    }
    ```

    或

    ```json
    {
        "code": 2,
        "message": "Node system not available",
        "data": null
    }
    ```

---

### 3. 获取节点类型列表

- **URL**: `GET /api/node-types`
- **描述**: 获取所有注册的节点类型及其详细信息（如输入输出端口、分组、颜色等）
- **成功响应（200）**:

    ```json
    {
        "code": 0,
        "message": "success",
        "data": {
            "node_types": [
                {
                    "id_name": "add",
                    "ui_name": "Add",
                    "color": [
                        0.30000001192092896,
                        0.5,
                        0.699999988079071,
                        1.0
                    ],
                    "inputs": [
                        {
                            "default_value": "1",
                            "identifier": "value",
                            "max_value": "10",
                            "min_value": "0",
                            "name": "value",
                            "optional": false,
                            "type": "int"
                        },
                        {
                            "default_value": "1",
                            "identifier": "value2",
                            "max_value": "10",
                            "min_value": "0",
                            "name": "value2",
                            "optional": false,
                            "type": "int"
                        },
                        // 其他输入端口...
                    ],
                    "outputs": [
                        {
                            "identifier": "value",
                            "name": "value",
                            "optional": false,
                            "type": "int"
                        },
                        // 其他输出端口...
                    ],
                    // 其他节点属性...
                }
                // 其他节点类型...
            ]
        }
    }
    ```

- **错误响应（500）**:

    ```json
    {
        "code": 1,
        "message": "Node system not available",
        "data": null
    }
    ```

    或

    ```json
    {
        "code": 2,
        "message": "Failed to get node types: <error_message>",
        "data": null
    }
    ```

---

### 4. 验证节点树结构

- **URL**: `POST /api/validate`
- **描述**: 验证节点树结构的合法性（不执行，仅校验），请求体为节点树 JSON
- **请求体示例**:

    ```json
    {
        "links": [
            {
                "from_node": "test-id-1",
                "from_socket": "value",
                "to_node": "test-id-2",
                "to_socket": "info"
            },
            // 其他连接...
        ],
        "nodes": [
            {
                "id": "test-id-1",
                "input_values": {
                    "value": 3,
                    "value2": 5
                },
                "type": "add"
            },
            {
                "id": "test-id-2",
                "input_values": null,
                "type": "print"
            },
            // 其他节点...
        ]
    }
    ```

  - `nodes`：节点数组，每个节点需包含唯一 `id`，`type`（节点类型），`input_values`（输入端口初始值，可为空对象）。
  - `links`：连接数组，每个连接需指定源节点/端口和目标节点/端口。
- **成功响应（200）**:

    ```json
    {
        "code": 0,
        "data": {
            "valid": true,
            "error": ""
        },
        "message": "success"
    }
    ```

    或

    ```json
    {
        "code": 0,
        "data": {
            "valid": false,
            "error": "<error_message>"
        },
        "message": "success"
    }
    ```

- **错误响应（500）**:

    ```json
    {
        "code": 1,
        "message": "Node system not available",
        "data": null
    }
    ```

    或

    ```json
    {
        "code": 2,
        "message": "Failed to validate node tree: <error_message>",
        "data": null
    }
    ```

---

### 5. 执行节点树

- **URL**: `POST /api/execute`
- **描述**: 执行节点树，返回执行结果（如耗时），请求体为节点树 JSON，格式同上（见“验证节点树结构”API）。
- **请求体示例**:

    ```json
    {
        "links": [
            {
                "from_node": "test-id-1",
                "from_socket": "value",
                "to_node": "test-id-2",
                "to_socket": "info"
            },
            // 其他连接...
        ],
        "nodes": [
            {
                "id": "test-id-1",
                "input_values": {
                    "value": 3,
                    "value2": 5
                },
                "type": "add"
            },
            {
                "id": "test-id-2",
                "input_values": null,
                "type": "print"
            },
            // 其他节点...
        ]
    }
    ```

- **成功响应（200）**:

    ```json
    {
        "code": 0,
        "data": {
            "success": true,
            "execution_time": 0.0,
            "error": ""
        },
        "message": "success"
    }
    ```

    或

    ```json
    {
        "code": 0,
        "data": {
            "success": false,
            "execution_time": 0.0,
            "error": "<error_message>"
        },
        "message": "success"
    }
    ```

- **错误响应（500）**:

    ```json
    {
        "code": 1,
        "message": "Node system not available",
        "data": null
    }
    ```

    或

    ```json
    {
        "code": 2,
        "message": "Failed to execute node tree: <error_message>",
        "data": null
    }
    ```

---

## WebSocket API

### 概述

RZNode Web Server 除了提供 RESTful API 外，还通过 WebSocket 协议支持实时几何可视化功能。WebSocket 连接主要用于：

- 实时传输几何数据到前端可视化器
- 接收用户在可视化器中的交互事件
- 支持双向的实时通信

### 连接信息

- **WebSocket URL**: `ws://localhost:8080/geometry`
- **协议**: WebSocket (RFC 6455)
- **数据格式**: JSON

### 消息类型

#### 1. 几何数据消息 (Backend → Frontend)

**几何数据更新**：

```typescript
interface GeometryMessage {
    type: 'geometry_update' | 'geometry_clear' | 'scene_update'
    geometries: GeometryData[]
    scene_id: string       // 当前由于只有一棵节点树，因此只有一个场景，scene_id 可固定为 "default"
    timestamp: number
}

interface GeometryData {
    id: string                    // 几何对象唯一标识符
    type: 'mesh' | 'line' | 'point'  // 几何类型
    mesh_data?: {               // 网格数据（type为'mesh'时必需）
        vertices: number[]       // 顶点数组 [x1, y1, z1, x2, y2, z2, ...]
        face_vertex_counts: number[] // 每个面的顶点数量 [3, 3, 4, ...]
        face_vertex_indices: number[] // 面顶点索引 [0, 1, 2, 2, 3, 0, ...]
        normals?: number[]     // 法向量数组 [nx1, ny1, nz1, nx2, ny2, nz2, ...]
        colors?: number[]      // 颜色数组 [r1, g1, b1, a1, r2, g2, b2, a2, ...]
        uvs?: number[]         // 纹理坐标数组 [u1, v1, u2, v2, ...]
    },
    line_data?: {               // 线数据（type为'line'时必需）
        vertices: number[]       // 顶点数组 [x1, y1, z1, x2, y2, z2, ...]
        widths?: number[]        // 每个顶点的线宽 [w1, w2, ...]
        vert_counts: number[] // 每条线的顶点数量 [2, 2, 3, ...]
        colors?: number[]      // 颜色数组 [r1, g1, b1, a1, r2, g2, b2, a2, ...]
        normals?: number[]     // 法向量数组 [nx1, ny1, nz1, nx2, ny2, nz2, ...]
        periodic?: boolean      // 是否为周期性闭合线
    },
    point_data?: {              // 点数据（type为'point'时必需）
        vertices: number[]       // 顶点数组 [x1, y1, z1, x2, y2, z2, ...]
        widths?: number[]         // 每个点的大小 [s1, s2, ...]
        colors?: number[]        // 颜色数组 [r1, g1, b1, a1, r2, g2, b2, a2, ...]
        normals?: number[]       // 法向量数组 [nx1, ny1, nz1, nx2, ny2, nz2, ...]
    },
    transform?: number[]          // 4x4变换矩阵（行主序）
}
```

**消息示例**：

```json
{
    "type": "geometry_update",
    "scene_id": "default",
    "timestamp": 1693536000000,
    "geometries": [
        {
            "id": "mesh_001",
            "type": "mesh",
            "mesh_data": {
                "vertices": [0, 0, 0, 1, 0, 0, 0.5, 1, 0],
                "face_vertex_counts": [3],
                "face_vertex_indices": [0, 1, 2],
                "normals": [0, 0, 1, 0, 0, 1, 0, 0, 1],
                "colors": [1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1]
            },
            "transform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        }
    ]
}
```

**几何数据清空**：

```json
{
    "type": "geometry_clear",
    "scene_id": "default",
    "timestamp": 1693536000000,
    "geometries": []
}
```

**场景更新**：

```json
{
    "type": "scene_update",
    "scene_id": "default",
    "timestamp": 1693536000000,
    "geometries": [
        {
            "id": "point_001",
            "type": "point",
            "point_data": {
                "vertices": [0, 0, 0, 1, 1, 1],
                "widths": [1.0, 2.0],
                "colors": [1, 1, 0, 1, 0, 1, 1, 1]
            }
        }
    ]
}
```

**JSON格式优势**（当前阶段）：

- 易于调试和日志记录
- 前后端开发并行，无需协议协商
- 浏览器原生支持，开发工具友好
- 快速原型验证和功能迭代
- 无需复杂的二进制解析逻辑

#### 2. 用户交互消息 (Frontend → Backend)

暂无，待扩展

### 连接流程

1. **建立连接**

   ```javascript
   const ws = new WebSocket('ws://localhost:8080/geometry');
   ```

2. **连接成功**

   ```javascript
   ws.onopen = () => {
       console.log('WebSocket连接已建立');
   };
   ```

3. **接收消息**

   ```javascript
   ws.onmessage = (event) => {
       const message = JSON.parse(event.data);
       // 处理几何数据或其他消息
   };
   ```

4. **发送消息**

    暂无，待扩展

5. **连接关闭**

   ```javascript
   ws.onclose = () => {
       console.log('WebSocket连接已关闭');
   };
   ```

### 错误处理

**连接错误**：

```javascript
ws.onerror = (error) => {
    console.error('WebSocket错误:', error);
};
```

**消息格式错误**：

- 服务器会忽略格式不正确的消息
- 客户端应验证接收到的消息格式

### 最佳实践

1. **重连机制**
   - 实现指数退避重连策略
   - 限制最大重连次数
   - 在网络不稳定时自动重连

2. **消息缓冲**
   - 在连接断开时缓冲待发送消息
   - 连接恢复后批量发送

3. **性能优化**
   - 对频繁的几何更新进行节流
   - 使用二进制格式传输大量几何数据（未来考虑）

4. **状态同步**
   - 连接建立后请求当前场景状态
   - 处理并发修改冲突

### 架构说明

#### 当前实现 (Phase 1 - 开发中)

- **JSON传输**: 使用标准JSON格式传输几何数据，便于调试和快速开发
- **混合通信模式**: 节点编辑器使用 RESTful API，几何可视化器使用 WebSocket
- **单向数据流**: 后端 → 可视化器（仅几何数据传输）
- **oatpp WebSocket**: 基于oatpp库的WebSocket实现，发送JSON消息
- **开发友好**: 浏览器原生支持，易于调试和日志记录

#### Phase 2 规划

- **全 WebSocket 架构**: 两个页面均使用 WebSocket 通信
- **双向数据流**: 可视化器交互数据回传到节点树
- **完整协议替代**: WebSocket 协议完全替代 RESTful API
- **交互节点**: 支持 `node_get_current_geom_in_visualizer` 等交互节点
- **性能优化**: 根据需要考虑二进制格式或数据压缩

#### 技术实现重点

**几何序列化模块** (`ext/GCore/`)：

- `MeshComponent` → JSON序列化接口
- 集成现有USD/IGL视图，保持API兼容
- 支持顶点、索引、颜色、法向量的完整导出

**WebSocket传输层** (`web_server/`)：

- 基于oatpp实现JSON消息传输
- 连接管理、错误处理、重连机制
- 多客户端消息分发

**开发阶段优势**：

- 快速原型验证和功能迭代
- 前后端并行开发，无协议复杂性
- 易于单元测试和集成测试
- 浏览器开发工具直接支持消息查看
