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
            "port": 8082,
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
        "data": [
            {
                "type_name": "int"
            },
            {
                "type_name": "float"
            },
            // 其他值类型...
        ]
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
        "data": [
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
