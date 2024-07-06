#ifndef Session_HPP
#define Session_HPP

#include <cpprest/ws_client.h>

class Session {

public:
    Session(std::string fileName);

    void subscribe();
    void get_icons();
    void run_forever();

private:
    pplx::task<std::string> request_json_async();
    pplx::task<void> fetch_logo(const std::string& logo);
    std::string symbol_to_logo(const std::string& symbol);

    web::websockets::client::websocket_client Client_;
    std::string token_;
    std::vector<std::string> symbols_;
    std::vector<std::string> icon_mappings_;

};

#endif // Session_HPP