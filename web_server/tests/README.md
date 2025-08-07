# Web Server 测试说明

## 测试概述

`TEST_F(WebServerTest, NodeSystemExecution)` 是一个复杂的集成测试，用于验证以下功能：

1. **服务器启动和配置** - 启动Web服务器并加载节点配置
2. **节点系统集成** - 验证节点系统与Web服务器的正确集成
3. **HTTP API功能** - 测试所有主要的API端点
4. **JSON数据交换** - 验证前后端数据序列化/反序列化

## 测试流程

### 1. 服务器初始化
- 在端口8082上初始化Web服务器
- 创建动态加载节点系统
- 加载测试节点配置 (`test_nodes.json`)
- 启动后台服务器线程

### 2. API端点测试

#### 2.1 服务器状态API (`/api/status`)
```cpp
// 测试内容：
// - HTTP状态码200
// - 返回JSON格式正确
// - 服务器状态为"running"
// - 端口号正确 (8082)
// - 节点系统已连接
```

#### 2.2 节点类型查询API (`/api/node-types`)
```cpp
// 测试内容：
// - HTTP状态码200
// - 返回JSON数组
// - 至少包含一个节点类型
// - 每个节点类型包含必需字段：id_name, ui_name, inputs, outputs
// - 输出所有可用节点类型信息
```

#### 2.3 节点树验证API (`/api/validate`)
```cpp
// 测试内容：
// - 使用空节点树测试验证功能
// - HTTP状态码200
// - 返回有效的验证结果JSON
```

#### 2.4 节点树执行API (`/api/execute`)
```cpp
// 测试内容：
// - 动态创建包含一个节点的简单测试树
// - 使用第一个可用节点类型
// - 验证执行结果格式正确
// - 包含success字段
```

### 3. 资源清理
- 停止Web服务器
- 等待后台线程结束
- 验证服务器已停止

## 测试配置文件

`test_nodes.json` 包含两个测试节点类型：

1. **TestNumberNode** - 简单数值节点
   - 无输入
   - 一个整数输出

2. **TestAddNode** - 加法节点
   - 两个整数输入 (value_a, value_b)
   - 一个整数输出 (result)

## 运行测试

### 编译
```bash
cd build
cmake --build . --target web_server_tests
```

### 运行
```bash
./bin/web_server_tests
```

### 预期输出示例
```
[==========] Running 4 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 4 tests from WebServerTest
[ RUN      ] WebServerTest.NodeSystemExecution
Server status: {"status":"running","message":"RzNode Web服务器运行正常","port":8082,"has_node_system":true,"node_system_info":"Node system attached"}
Available node types (2):
  - Test Number (TestNumberNode)
  - Test Add (TestAddNode)
Empty tree validation: {"valid":true,"message":"Node tree is valid"}
Simple execution test: {"success":true,"error_message":"","execution_time":0.001,"output_values":{}}
[       OK ] WebServerTest.NodeSystemExecution (150 ms)
```

## 故障排除

### 常见问题

1. **端口占用** - 如果端口8082被占用，修改测试中的端口号
2. **配置文件未找到** - 确保 `test_nodes.json` 在正确路径
3. **编译错误** - 确保包含了所有必需的依赖库

### 依赖检查
- httplib (HTTP客户端/服务器)
- nlohmann/json (JSON处理)
- Google Test (测试框架)
- spdlog (日志)

## 扩展测试

可以扩展此测试以包含：
- 更复杂的节点树构建
- 错误处理测试
- 性能测试
- 并发访问测试
- WebSocket功能测试
