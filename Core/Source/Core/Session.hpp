#ifndef Session_HPP
#define Session_HPP

#include <cpprest/ws_client.h>

class Session {

public:
    Session(std::string fileName);

    void subscribe();
    void run_forever();

private:
    pplx::task<void> request_async();

    web::websockets::client::websocket_client Client_;
    std::string token_;
    std::vector<std::string> subscriptions_;
};

#endif // Session_HPP