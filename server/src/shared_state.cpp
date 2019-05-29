#include <vector>
#include <memory>
#include <iostream>

#include <boost/date_time.hpp>

#include "shared_state.hpp"
#include "websocket_session.hpp"

SharedState::SharedState(std::string documentRoot)
    : documentRoot_(std::move(documentRoot))
{ }

void SharedState::join(WebSocketSession* session)
{
    sessions_.insert(session);
}

void SharedState::leave(WebSocketSession* session)
{
    sessions_.erase(session);
}

void SharedState::send(std::string message)
{
    // Put a message in a shared pointer so we can re-use it for each client
    auto const messageSPtr =
        std::make_shared<std::string const>(std::move(message));

    // Send message to each client
    for(auto session : sessions_) {
        session->send(messageSPtr);
    }

    // Show the sent message on cout
    auto localTime = boost::posix_time::second_clock::universal_time();
    std::cout << '[' << localTime << "] " << *messageSPtr << '\n';
}

