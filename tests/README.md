# Node System Python Bindings - Simplified Test Suite

精简的测试套件，解决DLL和配置文件路径问题。

## ⚠️ 重要：路径和工作目录配置

### 关键修复
所有测试文件现在都设置正确的工作目录：
```python
BINARY_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "..", "Binaries", "Release")
os.chdir(BINARY_DIR)  # 这是关键！
```

这样可以：
1. ✅ **找到DLL依赖**：所有.dll文件在同一目录
2. ✅ **直接加载test_nodes.json**：配置文件在当前目录
3. ✅ **模块导入正常**：Python模块(.pyd)在PATH中

## 核心测试文件

### 🎯 **主要测试文件**

1. **`nodes_core.py`** - 核心功能测试
   - ✅ 基础节点树操作
   - ✅ 注册节点集成测试
   - ✅ 动态套接字测试

2. **`node_system.py`** - 系统功能测试
   - ✅ 系统创建和初始化
   - ✅ 配置加载和节点创建
   - ✅ 网络执行测试

3. **`test_registered_nodes.py`** - 专项节点测试
   - ✅ 每个节点类型验证
   - ✅ 连接兼容性测试

4. **`simple_example.py`** - 最简使用示例
   - ✅ 系统基本使用流程

### 📁 **已清理的文件**
以下文件已精简，避免冗余：
- `demo_bindings.py` → 已清空
- `demo_system.py` → 已清空
- `test_bindings.py` → 已清空
- `complete_execution_demo.py` → 已清空
- `comprehensive_execution_test.py` → 已清空
- `network_execution_demo.py` → 已清空

## 支持的节点类型

### Add Node (`add`)
- **输入**: `value` (int), `value2` (int), `input_group` (动态)
- **输出**: `value` (int), `output_group` (动态)

### Print Node (`print`)
- **输入**: `info` (int)
- **输出**: 无（控制台日志输出）

### Sub Node (`sub`)
- **输入**: `value` (int), `float` (float)
- **输出**: `value` (int)

## 运行测试

### 推荐方式
```bash
# 核心功能测试
cd core/tests
python nodes_core.py

# 系统功能测试
cd system/tests
python node_system.py

# 专项测试
python test_registered_nodes.py

# 简单示例
python simple_example.py
```

## 关键功能验证

### ✅ **节点创建**
```python
add_node = tree.add_node("add")
assert "value" in [s.identifier for s in add_node.inputs]
```

### ✅ **节点连接**
```python
can_link = tree.can_create_link(add_output, print_input)
link = tree.add_link(add_output, print_input)
```

### ✅ **系统执行**
```python
node_system.execute()  # 看到 "Print Info: X" 消息
```

### ✅ **动态套接字**
```python
new_socket = add_node.group_add_socket(
    "input_group", "int", "dynamic", "Dynamic", core.PinKind.Input
)
```

## 故障排除

### 常见问题解决
1. **DLL找不到** → 工作目录已修复为 `Binaries/Debug`
2. **test_nodes.json加载失败** → 现在在当前工作目录查找
3. **模块导入失败** → 检查编译输出是否在 `Binaries/Debug`

### 验证设置
```python
import os
print(f"当前工作目录: {os.getcwd()}")
print(f"test_nodes.json存在: {os.path.exists('test_nodes.json')}")
```

## 简化的文件结构

```
rznode/
├── tests/
│   └── README.md                      # 本文档
├── core/tests/
│   └── nodes_core.py                  # 核心功能测试
└── system/tests/
    ├── node_system.py                 # 系统功能测试
    ├── test_registered_nodes.py       # 专项节点测试
    └── simple_example.py              # 使用示例
```

这个精简的测试套件专注于核心功能验证，去除了所有冗余的演示代码，通过正确的工作目录设置解决了DLL和配置文件路径问题。