/**
 * 参考资料：
 * https://github.com/oatpp/example-websocket/blob/master/server/src/websocket/WSListener.cpp
 */

#include "nodes/web_server_oatpp/wslistener.hpp"

#include "oatpp/base/Log.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSListener

void WSListener::onPing(const WebSocket& socket, const oatpp::String& message)
{
    OATPP_LOGd(TAG, "onPing");
    socket.sendPong(message);
}

void WSListener::onPong(const WebSocket& socket, const oatpp::String& message)
{
    OATPP_LOGd(TAG, "onPong");
}

void WSListener::onClose(
    const WebSocket& socket,
    v_uint16 code,
    const oatpp::String& message)
{
    OATPP_LOGd(TAG, "onClose code={}", code);
}

void WSListener::readMessage(
    const WebSocket& socket,
    v_uint8 opcode,
    p_char8 data,
    oatpp::v_io_size size)
{
    if (size == 0) {  // message transfer finished

        auto wholeMessage = m_messageBuffer.toString();
        m_messageBuffer.setCurrentPosition(0);

        OATPP_LOGd(TAG, "onMessage message='{}'", wholeMessage);

        /* Send message in reply */
        socket.sendOneFrameText("Hello from oatpp!: " + wholeMessage);
    }
    else if (size > 0) {  // message frame received
        m_messageBuffer.writeSimple(data, size);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WSInstanceListener

std::atomic<v_int32> WSInstanceListener::SOCKETS(0);

void WSInstanceListener::onAfterCreate(
    const oatpp::websocket::WebSocket& socket,
    const std::shared_ptr<const ParameterMap>& params)
{
    SOCKETS++;
    OATPP_LOGd(
        TAG, "New Incoming Connection. Connection count={}", SOCKETS.load());

    /* In this particular case we create one WSListener per each connection */
    /* Which may be redundant in many cases */
    socket.setListener(std::make_shared<WSListener>());
}

void WSInstanceListener::onBeforeDestroy(
    const oatpp::websocket::WebSocket& socket)
{
    SOCKETS--;
    OATPP_LOGd(TAG, "Connection closed. Connection count={}", SOCKETS.load());
}