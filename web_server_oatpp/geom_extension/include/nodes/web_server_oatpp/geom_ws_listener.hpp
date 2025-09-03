/**
 * 参考资料：
 * https://github.com/oatpp/example-websocket/blob/master/server/src/websocket/WSListener.hpp
 */

#pragma once

#include <memory>
#include <vector>

#include "oatpp-websocket/ConnectionHandler.hpp"
#include "oatpp-websocket/WebSocket.hpp"

/**
 * WebSocket listener listens on incoming WebSocket events.
 */
class GeometryWSListener : public oatpp::websocket::WebSocket::Listener {
   private:
    /**
     * Buffer for messages. Needed for multi-frame messages.
     */
    oatpp::data::stream::BufferOutputStream message_buffer_;

    /**
     * WebSocket
     */
    const oatpp::websocket::WebSocket& socket_;

    /**
     * ID
     */
    const int id_;

   public:
    GeometryWSListener(const oatpp::websocket::WebSocket& socket, const int id)
        : socket_(socket),
          id_(id)
    {
    }

   public:
    /**
     * Called on "ping" frame.
     */
    void onPing(const WebSocket& socket, const oatpp::String& message) override;

    /**
     * Called on "pong" frame
     */
    void onPong(const WebSocket& socket, const oatpp::String& message) override;

    /**
     * Called on "close" frame
     */
    void onClose(
        const WebSocket& socket,
        v_uint16 code,
        const oatpp::String& message) override;

    /**
     * Called on each message frame. After the last message will be called
     * once-again with size == 0 to designate end of the message.
     */
    void readMessage(
        const WebSocket& socket,
        v_uint8 opcode,
        p_char8 data,
        oatpp::v_io_size size) override;

    /**
     * Send message to the client
     * @param dto - DTO object to send
     * @return - true on success, false on error
     */
    bool send_message_via_ws(const std::string& message) const;
};

/**
 * Listener on new WebSocket connections.
 */
class GeometryWSInstanceListener
    : public oatpp::websocket::ConnectionHandler::SocketInstanceListener {
   public:
    /**
     * Counter for connected clients.
     */
    static std::atomic<v_int32> SOCKETS;

   private:
    std::vector<std::shared_ptr<GeometryWSListener>> listeners_;

   public:
    /**
     *  Called when socket is created
     */
    void onAfterCreate(
        const oatpp::websocket::WebSocket& socket,
        const std::shared_ptr<const ParameterMap>& params) override;

    /**
     *  Called before socket instance is destroyed.
     */
    void onBeforeDestroy(const oatpp::websocket::WebSocket& socket) override;

    /**
     * Send message to all connected clients
     * @param dto - DTO object to send
     * @return - true on success, false on error
     */
    bool send_message_via_ws(const std::string& message) const;
};
