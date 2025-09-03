#include <memory>
#include <string>
#ifdef GEOM_EXTENSION

/**
 * 参考资料：
 * https://github.com/oatpp/example-websocket/blob/master/server/src/websocket/WSListener.cpp
 */

#include "nodes/web_server_oatpp/geom_ws_listener.hpp"
#include "spdlog/spdlog.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GeometryWSListener

void GeometryWSListener::onPing(
    const WebSocket& socket,
    const oatpp::String& message)
{
    spdlog::debug("WebSocket: onPing");
    socket.sendPong(message);
}

void GeometryWSListener::onPong(
    const WebSocket& socket,
    const oatpp::String& message)
{
    spdlog::debug("WebSocket: onPong");
}

void GeometryWSListener::onClose(
    const WebSocket& socket,
    v_uint16 code,
    const oatpp::String& message)
{
    spdlog::debug("WebSocket: onClose code={}", code);
}

void GeometryWSListener::readMessage(
    const WebSocket& socket,
    v_uint8 opcode,
    p_char8 data,
    oatpp::v_io_size size)
{
    // TODO: 添加收到网页端传来消息时的反馈，例如：
    // 网页端要求重新获取几何信息，则发送（重新执行节点树，或有缓存机制）
    // 网页端更新某个几何体的顶点位置，则将该信息更新到对应节点（重新执行节点树）
    if (size == 0) {  // message transfer finished

        auto whole_message = message_buffer_.toString();
        message_buffer_.setCurrentPosition(0);

        spdlog::debug("WebSocket: onMessage message='{}'", *whole_message);

        /* Send message in reply */
        socket.sendOneFrameText("Hello from oatpp!: " + whole_message);
    }
    else if (size > 0) {  // message frame received
        message_buffer_.writeSimple(data, size);
    }
}

bool GeometryWSListener::send_message_via_ws(const std::string& message) const
{
    socket_.sendOneFrameText(message);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GeometryWSInstanceListener

std::atomic<v_int32> GeometryWSInstanceListener::SOCKETS(0);

void GeometryWSInstanceListener::onAfterCreate(
    const oatpp::websocket::WebSocket& socket,
    const std::shared_ptr<const ParameterMap>& params)
{
    SOCKETS++;
    spdlog::debug(
        "WebSocket: New Incoming Connection. Connection count={}",
        SOCKETS.load());

    /* In this particular case we create one WSListener per each connection */
    /* Which may be redundant in many cases */
    auto listener = std::make_shared<GeometryWSListener>(socket, SOCKETS);

    socket.setListener(listener);
    listeners_.push_back(listener);
}

void GeometryWSInstanceListener::onBeforeDestroy(
    const oatpp::websocket::WebSocket& socket)
{
    SOCKETS--;
    spdlog::debug(
        "WebSocket: Connection closed. Connection count={}", SOCKETS.load());
}

bool GeometryWSInstanceListener::send_message_via_ws(
    const std::string& message) const
{
    if (listeners_.empty()) {
        spdlog::warn("WebSocket: No active connections to send message");
        return false;
    }

    bool all_success = true;
    for (const auto& listener : listeners_) {
        if (!listener->send_message_via_ws(message)) {
            all_success = false;
        }
    }
    return all_success;
}

#endif