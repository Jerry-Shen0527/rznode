# RZNode WebSocket 客户端使用示例

## 1. JavaScript 浏览器客户端

```javascript
// 连接到WebSocket服务器
const ws = new WebSocket('ws://localhost:8080/ws');

// 连接建立时
ws.onopen = function(event) {
    console.log('Connected to RZNode WebSocket server');
    
    // 发送测试消息
    ws.send('Hello from client!');
};

// 接收消息时
ws.onmessage = function(event) {
    console.log('Message from server:', event.data);
    // 服务器会回复: "Hello from oatpp!: Hello from client!"
};

// 连接关闭时
ws.onclose = function(event) {
    console.log('Connection closed:', event.code, event.reason);
};

// 连接错误时
ws.onerror = function(error) {
    console.error('WebSocket error:', error);
};
```

## 2. Python 客户端 (使用 websockets 库)

```python
import asyncio
import websockets
import json

async def test_websocket():
    uri = "ws://localhost:8080/ws"
    
    try:
        # 连接到服务器
        async with websockets.connect(uri) as websocket:
            print("Connected to RZNode WebSocket server")
            
            # 发送消息
            await websocket.send("Hello from Python client!")
            
            # 接收回复
            response = await websocket.recv()
            print(f"Server response: {response}")
            
            # 发送JSON格式的消息
            json_message = json.dumps({
                "type": "test",
                "message": "JSON test from Python"
            })
            await websocket.send(json_message)
            
            # 接收JSON回复
            json_response = await websocket.recv()
            print(f"JSON response: {json_response}")
            
    except Exception as e:
        print(f"Error: {e}")

# 运行测试
if __name__ == "__main__":
    asyncio.run(test_websocket())
```

## 3. Node.js 客户端 (使用 ws 库)

```javascript
const WebSocket = require('ws');

// 连接到服务器
const ws = new WebSocket('ws://localhost:8080/ws');

ws.on('open', function open() {
    console.log('Connected to RZNode WebSocket server');
    
    // 发送消息
    ws.send('Hello from Node.js client!');
    
    // 发送JSON消息
    const jsonMessage = JSON.stringify({
        type: 'geometry_request',
        node_id: 'test_node_123'
    });
    ws.send(jsonMessage);
});

ws.on('message', function message(data) {
    console.log('Received:', data.toString());
});

ws.on('close', function close() {
    console.log('Disconnected from server');
});

ws.on('error', function error(err) {
    console.error('WebSocket error:', err);
});
```

## 4. C++ 客户端示例 (使用 Beast/Boost)

```cpp
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    try {
        net::io_context ioc;
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};

        // 连接到服务器
        auto const results = resolver.resolve("localhost", "8080");
        auto ep = net::connect(ws.next_layer(), results);

        // 设置WebSocket握手
        std::string host = "localhost:" + std::to_string(ep.port());
        ws.handshake(host, "/ws");

        // 发送消息
        ws.write(net::buffer(std::string("Hello from C++ client!")));

        // 读取回复
        beast::flat_buffer buffer;
        ws.read(buffer);
        
        std::cout << "Server response: " << beast::make_printable(buffer.data()) << std::endl;
        
        // 关闭连接
        ws.close(websocket::close_code::normal);
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

## 5. 测试你的WebSocket服务

### 方法一：使用测试页面

1. 启动你的RZNode服务器：`./run_server_oatpp.cpp.exe`
2. 在浏览器中打开 `websocket_test.html`
3. 点击"连接到服务器"
4. 输入消息并发送，观察服务器回复

### 方法二：使用浏览器开发者工具

```javascript
// 在浏览器控制台中执行
const ws = new WebSocket('ws://localhost:8080/ws');
ws.onmessage = (e) => console.log('Received:', e.data);
ws.onopen = () => {
    console.log('Connected!');
    ws.send('Test message from console');
};
```

### 方法三：使用curl (如果服务器支持)

```bash
# 注意：curl的WebSocket支持可能有限，推荐使用上述其他方法
curl --include \
     --no-buffer \
     --header "Connection: Upgrade" \
     --header "Upgrade: websocket" \
     --header "Sec-WebSocket-Key: SGVsbG8sIHdvcmxkIQ==" \
     --header "Sec-WebSocket-Version: 13" \
     http://localhost:8080/ws
```

## 当前服务器功能

根据你的 `wslistener.cpp` 实现，服务器目前支持：

- ✅ **连接管理**: 自动处理连接建立和关闭
- ✅ **消息回应**: 对收到的消息回复 "Hello from oatpp!: [原消息]"
- ✅ **连接计数**: 跟踪当前连接数量
- ✅ **Ping/Pong**: 自动处理心跳检测

## 下一步开发建议

基于项目文档中的几何可视化架构，你可以考虑：

1. **扩展消息类型**: 添加JSON格式的几何数据传输
2. **几何数据流**: 实现从节点树到可视化器的几何数据流
3. **双向通信**: 添加从可视化器到节点树的交互反馈
