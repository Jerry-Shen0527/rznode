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
    if (size == 0) {  // message transfer finished

        auto wholeMessage = m_messageBuffer.toString();
        m_messageBuffer.setCurrentPosition(0);

        spdlog::debug("WebSocket: onMessage message='{}'", *wholeMessage);

        /* Send message in reply */
        socket.sendOneFrameText("Hello from oatpp!: " + wholeMessage);
    }
    else if (size > 0) {  // message frame received
        m_messageBuffer.writeSimple(data, size);
    }
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
    socket.setListener(std::make_shared<GeometryWSListener>());
}

void GeometryWSInstanceListener::onBeforeDestroy(
    const oatpp::websocket::WebSocket& socket)
{
    SOCKETS--;
    spdlog::debug(
        "WebSocket: Connection closed. Connection count={}", SOCKETS.load());
}

#endif