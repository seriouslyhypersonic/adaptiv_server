/*
 * Copyright (c) Nuno Alves de Sousa 2019
 *
 * Use, modification and distribution is subject to the Boost Software License,
 * Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "net.hpp"
#include "beast.hpp"
#include "shared_state.hpp"

class WebSocketSession: public std::enable_shared_from_this<WebSocketSession>
{
    beast::flat_buffer buffer_;
    websocket::stream<beast::tcp_stream> websocket_;
    std::shared_ptr<SharedState> state_;
    std::vector<std::shared_ptr<std::string const>> queue_;

    void fail(error_code ec, char const* what);
    void onAccept(error_code ec);
    void doRead();
    void onRead(error_code ec, std::size_t bytesTransferred);
    void doWrite();
    void onWrite(error_code ec, std::size_t bytesTransferred);

    void onSend(std::shared_ptr<std::string const> const& messageSPtr);

public:
    WebSocketSession(
        tcp::socket&& socket,
        std::shared_ptr<SharedState> const& state);

    ~WebSocketSession();

    template<class Body, class Allocator>
    void
    run(http::request<Body, http::basic_fields<Allocator>> request);

    // Send a message (to all sessions>
    void send(std::shared_ptr<std::string const> const& messageSPtr);
};

template<class Body, class Allocator>
void WebSocketSession::run(
    http::request<Body, boost::beast::http::basic_fields<Allocator>> request)
{
    // Set suggest timeout settings for the WebSocket
    websocket_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set decorator to change the server handshake
    websocket_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& response)
        {
            response.set(
                http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " adaptive-server");
        }));

    // Accept the WebSocket handshake
    websocket_.async_accept(
        request,
        [self = shared_from_this()](error_code const& ec)
        {
            self->onAccept(ec);
        });
}

#endif //WEBSOCKETSESSION
