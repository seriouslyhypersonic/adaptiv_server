#include <iostream>
#include <cstdlib>
#include <memory>

#include <boost/asio/signal_set.hpp>

#include "listener.hpp"
#include "shared_state.hpp"

int main(int argc, char** argv)
{
    // Check command line arguments
    if (argc != 4) {
        std::cerr <<
            "  Usage: server <address> <port> <documentRoot>\n" <<
            "Example:\n" <<
            "         server 127.0.0.1 8080 .\n";
        return EXIT_FAILURE;
    }
    auto address = net::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto documentRoot = argv[3];

    // The I/O context is required for all I/O
    net::io_context ioc;

    // Create and launch a listening port
    std::make_shared<Listener>(
        ioc,
        tcp::endpoint{address, port},
        std::make_shared<SharedState>(documentRoot))->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](error_code const& ec, int)
        {
            // Stop the io_context. This will cause run() to return
            // immediately, eventually destroying the io_context and any
            // remaining handlers in it.
            ioc.stop();
        });

    // Run the I/O service
    ioc.run();

    // If we get here, it means we got a SIGINT or SIGTERM

    return EXIT_SUCCESS;
}