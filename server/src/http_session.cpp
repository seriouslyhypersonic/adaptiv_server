#include <iostream>
#include <chrono>
#include <string>
#include <memory>
#include <utility>
#include <cstddef>

#include "http_session.hpp"
#include "websocket_session.hpp"

// HTTP helpers ----------------------------------------------------------------

// Return a reasonable mim type based on the extension of a file
beast::string_view mimeType(beast::string_view path)
{
    using boost::beast::iequals;
    auto const extension = [&path]
    {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos) {
            return beast::string_view{};
        }
        return path.substr(pos);
    }();

    if(iequals(extension, ".htm"))  return "text/html";
    if(iequals(extension, ".html")) return "text/html";
    if(iequals(extension, ".php"))  return "text/html";
    if(iequals(extension, ".css"))  return "text/css";
    if(iequals(extension, ".txt"))  return "text/plain";
    if(iequals(extension, ".js"))   return "application/javascript";
    if(iequals(extension, ".json")) return "application/json";
    if(iequals(extension, ".xml"))  return "application/xml";
    if(iequals(extension, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(extension, ".flv"))  return "video/x-flv";
    if(iequals(extension, ".png"))  return "image/png";
    if(iequals(extension, ".jpe"))  return "image/jpeg";
    if(iequals(extension, ".jpeg")) return "image/jpeg";
    if(iequals(extension, ".jpg"))  return "image/jpeg";
    if(iequals(extension, ".gif"))  return "image/gif";
    if(iequals(extension, ".bmp"))  return "image/bmp";
    if(iequals(extension, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(extension, ".tiff")) return "image/tiff";
    if(iequals(extension, ".tif"))  return "image/tiff";
    if(iequals(extension, ".svg"))  return "image/svg+xml";
    if(iequals(extension, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
std::string pathConcatenate(beast::string_view base, beast::string_view path)
{
    if (base.empty()) {
        return path.to_string();
    }
    std::string result = base.to_string();

    // Platform separator
#if BOOST_MSVC
    char constexpr pathSeparator = '\\';
#else
    char constexpr pathSeparator = '/';
#endif
    // Append HTTP path to local filesystem path
    if (result.back() == pathSeparator) {
        result.resize(result.size()-1);
    }
    result.append(path.data(), path.size());

#if BOOST_MSVC
    // Windows: replace POSIX separators, if any.
    for (auto& c : result) {
        if (c == '/') {
            c = pathSeparator;
        }
    }
#endif
    return result;
}

/**
 * Produce an HTTP response for the given request. The type of the response
 * object depends on the contents of the request, so the interface requires the
 * caller to pass a generic lambda for receiving the response
 */
template<class Body, class Allocator, class Send>
void
handleRequest(
    beast::string_view documentRoot,
    http::request<Body, http::basic_fields<Allocator>>&& request,
    Send&& send)
{
    // Return a bad request response
    auto const badRequest =
        [&request](beast::string_view why)
        {
            http::response<http::string_body>
                response{http::status::bad_request, request.version()};
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = why.to_string();
            response.prepare_payload();
            return response;
        };

    // Return a not found response
    auto const notFound =
        [&request](beast::string_view target)
        {
            http::response<http::string_body>
                response{http::status::not_found, request.version()};
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() =
                "The resource '" + target.to_string() + "' was not found";
            response.prepare_payload();
            return response;
        };

    // Return a server error response
    auto const serverError =
        [&request](beast::string_view what)
        {
            http::response<http::string_body>
                response{http::status::internal_server_error, request.version()};
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = "An error occured: " + what.to_string() + "'";
            response.prepare_payload();
            return response;
        };

    // Make sure we can handle the method
    if (request.method() != http::verb::get &&
        request.method() != http::verb::head) {
        return send(badRequest("Unknown HTTP-method"));
    }

    // Request path must be absolute and not contain ".."
    if (request.target().empty() ||
        request.target()[0] != '/' ||
        request.target().find("..") != boost::beast::string_view::npos) {
        return send(badRequest("Illegal request-target"));
    }

    // Build the path to the requested file
    std::string path = pathConcatenate(documentRoot, request.target());
    if (request.target().back() == '/') {
        path.append("index.html");
    }

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory) {
        return send(notFound(request.target()));
    }

    // Handle an unknown error
    if (ec) {
        return send(serverError(ec.message()));
    }

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (request.method() == http::verb::head) {
        http::response<http::empty_body>
            response{http::status::ok, request.version()};
        response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(http::field::content_type, mimeType(path));
        response.content_length(size);
        response.keep_alive(request.keep_alive());
        return send(std::move(response));
    }

    // Respond to GET request
    http::response<http::file_body>
        response{std::piecewise_construct,
                 std::make_tuple(std::move(body)),
                 std::make_tuple(http::status::ok, request.version())};
    response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(http::field::content_type, mimeType(path));
    response.content_length(size);
    response.keep_alive(request.keep_alive());

    std::cout << "[sent] " << request.target() << '\n';

    return send(std::move(response));
}

// HTTP helpers ----------------------------------------------------------------

HttpSession::HttpSession(
    tcp::socket&& socket,
    std::shared_ptr<SharedState> const& state)
    : stream_(std::move(socket))
    , state_(state)
{ }

void HttpSession::run()
{
    doRead();
}

void HttpSession::fail(error_code ec, char const* what)
{
    // Don't report on canceled operations
    if (ec == net::error::operation_aborted) {
        return;
    }
    std::cerr << what << ": " << ec.message() << '\n';
}

void HttpSession::doRead()
{
    // Construct a new parser for each message
    parser_.emplace();

    // Apply a reasonable limit to the allowed size of the body in bytes
    // to prevent abuse
    parser_->body_limit(HttpSession::maxBodySize);

    // Set timeout
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(
        stream_,
        buffer_,
        parser_->get(),
        [self = shared_from_this()](error_code ec, std::size_t bytes)
        {
            self->onRead(ec, bytes);
        });
}

void HttpSession::onRead(error_code ec, std::size_t)
{
    // This means they close the connection
    if (ec == http::error::end_of_stream) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Handle the error, if any
    if (ec) {
        return fail(ec, "read");
    }

    // --- WebSocket (check if it is an upgrade)
    if (websocket::is_upgrade(parser_->get())) {
        // Create a WebSocket session by transferring ownership of both the
        // socket and request using release*() which also performs a move().
        std::make_shared<WebSocketSession>(
            stream_.release_socket(),
            state_)->run(parser_->release());
        return;
    }

    // --- HTTP response
    handleRequest(
        state_->documentRoot(),
        std::move(parser_->get()),
        [this](auto&& response)
        {
            // The lifetime of the message has to extend for the duration of
            // the async operation so we use a shared_ptr to manage it.
            using response_type = typename std::decay_t<decltype(response)>;
            auto responseSPtr = std::make_shared<response_type>
                (std::forward<decltype(response)>(response));

            // Write the response
            // Note: declaring self inside the capture causes an ICE in gcc 7.3
            auto self = shared_from_this();
            http::async_write(stream_, *responseSPtr,
            [self, responseSPtr](error_code ec, std::size_t bytes)
            {
                self->onWrite(ec, bytes, responseSPtr->need_eof());
            });
        });
}

void HttpSession::onWrite(error_code ec, std::size_t, bool close)
{
    // Handle error, if any
    if (ec) {
        return fail(ec, "write");
    }

    if (close) {
        // This means we should close the connection, usually because the
        // response indicated the "Connection: close" semantic.
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Read another request
    doRead();
}
