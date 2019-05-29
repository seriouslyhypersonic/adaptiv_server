/*
 * Copyright (c) Nuno Alves de Sousa 2019
 *
 * Use, modification and distribution is subject to the Boost Software License,
 * Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <memory>
#include <cstddef>
#include <cstdint>

#include "net.hpp"
#include "beast.hpp"
#include "shared_state.hpp"

class HttpSession: public std::enable_shared_from_this<HttpSession>
{
    beast::tcp_stream stream_;

    beast::flat_buffer buffer_;
    std::shared_ptr<SharedState> state_;
    // The parser is stored in an optional container so we can construct it
    // from scratch at the beginning of each new message
    std::optional<http::request_parser<http::string_body>> parser_;

    void fail(error_code ec, char const* what); ///< Report a failure
    void doRead();
    void onRead(error_code, std::size_t);
    void onWrite(error_code ec, std::size_t, bool close);

public:
    static const std::uint64_t maxBodySize = 10'000; ///< In bytes

    HttpSession(
        tcp::socket&& socket,
        std::shared_ptr<SharedState> const& state);

    void run();
};


#endif //HTTPSESSION
